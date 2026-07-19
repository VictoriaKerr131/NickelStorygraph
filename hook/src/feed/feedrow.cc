#include <QVariant>

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkReply>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../files.h"
#include "../synccontroller.h"
#include "../widgets/elidedlabel.h"
#include "../widgets/label.h"
#include "feedrow.h"

FeedRow::FeedRow(QJsonObject json, QWidget *parent) : QFrame(parent) {
  setStyleSheet(R"(
    FeedRow { padding: 12px 20px; border-top: 1px solid #e0e0e0; }

    [qApp_deviceIsTrilogy=true]  FeedRow { padding: 10px 16px; }
    [qApp_deviceIsPhoenix=true]  FeedRow { padding: 12px 20px; }
    [qApp_deviceIsDragon=true]   FeedRow { padding: 16px 28px; }
    [qApp_deviceIsStorm=true]    FeedRow { padding: 18px 32px; }
    [qApp_deviceIsDaylight=true] FeedRow { padding: 20px 36px; }
    FeedRow[noBorder=true] { border-top-width: 0; }

    [qApp_deviceIsTrilogy=true]  QLabel#cover { min-width: 42px; max-width: 42px; min-height: 63px;  max-height: 63px;  }
    [qApp_deviceIsPhoenix=true]  QLabel#cover { min-width: 52px; max-width: 52px; min-height: 78px;  max-height: 78px;  }
    [qApp_deviceIsDragon=true]   QLabel#cover { min-width: 70px; max-width: 70px; min-height: 105px; max-height: 105px; }
    [qApp_deviceIsStorm=true]    QLabel#cover { min-width: 80px; max-width: 80px; min-height: 120px; max-height: 120px; }
    [qApp_deviceIsDaylight=true] QLabel#cover { min-width: 90px; max-width: 90px; min-height: 135px; max-height: 135px; }

    QLabel#cover[blank=true] { background-color: #d9d9d9; }
    QLabel#rating  { color: #1FB784; font-weight: bold; }
    QLabel#timeAgo { color: #999999; }
    QLabel#author  { color: #666666; }
    QLabel#genres  { color: #2d8a6c; }
    QLabel#review  { color: #888888; }
  )");

  QString username       = json.value("username").toString();
  QString action         = json.value("action").toString();
  QString timeAgo        = json.value("time_ago").toString();
  QString bookTitle      = json.value("book_title").toString();
  QString bookAuthor     = json.value("book_author").toString();
  QString imgUrl         = json.value("cover_url").toString();
  QJsonArray genresArr   = json.value("genres").toArray();
  double ratingVal       = json.value("rating").toDouble(-1.0);
  QString reviewSnippet  = json.value("review_snippet").toString();

  QStringList genreList;
  for (const QJsonValue &g : genresArr)
    genreList.append(g.toString());

  QVBoxLayout *outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(6);

  // Header: "<b>username</b> action" on left, time on right
  QWidget     *header       = new QWidget(this);
  QHBoxLayout *headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(0, 0, 0, 0);
  headerLayout->setSpacing(4);

  QString headerHtml = QStringLiteral("<b>") + username.toHtmlEscaped()
                       + QStringLiteral("</b> ")
                       + action.toHtmlEscaped();
  Label *headerLabel = new Label(Label::Small, QString(), header);
  headerLabel->setText(headerHtml);
  headerLabel->setWordWrap(true);

  Label *timeLabel = new Label(Label::ExtraSmall, timeAgo, header);
  timeLabel->setObjectName("timeAgo");
  timeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

  headerLayout->addWidget(headerLabel, 1);
  headerLayout->addWidget(timeLabel, 0, Qt::AlignTop);
  outer->addWidget(header);

  // Book row: cover + info
  QWidget     *bookRow    = new QWidget(this);
  QHBoxLayout *bookLayout = new QHBoxLayout(bookRow);
  bookLayout->setContentsMargins(0, 0, 0, 0);
  bookLayout->setSpacing(10);

  cover = new QLabel(this);
  cover->setObjectName("cover");
  cover->setScaledContents(true);
  if (imgUrl.isEmpty()) {
    cover->setProperty("blank", QVariant(true));
  } else {
    cover->setPixmap(QPixmap(Files::loading));
    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(imgUrl)));
    QObject::connect(reply, &QNetworkReply::finished, this, &FeedRow::loadCover);
  }
  bookLayout->addWidget(cover, 0, Qt::AlignTop);

  QVBoxLayout *info = new QVBoxLayout();
  info->setContentsMargins(0, 0, 0, 0);
  info->setSpacing(3);

  // Title — large and bold
  Label *titleLabel = new Label(Label::Large, bookTitle);
  titleLabel->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"Large\"] { font-weight: bold; }"));
  info->addWidget(titleLabel);

  // Author — small, muted
  Label *authorLabel = new Label(Label::Small, bookAuthor);
  authorLabel->setObjectName("author");
  info->addWidget(authorLabel);

  // Genres — extra-small, teal
  if (!genreList.isEmpty()) {
    Label *genresLabel = new Label(Label::ExtraSmall, genreList.join(QStringLiteral(" • ")));
    genresLabel->setObjectName("genres");
    genresLabel->setWordWrap(true);
    info->addWidget(genresLabel);
  }

  // Rating — small, teal star
  if (ratingVal >= 0.0) {
    Label *ratingLabel = new Label(Label::Small, QStringLiteral("★ ") + QString::number(ratingVal, 'f', 1));
    ratingLabel->setObjectName("rating");
    info->addWidget(ratingLabel);
  }

  // Review snippet — extra-small, muted, capped at 3 lines
  if (!reviewSnippet.isEmpty()) {
    ElidedLabel *reviewLabel = new ElidedLabel(Label::ExtraSmall, reviewSnippet, 3);
    reviewLabel->setObjectName("review");
    info->addWidget(reviewLabel);
  }

  bookLayout->addLayout(info, 1);
  outer->addWidget(bookRow);
}

void FeedRow::loadCover() {
  QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
  if (reply->error() == QNetworkReply::NoError) {
    QPixmap pixmap;
    pixmap.loadFromData(reply->readAll());
    cover->setPixmap(pixmap);
  } else {
    cover->clear();
    cover->setProperty("blank", QVariant(true));
  }
  reply->deleteLater();
}
