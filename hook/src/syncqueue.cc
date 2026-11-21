#include <NickelHook.h>
#include <QDateTime>
#include <QLabel>
#include <QSettings>
#include <QTimer>

#include "cli.h"
#include "qglobal.h"
#include "review/reviewdialog.h"
#include "settings.h"
#include "syncqueue.h"

SyncQueue::SyncQueue(QObject *parent) : QObject(parent) {};

void SyncQueue::updateReadProgress(QString contentId) {
  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);
  if (!cv) return;

  int progress = ReadingView__getCalculatedReadProgress(cv);
  if (progress == 99) {
    progress = 100;
  }

  if (progress == 0 || progress == queue[contentId]) {
    return;
  }

  queue[contentId] = progress;

  nh_log("Update %s queued progress to %d%%", qPrintable(contentId), queue[contentId]);
}

int SyncQueue::getReadProgress(QString contentId) { return queue[contentId]; }

bool SyncQueue::checkThreshold(QString contentId, int threshold) {
  return threshold > 0 && queue[contentId] > 0 &&
         abs(Settings::getInstance()->getLastProgress(contentId) - queue[contentId]) >= threshold;
}

void SyncQueue::prepareNext() {
  QObject::connect(this, &SyncQueue::finished, this, &SyncQueue::prepareNext, Qt::UniqueConnection);

  QHashIterator<QString, int> i(queue);
  if (i.hasNext()) {
    i.next();

    QString contentId = i.key();
    if (Settings::getInstance()->isEnabled(contentId) && queue[contentId] > 0) {
      nh_log("Syncing %s", qPrintable(contentId));
      run(contentId);
    } else {
      nh_log("Removing %s from queue and skipping", qPrintable(contentId));
      queue.remove(contentId);
      prepareNext();
    }
  } else {
    nh_log("No more items in queue");
    QObject::disconnect(this, &SyncQueue::finished, this, &SyncQueue::prepareNext);
  }
}

void SyncQueue::run(QString contentId, bool manual, bool reread) {
  int progress = queue[contentId];
  queue.remove(contentId);

  if (progress == 0) {
    nh_log("Attempted to sync %s with no saved reading progress", qPrintable(contentId));
    ConfirmationDialogFactory__showErrorDialog("StoryGraph",
                                               "Reading progress must be at least 1% to sync with StoryGraph");
    return;
  }

  // Only sync forward — don't overwrite a higher StoryGraph progress if the user
  // navigated back in the book (e.g. returned to check an earlier chapter).
  if (!manual && progress < Settings::getInstance()->getLastProgress(contentId)) {
    nh_log("Skipping backward sync for %s: %d%% < last synced %d%%",
           qPrintable(contentId), progress, Settings::getInstance()->getLastProgress(contentId));
    finished();
    return;
  }

  this->contentId = contentId;
  this->lastProgress = progress;

  if (manual) {
    dialog = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
    ConfirmationDialog__showCloseButton(dialog, false);
    ConfirmationDialog__setText(dialog, "Syncing with StoryGraph...");
    dialog->open();
  }

  if (progress == 100) {
    Settings::getInstance()->setEnabled(contentId, false);
  }

  Settings::getInstance()->setLastProgress(contentId, progress);

  CLI::Options options;
  options.silent = !manual;
  options.icon = true;
  options.contentId = contentId;

  bool hasLinkedId = !Settings::getInstance()->getLinkedId(contentId).isEmpty();
  CLI *cli = CLI::update(progress, options, reread);
  QObject::connect(cli, &CLI::response, this, [this, contentId, hasLinkedId, manual](QJsonObject doc) {
    if (!hasLinkedId) {
      QString bookId = doc.value("resolved_book_id").toString();
      if (!bookId.isEmpty()) {
        Settings::getInstance()->setLinkedId(contentId, bookId);
      }
    }
    int statusId = doc.value("status_id").toInt(0);
    if (statusId > 0) {
      Settings::getInstance()->setStatus(contentId, statusId);
    }
    // Finished on StoryGraph, so nothing was synced (failure() follows). A
    // manual sync asks about a reread once the "Syncing..." dialog has closed.
    // An auto-sync stays silent and switches auto-sync off for the book, since
    // it can't do anything for it; the saved status makes turning it back on,
    // or opening the book (with auto-sync on by default), ask about a reread.
    if (doc.value("error_code").toString() == "BOOK_FINISHED") {
      if (manual) {
        QTimer::singleShot(0, this, [this, contentId]() { showRereadPrompt(contentId); });
      } else {
        nh_log("%s is finished on StoryGraph, turning auto-sync off for it", qPrintable(contentId));
        Settings::getInstance()->setEnabled(contentId, false);
      }
    }
    if (doc.value("needs_shelf_confirmation").toBool()) {
      pendingShelfConfirmation = true;
      pendingShelfProgress = doc.value("progress_percent").toInt(0);
    }
  });
  QObject::connect(cli, &CLI::success, this, &SyncQueue::success);
  QObject::connect(cli, &CLI::failure, this, &SyncQueue::closeDialog);
  if (manual) {
    QObject::connect(cli, &CLI::failure, this, &SyncQueue::finished);
  } else {
    QObject::connect(cli, &CLI::failure, this, [this, contentId]() {
      WirelessWorkflowManager *wfm = WirelessWorkflowManager__sharedInstance();
      if (!WirelessWorkflowManager__isInternetAccessible(wfm) && lastProgress > 0 && retryCount[contentId] < 3) {
        retryCount[contentId]++;
        queue[contentId] = lastProgress;
        nh_log("WiFi unavailable, retrying %s in 30s (attempt %d/3)", qPrintable(contentId), retryCount[contentId]);
        QTimer::singleShot(30000, this, &SyncQueue::prepareNext);
      } else {
        retryCount.remove(contentId);
        finished();
      }
    });
  }
}

