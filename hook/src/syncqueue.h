#pragma once

#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>

#include "nickelstorygraph.h"

class SyncQueue : public QObject {
  Q_OBJECT

public:
  SyncQueue(QObject *parent = nullptr);

  void updateReadProgress(QString contentId);
  int getReadProgress(QString contentId);
  bool checkThreshold(QString contentId, int threshold);

  // `reread`: the user agreed to start a reread if the book is finished on
  // StoryGraph. Without it, a finished book isn't synced: a manual sync then
  // asks, an auto-sync just remembers the status for the prompt on opening.
  void run(QString contentId, bool manual = false, bool reread = false);

  // Asks whether to start a reread of a book finished on StoryGraph. On
  // "Start reread", syncs with a reread and turns auto-sync on for the book
  // if `enableAutoSync` or auto-sync is on by default. "Not now" changes nothing.
  void showRereadPrompt(QString contentId, bool enableAutoSync = false);

  // Fired independently of auto-sync/threshold state, the moment a book's
  // reading progress first reaches 100% - offers to write a review or mark
  // the book finished without requiring a server round-trip first.
  void showCompletionPrompt(QString contentId);

public Q_SLOTS:
  void prepareNext();
  void success();
  void closeDialog();

Q_SIGNALS:
  void finished();

private:
  ConfirmationDialog *dialog = nullptr;

  QString contentId;
  int lastProgress = 0;
  QHash<QString, int> queue;
  QHash<QString, int> retryCount;

  bool pendingShelfConfirmation = false;
  int pendingShelfProgress = 0;

  void showShelfConfirmation();
  void startReread(QString contentId, bool enableAutoSync);
};
