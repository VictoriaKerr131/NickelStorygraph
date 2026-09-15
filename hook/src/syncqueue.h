#pragma once

#include <QLabel>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>

#include "nickelstoregraph.h"

class SyncQueue : public QObject {
  Q_OBJECT

public:
  SyncQueue(QObject *parent = nullptr);

  void updateReadProgress(QString contentId);
  int getReadProgress(QString contentId);
  bool checkThreshold(QString contentId, int threshold);

  void run(QString contentId, bool manual = false);

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
};
