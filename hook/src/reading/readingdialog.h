#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QStackedWidget>

#include "../widgets/homepanel.h"
#include "../widgets/pagedstack.h"

class ReadingDialog : public HomePanel {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void response(QJsonObject doc);
  void failed();
  void showReading();
  void showPaused();

private:
  ReadingDialog();

  void fillPages(PagedStack *stack, const QJsonArray &books);
  void setActiveTab(bool readingActive);

  QLabel         *statusLabel  = nullptr;
  QLabel         *readingTabBtn = nullptr;
  QLabel         *pausedTabBtn  = nullptr;
  QStackedWidget *tabContent   = nullptr;
  PagedStack     *readingPages = nullptr;
  PagedStack     *pausedPages  = nullptr;
};
