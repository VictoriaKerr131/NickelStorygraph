#pragma once

#include <QFrame>
#include <QVBoxLayout>

// Full-screen overlay widget that parents itself to the Kobo main window,
// covering everything (content area AND nav bars) like NickelMenu's Sudoku.
// Subclasses add their widgets to contentLayout.
class HomePanel : public QFrame {
  Q_OBJECT

public:
  HomePanel(QString title);

protected:
  QVBoxLayout *contentLayout;

private Q_SLOTS:
  void closePanel();
};
