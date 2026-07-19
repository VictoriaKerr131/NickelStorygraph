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
  // overlayHome: skip Nickel's normal pushView navigation and instead parent
  // the dialog directly onto the main window and raise it - same technique
  // HomePanel (widgets/homepanel.cc) uses for the Reading/Feed/Goals home
  // panels. Only needed for a dialog that can be opened from the home
  // screen, where pushView's target stack sits behind the persistent nav
  // bar chrome. Every Dialog subclass except SettingsDialog requires a
  // linked book and is only ever opened from within one, so this defaults
  // to false and leaves them unaffected.
  Dialog(QString title, bool overlayHome = false);

  N3Dialog *dialog = nullptr;

  KeyboardFrame *buildKeyboardFrame(TouchLineEdit *lineEdit, QString goText);
  KeyboardFrame *buildKeyboardFrame(QTextEdit *textEdit, QString goText);
  KeyboardFrame *buildKeyboardFrameHideOnly(QTextEdit *textEdit, QString goText);
};
