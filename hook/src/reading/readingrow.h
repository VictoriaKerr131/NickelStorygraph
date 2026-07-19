#pragma once

#include <QFrame>
#include <QJsonObject>
#include <QLabel>

class ReadingRow : public QFrame {
  Q_OBJECT

public:
  ReadingRow(QJsonObject json, QWidget *parent = nullptr);

public Q_SLOTS:
  void loadCover();

private:
  QLabel *cover = nullptr;
};
