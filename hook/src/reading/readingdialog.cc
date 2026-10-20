#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QStackedWidget>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include <NickelHook.h>

#include "../cli.h"
#include "../nickelstorygraph.h"
#include "readingdialog.h"
#include "readingrow.h"
#include "../widgets/label.h"

void ReadingDialog::show() { new ReadingDialog(); }

static const QString TAB_ACTIVE = QStringLiteral(
  "font-size: 32px;"
  "font-weight: bold;"
  "border-bottom: 3px solid #1FB784;"
  "background-color: white;"
  "color: black;"
  "padding: 8px 16px;");

static const QString TAB_INACTIVE = QStringLiteral(
  "font-size: 32px;"
  "border-bottom: 3px solid transparent;"
  "background-color: #eeeeee;"
  "color: #888888;"
  "padding: 8px 16px;");

ReadingDialog::ReadingDialog() : HomePanel("") {  
  Label *heading = new Label(Label::ExtraLarge, "Currently Reading", this);
  heading->setAlignment(Qt::AlignCenter);
  heading->setStyleSheet(Label::Stylesheet + QStringLiteral("\n[textSize=\"ExtraLarge\"] { font-weight: bold; padding-top: 16px; padding-bottom: 12px; }"));
  contentLayout->addWidget(heading);
  contentLayout->addSpacing(10);

  statusLabel = new QLabel("Loading...");
  statusLabel->setAlignment(Qt::AlignCenter);
  contentLayout->addWidget(statusLabel, 1);


  // Tab bar
  QWidget     *tabBar    = new QWidget(this);
  QHBoxLayout *tabLayout = new QHBoxLayout(tabBar);
  tabLayout->setContentsMargins(0, 0, 0, 0);
  tabLayout->setSpacing(0);

  readingTabBtn = reinterpret_cast<QLabel *>(construct_TouchLabel(tabBar));
  readingTabBtn->setText("Reading");
  readingTabBtn->setAlignment(Qt::AlignCenter);
  readingTabBtn->setStyleSheet(TAB_ACTIVE);
  QObject::connect(readingTabBtn, SIGNAL(tapped(bool)), this, SLOT(showReading()));
  tabLayout->addWidget(readingTabBtn, 1);

  pausedTabBtn = reinterpret_cast<QLabel *>(construct_TouchLabel(tabBar));
  pausedTabBtn->setText("Paused");
  pausedTabBtn->setAlignment(Qt::AlignCenter);
  pausedTabBtn->setStyleSheet(TAB_INACTIVE);
  QObject::connect(pausedTabBtn, SIGNAL(tapped(bool)), this, SLOT(showPaused()));
  tabLayout->addWidget(pausedTabBtn, 1);

  tabBar->hide();
  contentLayout->addWidget(tabBar);
  contentLayout->addSpacing(10);

  // Two PagedStacks inside a QStackedWidget
  tabContent = new QStackedWidget(this);
  readingPages = new PagedStack(tabContent);
  pausedPages  = new PagedStack(tabContent);
  tabContent->addWidget(readingPages);
  tabContent->addWidget(pausedPages);
  tabContent->hide();
  contentLayout->addWidget(tabContent, 1);

  // Store tabBar pointer so we can show it in response()
  tabBar->setObjectName("tabBar");

  CLI *cli = CLI::listReading();
  QObject::connect(cli, &CLI::response, this, &ReadingDialog::response);
  QObject::connect(cli, &CLI::failure,  this, &ReadingDialog::failed);
}

void ReadingDialog::response(QJsonObject doc) {
  QJsonArray books = doc.value("books").toArray();

  // Separate into reading and paused
  QJsonArray reading, paused;
  for (const QJsonValue &v : books) {
    QJsonObject b = v.toObject();
    if (b.value("status").toString() == "paused")
      paused.append(b);
    else
      reading.append(b);
  }

  // Update tab labels with counts
  readingTabBtn->setText(QString("Reading (%1)").arg(reading.size()));
  pausedTabBtn->setText(QString("Paused (%1)").arg(paused.size()));

  statusLabel->hide();

  QWidget *tabBar = findChild<QWidget *>("tabBar");
  if (tabBar) tabBar->show();
  tabContent->show();

  fillPages(readingPages, reading);
  fillPages(pausedPages, paused);

  // Start on Reading tab
  tabContent->setCurrentIndex(0);
}

void ReadingDialog::fillPages(PagedStack *stack, const QJsonArray &books) {
  if (books.isEmpty()) {
    stack->setStatusText("Nothing here.");
    stack->setTotal(0);
    return;
  }

  // Fixed page size. Height-based fitting is unreliable here: the stacks are
  // filled before layout has run, so getAvailableHeight() isn't trustworthy
  // and pages were over-filled (rows squished).
  static const int ROWS_PER_PAGE = 4;
  static const int ROW_GAP = 10;

  QWidget     *page = nullptr;
  QVBoxLayout *rows = nullptr;

  for (int i = 0; i < books.size(); i++) {
    if (i % ROWS_PER_PAGE == 0) {
      if (page) {
        rows->addStretch(1);
        stack->addPage(page);
      }

      page = new QWidget(this);
      rows = new QVBoxLayout(page);
      rows->setContentsMargins(0, 0, 0, 0);
      rows->setSpacing(ROW_GAP);
    }

    ReadingRow *row = new ReadingRow(books.at(i).toObject(), stack);
    if (i % ROWS_PER_PAGE == 0) row->setProperty("noBorder", QVariant(true));
    rows->addWidget(row);
  }

  rows->addStretch(1);
  stack->addPage(page);
  stack->setTotal(stack->countPages());
  stack->next(); // jumps to page 1 after all pages built
}

void ReadingDialog::setActiveTab(bool readingActive) {
  readingTabBtn->setStyleSheet(readingActive ? TAB_ACTIVE : TAB_INACTIVE);
  pausedTabBtn->setStyleSheet(readingActive ? TAB_INACTIVE : TAB_ACTIVE);
  tabContent->setCurrentIndex(readingActive ? 0 : 1);
}

void ReadingDialog::showReading() { setActiveTab(true); }
void ReadingDialog::showPaused()  { setActiveTab(false); }

void ReadingDialog::failed() {
  statusLabel->setText("Failed to load books.");
}
