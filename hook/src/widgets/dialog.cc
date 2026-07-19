#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScreen>
#include <QTextEdit>

#include <NickelHook.h>

#include "../synccontroller.h"
#include "dialog.h"

Dialog::Dialog(QString title, bool overlayHome) : QFrame() {
  dialog = N3DialogFactory__getDialog(this, true);
  N3Dialog__setTitle(dialog, title);

  QScreen *screen = QApplication::primaryScreen();
  QRect screenGeometry = screen->geometry();
  dialog->setFixedSize(screenGeometry.width(), screenGeometry.height());

  if (overlayHome) {
    // pushView's target stack sits behind the persistent home-screen nav bar
    // chrome, so a dialog pushed while sitting on the home screen renders
    // underneath it - same root cause HomePanel was built to solve for the
    // Reading/Feed/Goals panels. Skip pushView entirely and instead parent
    // directly onto the main window and raise, like HomePanel does.
    MainWindowController *mwc = MainWindowController__sharedInstance();
    QWidget *cv = MainWindowController__currentView(mwc);
    QWidget *mainWindow = cv ? cv->window() : nullptr;
    if (mainWindow) {
      dialog->setParent(mainWindow);
      dialog->move(0, 0);
      dialog->raise();
    }
  } else {
    MainWindowController *mwc = MainWindowController__sharedInstance();
    MainWindowController__pushView(mwc, dialog);
  }

  // Connected after pushView (not before): pushView synchronously swaps
  // Nickel's current view, which fires currentViewChanged with a name that
  // isn't "ReadingView" - if connected first, currentViewChanged() reads
  // that as "navigated away from reading" and immediately deleteLater()s
  // the dialog that's still being constructed.
  QObject::connect(SyncController::getInstance(), &SyncController::currentViewChanged, this,
                   &Dialog::currentViewChanged);
  QObject::connect(dialog, SIGNAL(closeTapped()), dialog, SLOT(deleteLater()));

  dialog->show();
}

void Dialog::currentViewChanged(QString name) {
  if (name != "ReadingView") {
    dialog->deleteLater();
  }
}

void Dialog::showKeyboard() { N3Dialog__showKeyboard(dialog); }

void Dialog::hideKeyboard() { N3Dialog__hideKeyboard(dialog); }

KeyboardFrame *Dialog::buildKeyboardFrame(TouchLineEdit *lineEdit, QString goText) {
  KeyboardReceiver *receiver = construct_KeyboardReceiver(lineEdit);
  KeyboardFrame *keyboard = buildKeyboardFrame(receiver, goText);
  QObject::connect(lineEdit, SIGNAL(tapped()), this, SLOT(showKeyboard()));

  return keyboard;
}

KeyboardFrame *Dialog::buildKeyboardFrame(QTextEdit *textEdit, QString goText) {
  KeyboardReceiver *receiver = construct_KeyboardReceiver(textEdit);
  KeyboardFrame *keyboard = buildKeyboardFrame(receiver, goText);

  return keyboard;
}

KeyboardFrame *Dialog::buildKeyboardFrameHideOnly(QTextEdit *textEdit, QString goText) {
  KeyboardReceiver *receiver = construct_KeyboardReceiver(textEdit);
  KeyboardFrame *keyboard = N3Dialog__keyboardFrame(dialog);

  SearchKeyboardController *ctl = KeyboardFrame__createKeyboard(keyboard, 0, locale());
  SearchKeyboardController__setReceiver(ctl, receiver, false);
  SearchKeyboardController__setGoText(ctl, goText);

  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(hideKeyboard()));

  return keyboard;
}

KeyboardFrame *Dialog::buildKeyboardFrame(KeyboardReceiver *receiver, QString goText) {
  KeyboardFrame *keyboard = N3Dialog__keyboardFrame(dialog);

  SearchKeyboardController *ctl = KeyboardFrame__createKeyboard(keyboard, 0, locale());
  SearchKeyboardController__setReceiver(ctl, receiver, false);
  SearchKeyboardController__setGoText(ctl, goText);

  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(hideKeyboard()));
  QObject::connect(ctl, SIGNAL(commitRequested()), this, SLOT(commit()));

  return keyboard;
}
