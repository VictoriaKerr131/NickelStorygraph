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
  bool pendingReviewPrompt = false;
  QHash<QString, int> queue;
  QHash<QString, int> retryCount;

  bool pendingShelfConfirmation = false;
  int pendingShelfProgress = 0;

  void showReviewPrompt();
  void showShelfConfirmation();
};
