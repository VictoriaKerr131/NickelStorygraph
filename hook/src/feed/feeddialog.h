#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>

#include "../widgets/homepanel.h"
#include "../widgets/pagedstack.h"

class FeedDialog : public HomePanel {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void response(QJsonObject doc);
  void failed();

private:
  FeedDialog();
  void fillPages(const QJsonArray &items);

  QLabel     *statusLabel = nullptr;
  PagedStack *pages       = nullptr;
};
