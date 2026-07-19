#pragma once

#include <QFrame>
#include <QJsonObject>

class GoalsRow : public QFrame {
  Q_OBJECT

public:
  GoalsRow(QJsonObject json, QWidget *parent = nullptr);
};