void SyncQueue::success() {
  retryCount.remove(contentId);

  if (pendingShelfConfirmation) {
    showShelfConfirmation();
    return;
  }
  QString syncMode = Settings::getInstance()->getSyncBookmarks();
  bool syncBookmarks = (syncMode == "always") ||
                       (syncMode == "manual" && dialog != nullptr) ||
                       (syncMode == "finished" && lastProgress == 100);

  if (syncBookmarks) {
    CLI::Options options;
    options.silent = true;
    options.contentId = contentId;
    CLI::updateJournal(options);
  }

  finished();

  lastProgress = 0;

  if (dialog != nullptr) {
    ConfirmationDialog__setText(dialog, "Success!");
    QTimer::singleShot(800, this, &SyncQueue::closeDialog);
  }
}

void SyncQueue::showShelfConfirmation() {
  closeDialog();

  int shelfProgress = pendingShelfProgress;
  pendingShelfConfirmation = false;
  pendingShelfProgress = 0;

  ConfirmationDialog *prompt = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
  ConfirmationDialog__setTitle(prompt, "Already on your shelf");
  ConfirmationDialog__setText(prompt,
      QString("Your StoryGraph shelf has this book at %1%. Keep that progress?").arg(shelfProgress));
  ConfirmationDialog__setAcceptButtonText(prompt, "Keep StoryGraph progress");
  ConfirmationDialog__setRejectButtonText(prompt, "Sync from Kobo");
  ConfirmationDialog__showCloseButton(prompt, false);

  QObject::connect(prompt, &QDialog::accepted, this, [this, shelfProgress, prompt]() {
    prompt->deleteLater();
    Settings::getInstance()->setLastProgress(contentId, shelfProgress);
    success();
  });

  QObject::connect(prompt, &QDialog::rejected, this, [this, prompt]() {
    prompt->deleteLater();
    CLI::Options opts;
    opts.silent = false;
    opts.icon = true;
    opts.contentId = contentId;
    CLI *cli = CLI::update(lastProgress, opts);
    QObject::connect(cli, &CLI::success, this, &SyncQueue::success);
    QObject::connect(cli, &CLI::failure, this, &SyncQueue::closeDialog);
    QObject::connect(cli, &CLI::failure, this, &SyncQueue::finished);
  });

  prompt->open();
}

void SyncQueue::showCompletionPrompt(QString contentId) {
  ConfirmationDialog *prompt = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
  ConfirmationDialog__setTitle(prompt, "Book Complete!");
  ConfirmationDialog__setText(prompt, "Let StoryGraph know you finished: Write a review, or update your reading status?");
  ConfirmationDialog__setAcceptButtonText(prompt, "Write a review");
  ConfirmationDialog__setRejectButtonText(prompt, "Mark as finished");
  ConfirmationDialog__showCloseButton(prompt, true);

  QObject::connect(prompt, &QDialog::accepted, prompt, [prompt]() {
    ReviewDialog::show();
    prompt->deleteLater();
  });
  QObject::connect(prompt, &QDialog::rejected, this, [this, prompt, contentId]() {
    CLI::Options options;
    options.contentId = contentId;
    CLI *cli = CLI::setUserBook(3, options);
    // Saved so reopening the book later offers a reread.
    QObject::connect(cli, &CLI::response, this, [contentId](QJsonObject doc) {
      int status = doc.value("status_id").toInt(0);
      if (status > 0)
        Settings::getInstance()->setStatus(contentId, status);
    });
    prompt->deleteLater();
  });
  prompt->open();
}

void SyncQueue::showRereadPrompt(QString contentId, bool enableAutoSync) {
  ConfirmationDialog *prompt = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
  ConfirmationDialog__setTitle(prompt, "Start a reread?");
  ConfirmationDialog__setText(prompt,
      "This book is marked as read on StoryGraph. Start a new read-through and sync your progress?");
  ConfirmationDialog__setAcceptButtonText(prompt, "Start reread");
  ConfirmationDialog__setRejectButtonText(prompt, "Not now");
  ConfirmationDialog__showCloseButton(prompt, false);

  QObject::connect(prompt, &QDialog::accepted, this, [this, prompt, contentId, enableAutoSync]() {
    prompt->deleteLater();
    startReread(contentId, enableAutoSync);
  });
  QObject::connect(prompt, &QDialog::rejected, prompt, [prompt]() { prompt->deleteLater(); });

  prompt->open();
}

void SyncQueue::startReread(QString contentId, bool enableAutoSync) {
  Settings *settings = Settings::getInstance();

  // Auto-sync was likely switched off when the book reached 100% last time.
  if (enableAutoSync || settings->getAutoSyncDefault()) {
    settings->setEnabled(contentId, true);
  }

  updateReadProgress(contentId);
  if (queue.value(contentId) > 0) {
    run(contentId, true, true);
    return;
  }

  // At the very start of the book there's no progress to sync yet, so just set
  // the status: currently reading starts the reread, as on the website.
  queue.remove(contentId);
  settings->setLastProgress(contentId, 0);

  CLI::Options options;
  options.contentId = contentId;
  CLI *cli = CLI::setUserBook(2, options);
  QObject::connect(cli, &CLI::response, this, [contentId](QJsonObject doc) {
    int status = doc.value("status_id").toInt(0);
    if (status > 0)
      Settings::getInstance()->setStatus(contentId, status);
  });
}

void SyncQueue::closeDialog() {
  if (dialog == nullptr)
    return;

  dialog->close();
  dialog->deleteLater();
  dialog = nullptr;
}
