#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QNetworkReply>
#include <QStringList>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../files.h"
#include "../synccontroller.h"
#include "../widgets/elidedlabel.h"
#include "../widgets/label.h"
#include "readingrow.h"

ReadingRow::ReadingRow(QJsonObject json, QWidget *parent) : QFrame(parent) {
  setStyleSheet(R"(
    ReadingRow { padding: 12px 20px; border-top: 1px solid #e0e0e0; }

    [qApp_deviceIsTrilogy=true]  ReadingRow { padding: 10px 16px; }
    [qApp_deviceIsPhoenix=true]  ReadingRow { padding: 12px 20px; }
    [qApp_deviceIsDragon=true]   ReadingRow { padding: 16px 28px; }
    [qApp_deviceIsStorm=true]    ReadingRow { padding: 18px 32px; }
    [qApp_deviceIsDaylight=true] ReadingRow { padding: 20px 36px; }
    ReadingRow[noBorder=true] { border-top-width: 0; }

    [qApp_deviceIsTrilogy=true]  QLabel#cover { min-width: 56px;  max-width: 56px;  min-height: 84px;  max-height: 84px;  }
    [qApp_deviceIsPhoenix=true]  QLabel#cover { min-width: 68px;  max-width: 68px;  min-height: 102px; max-height: 102px; }
    [qApp_deviceIsDragon=true]   QLabel#cover { min-width: 92px;  max-width: 92px;  min-height: 138px; max-height: 138px; }
    [qApp_deviceIsStorm=true]    QLabel#cover { min-width: 104px; max-width: 104px; min-height: 156px; max-height: 156px; }
    [qApp_deviceIsDaylight=true] QLabel#cover { min-width: 118px; max-width: 118px; min-height: 177px; max-height: 177px; }

    QLabel#cover[blank=true] { background-color: #d9d9d9; }
    QLabel#author { color: #666666; }
    QLabel#meta   { color: #999999; }
    QLabel#pct    { color: #1FB784; font-weight: bold; }

    QFrame#progressTrack {
      background-color: #e8e8e8;
      min-height: 5px;
      max-height: 5px;
    }
    QFrame#progressFill {
      background-color: #1FB784;
      min-height: 5px;
      max-height: 5px;
    }
  )");

  QString title   = json.value("title").toString();
  QString author  = json.value("author").toString();
  int     pages   = json.value("pages").toInt();
  QString format  = json.value("format").toString();
  int     year    = json.value("year").toInt();
  int     percent = json.value("progress_percent").toInt();
  QString imgUrl  = json.value("cover_url").toString();

  QHBoxLayout *outer = new QHBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(10);

  // Cover thumbnail
  cover = new QLabel(this);
  cover->setObjectName("cover");
  cover->setScaledContents(true);
  if (imgUrl.isEmpty()) {
    cover->setProperty("blank", QVariant(true));
  } else {
    cover->setPixmap(QPixmap(Files::loading));
    QNetworkReply *reply = SyncController::getInstance()->network->get(QNetworkRequest(QUrl(imgUrl)));
    QObject::connect(reply, &QNetworkReply::finished, this, &ReadingRow::loadCover);
  }
  outer->addWidget(cover, 0, Qt::AlignVCenter);

  // Text + progress
  QVBoxLayout *text = new QVBoxLayout();
  text->setContentsMargins(0, 0, 0, 0);
  text->setSpacing(2);

  // Title — large and bold, matching the goals page category name weight.
  // Wrap instead of cutting off long titles; the row height estimate in
  // ReadingDialog::fillPages() accounts for up to two lines here.
  Label *titleLabel = new Label(Label::Large, title);
  titleLabel->setWordWrap(true);
  titleLabel->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"Large\"] { font-weight: bold; }"));
  text->addWidget(titleLabel);

  // Author — small, muted
  Label *authorLabel = new Label(Label::Small, author);
  authorLabel->setObjectName("author");
  text->addWidget(authorLabel);

  // Meta line (pages • format • year) — extra-small, very muted
  QStringList meta;
  if (pages > 0)         meta.append(QString::number(pages) + " pages");
  if (!format.isEmpty()) meta.append(format);
  if (year > 0)          meta.append(QString::number(year));
  if (!meta.isEmpty()) {
    Label *metaLabel = new Label(Label::ExtraSmall, meta.join(QStringLiteral(" • ")));
    metaLabel->setObjectName("meta");
    text->addWidget(metaLabel);
  }

  // Progress bar + teal percentage
  QWidget     *progRow    = new QWidget();
  QHBoxLayout *progLayout = new QHBoxLayout(progRow);
  progLayout->setContentsMargins(0, 6, 0, 0);
  progLayout->setSpacing(6);

  QFrame *track = new QFrame(progRow);
  track->setObjectName("progressTrack");
  QFrame *fill = new QFrame(track);
  fill->setObjectName("progressFill");
  QHBoxLayout *trackLayout = new QHBoxLayout(track);
  trackLayout->setContentsMargins(0, 0, 0, 0);
  trackLayout->setSpacing(0);
  if (percent > 0)   trackLayout->addWidget(fill, percent);
  if (percent < 100) trackLayout->addStretch(100 - percent);

  QLabel *pctLabel = new QLabel(QString::number(percent) + "%");
  pctLabel->setObjectName("pct");

  progLayout->addWidget(track, 1);
  progLayout->addWidget(pctLabel);

  text->addWidget(progRow);
  outer->addLayout(text, 1);
}

void ReadingRow::loadCover() {
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
