#include <QHBoxLayout>
#include <QJsonObject>
#include <QLabel>
#include <QVBoxLayout>

#include "../widgets/elidedlabel.h"
#include "../widgets/label.h"
#include "goalsrow.h"

GoalsRow::GoalsRow(QJsonObject json, QWidget *parent) : QFrame(parent) {
  setStyleSheet(R"(
    GoalsRow { padding: 16px 22px; border-top: 1px solid #e0e0e0; }

    [qApp_deviceIsTrilogy=true]  GoalsRow { padding: 14px 20px; }
    [qApp_deviceIsPhoenix=true]  GoalsRow { padding: 18px 25px; }
    [qApp_deviceIsDragon=true]   GoalsRow { padding: 22px 37px; }
    [qApp_deviceIsStorm=true]    GoalsRow { padding: 26px 42px; }
    [qApp_deviceIsDaylight=true] GoalsRow { padding: 30px 48px; }
    GoalsRow[noBorder=true] {
      border-top-width: 0;
    }
    QFrame#progressTrack {
      background-color: #d9d9d9;
      min-height: 6px;
      max-height: 6px;
    }
  )");

  QString label   = json.value("label").toString();
  QString current = json.value("current").toString();
  QString target  = json.value("target").toString();
  QString unit    = json.value("unit").toString();
  int     percent = json.value("percent").toInt();
  QString status  = json.value("status").toString();

  // Map label to StoryGraph's own brand colors
  QString color;
  if (label == "Books")      color = "#0E7490";
  else if (label == "Pages") color = "#8B08BB";
  else if (label == "Hours") color = "#BF5600";
  else                       color = "#1FB784";

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(4);

  // Label name ("BOOKS", "PAGES", "HOURS") — use Label (QLabel) so color applies reliably
  Label *nameLabel = new Label(Label::Large, label.toUpper(), this);
  nameLabel->setStyleSheet(Label::Stylesheet + QString("\n[textSize=\"Large\"] { color: %1; font-weight: bold; }").arg(color));
  layout->addWidget(nameLabel);

  // Fraction: "10 / 26 books"
  if (!current.isEmpty() && !target.isEmpty()) {
    ElidedLabel *fractionLabel = new ElidedLabel(Label::Medium, current + " / " + target + " " + unit);
    layout->addWidget(fractionLabel);
  }

  // Progress bar + percentage
  QWidget     *progRow    = new QWidget(this);
  QHBoxLayout *progLayout = new QHBoxLayout(progRow);
  progLayout->setContentsMargins(0, 4, 0, 0);
  progLayout->setSpacing(8);

  QFrame      *track       = new QFrame(progRow);
  track->setObjectName("progressTrack");
  QFrame      *fill        = new QFrame(track);
  fill->setStyleSheet(
    QString("background-color: %1; min-height: 6px; max-height: 6px;").arg(color));
  QHBoxLayout *trackLayout = new QHBoxLayout(track);
  trackLayout->setContentsMargins(0, 0, 0, 0);
  trackLayout->setSpacing(0);
  if (percent > 0)   trackLayout->addWidget(fill, percent);
  if (percent < 100) trackLayout->addStretch(100 - percent);

  progLayout->addWidget(track, 1);
  progLayout->addWidget(new QLabel(QString::number(percent) + "%"));
  layout->addWidget(progRow);

  if (!status.isEmpty())
    layout->addWidget(new ElidedLabel(Label::Small, status));
}
