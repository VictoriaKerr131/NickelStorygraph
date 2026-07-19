#pragma once

#include <QJsonObject>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "../widgets/homepanel.h"

class GoalsDialog : public HomePanel {
  Q_OBJECT

public:
  static void show();

public Q_SLOTS:
  void response(QJsonObject doc);
  void failed();
  void streakResponse(QJsonObject doc);

private:
  GoalsDialog();

  QLabel      *statusLabel   = nullptr;
  QWidget     *goalsWidget   = nullptr;
  QVBoxLayout *goalsLayout   = nullptr;
};
