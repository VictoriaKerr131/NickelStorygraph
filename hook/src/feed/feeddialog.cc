#include <QVariant>

#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "../cli.h"
#include "../widgets/label.h"
#include "feeddialog.h"
#include "feedrow.h"

void FeedDialog::show() { new FeedDialog(); }

FeedDialog::FeedDialog() : HomePanel("") {
  Label *heading = new Label(Label::ExtraLarge, "Community Feed", this);
  heading->setAlignment(Qt::AlignCenter);
  heading->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"ExtraLarge\"] { font-weight: bold; padding-top: 16px; padding-bottom: 12px; }"));
  contentLayout->addWidget(heading);

  statusLabel = new QLabel("Loading...");
  statusLabel->setAlignment(Qt::AlignCenter);
  contentLayout->addWidget(statusLabel, 1);

  pages = new PagedStack(this);
  pages->hide();
  contentLayout->addWidget(pages, 1);

  CLI *cli = CLI::getFeed();
  QObject::connect(cli, &CLI::response, this, &FeedDialog::response);
  QObject::connect(cli, &CLI::failure,  this, &FeedDialog::failed);
}

void FeedDialog::response(QJsonObject doc) {
  QJsonArray items = doc.value("items").toArray();
  statusLabel->hide();
  pages->show();
  fillPages(items);
}

void FeedDialog::fillPages(const QJsonArray &items) {
  if (items.isEmpty()) {
    pages->setStatusText("No activity in your feed.");
    pages->setTotal(0);
    return;
  }

  // Feed rows vary in height. Rows with reviews (header + cover + title + author +
  // genres + rating + 3-line review + padding) reach ~180-200px. Use 250 so the
  // page never overflows and clips the bottom of the last row.
  static const int ROW_H  = 230;
  static const int ROW_GAP = 10;

  int availableHeight = pages->getAvailableHeight();
  if (availableHeight <= 0) availableHeight = 800;

  QWidget     *page = new QWidget(this);
  QVBoxLayout *rows = new QVBoxLayout(page);
  rows->setContentsMargins(0, 0, 0, 0);
  rows->setSpacing(ROW_GAP);

  int  pageHeight  = 0;
  bool firstOnPage = true;

  for (int i = 0; i < items.size(); i++) {
    FeedRow *row = new FeedRow(items.at(i).toObject(), pages);
    int cost = ROW_H + (firstOnPage ? 0 : ROW_GAP);

    if (!firstOnPage && pageHeight + cost > availableHeight) {
      rows->addStretch(1);
      pages->addPage(page);

      page = new QWidget(this);
      rows = new QVBoxLayout(page);
      rows->setContentsMargins(0, 0, 0, 0);
      rows->setSpacing(ROW_GAP);
      pageHeight  = 0;
      firstOnPage = true;
      cost        = ROW_H;
    }

    if (firstOnPage) row->setProperty("noBorder", QVariant(true));
    rows->addWidget(row);
    pageHeight += cost;
    firstOnPage = false;
  }

  rows->addStretch(1);
  pages->addPage(page);
  pages->setTotal(pages->countPages());
  pages->next();
}

void FeedDialog::failed() {
  statusLabel->setText("Failed to load community feed.");
}
