#pragma once

#include <QFrame>
#include <QJsonObject>
#include <QLabel>

class FeedRow : public QFrame {
  Q_OBJECT

public:
  FeedRow(QJsonObject json, QWidget *parent = nullptr);

public Q_SLOTS:
  void loadCover();

private:
  QLabel *cover = nullptr;
};
