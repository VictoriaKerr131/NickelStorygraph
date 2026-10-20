#pragma once

#include <QVBoxLayout>
#include <QWidget>

#include "../nickelstorygraph.h"

class Rating : public QWidget {
  Q_OBJECT

public:
  Rating(float value, QWidget *parent = nullptr);
  void setValue(float value);

public Q_SLOTS:
  void mouseDown();

Q_SIGNALS:
  void tapped(float value);
};
