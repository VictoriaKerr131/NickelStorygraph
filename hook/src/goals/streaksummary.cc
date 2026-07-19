#include <QHBoxLayout>
#include <QJsonObject>
#include <QVBoxLayout>

#include "../widgets/label.h"
#include "streaksummary.h"

static QWidget *statTile(QString caption, QString value, QString color, QWidget *parent) {
  QWidget     *tile   = new QWidget(parent);
  QVBoxLayout *layout = new QVBoxLayout(tile);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(1);
  layout->setAlignment(Qt::AlignHCenter);

  Label *captionLabel = new Label(Label::Small, caption.toUpper(), tile);
  captionLabel->setAlignment(Qt::AlignCenter);
  captionLabel->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"Medium\"] { color: #555555; }"));
  layout->addWidget(captionLabel);

  Label *valueLabel = new Label(Label::ExtraLarge, value.isEmpty() ? "—" : value, tile);
  valueLabel->setAlignment(Qt::AlignCenter);
  valueLabel->setStyleSheet(Label::Stylesheet +
                            QString("\n[textSize=\"ExtraLarge\"] { color: %1; font-weight: bold; }").arg(color));
  layout->addWidget(valueLabel);

  return tile;
}

StreakSummary::StreakSummary(QJsonObject json, QWidget *parent) : QFrame(parent) {
  setStyleSheet(R"(
    /* padding: top right bottom left — top is trimmed down from the other
       three since the "Streak" heading above already adds its own
       padding-bottom; bottom/left/right stay as-is for spacing below the
       stat tiles and around the edges. */
    StreakSummary { padding: 6px 22px 16px 22px; border-top: 1px solid #e0e0e0; }

    [qApp_deviceIsTrilogy=true]  StreakSummary { padding: 5px 20px 14px 20px; }
    [qApp_deviceIsPhoenix=true]  StreakSummary { padding: 6px 25px 18px 25px; }
    [qApp_deviceIsDragon=true]   StreakSummary { padding: 8px 37px 22px 37px; }
    [qApp_deviceIsStorm=true]    StreakSummary { padding: 9px 42px 26px 42px; }
    [qApp_deviceIsDaylight=true] StreakSummary { padding: 10px 48px 30px 48px; }
    StreakSummary[noBorder=true] { border-top-width: 0; }
  )");

  QString requirement  = json.value("requirement").toString();
  QString current      = json.value("current").toString();
  QString longest      = json.value("longest").toString();
  QString readingLeft  = json.value("reading_left").toString();

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);

  QWidget     *statsRow    = new QWidget(this);
  QHBoxLayout *statsLayout = new QHBoxLayout(statsRow);
  statsLayout->setContentsMargins(0, 0, 0, 0);
  statsLayout->addWidget(statTile("Current streak", current, "#000000", statsRow), 1);
  statsLayout->addWidget(statTile("Longest streak", longest, "#000000", statsRow), 1);
  layout->addWidget(statsRow);

  if (!requirement.isEmpty()) {
    Label *requirementLabel = new Label(Label::Small, requirement, this);
    requirementLabel->setAlignment(Qt::AlignCenter);
    requirementLabel->setWordWrap(true);
    requirementLabel->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"Small\"] { color: #666666; }"));
    layout->addWidget(requirementLabel);
  }

  if (!readingLeft.isEmpty()) {
    Label *readingLeftLabel = new Label(Label::ExtraSmall, readingLeft, this);
    readingLeftLabel->setAlignment(Qt::AlignCenter);
    readingLeftLabel->setWordWrap(true);
    readingLeftLabel->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"Small\"] { color: #999999; }"));
    layout->addWidget(readingLeftLabel);
  }
}
