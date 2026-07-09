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

void SyncQueue::run(QString contentId, bool manual) {
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
  CLI *cli = CLI::update(progress, options);
  QObject::connect(cli, &CLI::response, this, [this, contentId, hasLinkedId](QJsonObject doc) {
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
    if (doc.value("needs_shelf_confirmation").toBool()) {
      pendingShelfConfirmation = true;
      pendingShelfProgress = doc.value("progress_percent").toInt(0);
    }
    if (lastProgress == 100)
      pendingReviewPrompt = true;
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
  bool prompt = pendingReviewPrompt;
  pendingReviewPrompt = false;

  if (dialog != nullptr) {
    ConfirmationDialog__setText(dialog, "Success!");
    if (prompt) {
      QTimer::singleShot(800, this, [this]() { closeDialog(); showReviewPrompt(); });
    } else {
      QTimer::singleShot(800, this, &SyncQueue::closeDialog);
    }
  } else if (prompt) {
    showReviewPrompt();
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

void SyncQueue::showReviewPrompt() {
  ConfirmationDialog *prompt = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
  ConfirmationDialog__setTitle(prompt, "Book complete!");
  ConfirmationDialog__setText(prompt, "Would you like to write a review?");
  ConfirmationDialog__setAcceptButtonText(prompt, "Yes");
  ConfirmationDialog__setRejectButtonText(prompt, "No");
  ConfirmationDialog__showCloseButton(prompt, false);
  QObject::connect(prompt, &QDialog::accepted, prompt, [prompt]() {
    ReviewDialog::show();
    prompt->deleteLater();
  });
  QObject::connect(prompt, &QDialog::rejected, prompt, &QDialog::deleteLater);
  prompt->open();
}

void SyncQueue::closeDialog() {
  if (dialog == nullptr)
    return;

  dialog->close();
  dialog->deleteLater();
  dialog = nullptr;
}
