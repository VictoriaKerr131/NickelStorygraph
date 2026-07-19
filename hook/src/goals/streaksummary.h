#pragma once

#include <QFrame>
#include <QJsonObject>

class StreakSummary : public QFrame {
  Q_OBJECT

public:
  StreakSummary(QJsonObject json, QWidget *parent = nullptr);
};
