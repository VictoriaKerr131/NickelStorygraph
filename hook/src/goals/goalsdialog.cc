#include <QVariant>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "../cli.h"
#include "../widgets/label.h"
#include "goalsdialog.h"
#include "goalsrow.h"
#include "streaksummary.h"

void GoalsDialog::show() { new GoalsDialog(); }

GoalsDialog::GoalsDialog() : HomePanel("") {
  statusLabel = new QLabel("Loading...");
  statusLabel->setAlignment(Qt::AlignCenter);
  contentLayout->addWidget(statusLabel, 1);

  goalsWidget = new QWidget();
  goalsLayout = new QVBoxLayout(goalsWidget);
  goalsLayout->setContentsMargins(0, 0, 0, 0);
  goalsLayout->setSpacing(12);
  goalsWidget->hide();
  contentLayout->addWidget(goalsWidget);

  CLI *cli = CLI::getGoals();
  QObject::connect(cli, &CLI::response, this, &GoalsDialog::response);
  QObject::connect(cli, &CLI::failure,  this, &GoalsDialog::failed);

  // Streak is independent of yearly goals (a user may have one without the
  // other), so it's fetched separately and shown regardless of whether
  // goals load successfully.
  CLI *streakCli = CLI::getStreak();
  QObject::connect(streakCli, &CLI::response, this, &GoalsDialog::streakResponse);
}

void GoalsDialog::response(QJsonObject doc) {
  QJsonArray goals     = doc.value("goals").toArray();
  QString    year      = doc.value("year").toString();
  int        booksRead = doc.value("books_read").toInt();

  if (goals.isEmpty()) {
    statusLabel->setText("No reading goals set for this year.");
    return;
  }

  statusLabel->hide();

  // Large centered year title
  Label *yearLabel = new Label(Label::ExtraLarge, year + " Reading Goals", goalsWidget);
  yearLabel->setAlignment(Qt::AlignCenter);
  yearLabel->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"ExtraLarge\"] { font-weight: bold; padding-top: 16px; padding-bottom: 4px; }"));
  goalsLayout->addWidget(yearLabel);

  // Medium centered books-read sub-header
  if (booksRead > 0) {
    Label *booksLabel = new Label(Label::Medium, QString::number(booksRead) + " books read", goalsWidget);
    booksLabel->setAlignment(Qt::AlignCenter);
    booksLabel->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"Medium\"] { color: #555555; padding-bottom: 12px; }"));
    goalsLayout->addWidget(booksLabel);
  }

  for (int i = 0; i < goals.size(); i++) {
    GoalsRow *row = new GoalsRow(goals.at(i).toObject(), goalsWidget);
    if (i == 0) row->setProperty("noBorder", QVariant(true));
    goalsLayout->addWidget(row);
  }

  goalsWidget->show();
}

void GoalsDialog::failed() {
  statusLabel->setText("Failed to load reading goals.");
}

void GoalsDialog::streakResponse(QJsonObject doc) {
  QString current = doc.value("current").toString();
  if (current.isEmpty()) return; // no streak set up on StoryGraph

  // Appended (not inserted at a fixed index) so it always lands after
  // statusLabel/goalsWidget regardless of whether this or the goals
  // response arrives first — those two are already in contentLayout from
  // the constructor, so appending here always puts the streak section
  // after them, at the bottom of the panel.

  contentLayout->addSpacing(10);

  // Large centered "Reading Streak" title, matching the "<year> Reading Goals" heading
  Label *heading = new Label(Label::ExtraLarge, "Reading Streak", this);
  heading->setAlignment(Qt::AlignCenter);
  heading->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"ExtraLarge\"] { font-weight: bold; padding-top: 16px; padding-bottom: 4px; }"));
  contentLayout->addWidget(heading);

  StreakSummary *streak = new StreakSummary(doc, this);
  streak->setProperty("noBorder", QVariant(true));
  contentLayout->addWidget(streak);
}
