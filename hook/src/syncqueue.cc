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
      int prevStatus = Settings::getInstance()->getStatus(contentId);
      Settings::getInstance()->setStatus(contentId, statusId);
      if (statusId == 3 && prevStatus != 3)
        pendingReviewPrompt = true;
    }
    if (lastProgress == 100)
      pendingReviewPrompt = true;
  });
  QObject::connect(cli, &CLI::success, this, &SyncQueue::success);
  QObject::connect(cli, &CLI::failure, this, &SyncQueue::closeDialog);
  QObject::connect(cli, &CLI::failure, this, &SyncQueue::finished);
}

void SyncQueue::success() {
  if (dialog != nullptr) {
    // Manual sync: also pull in any new annotations silently.
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
