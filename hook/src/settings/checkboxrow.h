#pragma once

#include <QLabel>
#include <QVariant>
#include <QWidget>

#include "../nickelstoregraph.h"
#include "../widgets/label.h"

class CheckboxRow : public QWidget {
  Q_OBJECT

public:
  CheckboxRow(QString heading, bool checked, QWidget *parent = nullptr);

  // Greys the row out and stops it from responding to taps - used for
  // sub-options that are meaningless while their parent toggle is off.
  void setRowEnabled(bool enabled);

public Q_SLOTS:
  void tapped();

Q_SIGNALS:
  void triggered(bool value);

private:
  Label *headingLabel = nullptr;
  TouchCheckBox *checkbox = nullptr;
  bool checked = false;
  bool rowEnabled = true;
};
