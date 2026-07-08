#pragma once

#include <QLabel>
#include <QVariant>
#include <QWidget>

#include "../nickelstoregraph.h"

class CheckboxRow : public QWidget {
  Q_OBJECT

public:
  CheckboxRow(QString heading, bool checked, QWidget *parent = nullptr);

public Q_SLOTS:
  void tapped();

Q_SIGNALS:
  void triggered(bool value);

private:
  TouchCheckBox *checkbox = nullptr;
  bool checked = false;
};
