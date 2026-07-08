#pragma once

#include <QFrame>

#include "../nickelstoregraph.h"

class Dialog : public QFrame {
  Q_OBJECT

public Q_SLOTS:
  void currentViewChanged(QString name);
  void showKeyboard();
  void hideKeyboard();

  virtual void commit() {}

private:
  KeyboardFrame *buildKeyboardFrame(KeyboardReceiver *receiver, QString goText);

protected:
  Dialog(QString title);

  N3Dialog *dialog = nullptr;

  KeyboardFrame *buildKeyboardFrame(TouchLineEdit *lineEdit, QString goText);
  KeyboardFrame *buildKeyboardFrame(QTextEdit *textEdit, QString goText);
  KeyboardFrame *buildKeyboardFrameHideOnly(QTextEdit *textEdit, QString goText);
};
