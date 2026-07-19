#include <QHBoxLayout>
#include <QWidgetAction>

#include <NickelHook.h>

#include "../widgets/label.h"
#include "checkboxrow.h"

CheckboxRow::CheckboxRow(QString heading, bool checked, QWidget *parent) : QWidget(parent), checked(checked) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  SettingContainer *row = construct_SettingContainer(this);
  layout->addWidget(row);

  QHBoxLayout *rowLayout = new QHBoxLayout(row);
  rowLayout->setContentsMargins(0, 0, 0, 0);
  headingLabel = new Label(Label::Medium, heading);
  rowLayout->addWidget(headingLabel, 1);

  checkbox = construct_TouchCheckBox(this);
  checkbox->setText(checked ? "On" : "Off");
  checkbox->setChecked(checked);
  checkbox->setAttribute(Qt::WA_TransparentForMouseEvents);
  rowLayout->addWidget(checkbox);
  QObject::connect(row, SIGNAL(tapped()), this, SLOT(tapped()));
}

void CheckboxRow::tapped() {
  if (!rowEnabled) return;

  nh_log("CheckboxRow::tapped()");

  checked = !checked;
  checkbox->setChecked(checked);
  checkbox->setText(checked ? "On" : "Off");
  triggered(checked);
}

void CheckboxRow::setRowEnabled(bool enabled) {
  rowEnabled = enabled;
  setEnabled(enabled);
  headingLabel->setEnabled(enabled);
  checkbox->setEnabled(enabled);
}
