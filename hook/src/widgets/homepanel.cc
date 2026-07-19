#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QScreen>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../nickelstoregraph.h"
#include "homepanel.h"

HomePanel::HomePanel(QString title) : QFrame() {
  // Parent to the application's main window so we cover the content area
  // AND the nav bars — same technique as showIcon() in cli.cc.
  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv               = MainWindowController__currentView(mwc);
  QWidget *mainWindow       = cv ? cv->window() : nullptr;

  if (mainWindow) {
    setParent(mainWindow);
    setFixedSize(mainWindow->size());
  } else {
    QScreen *screen = QApplication::primaryScreen();
    setFixedSize(screen->geometry().size());
  }

  move(0, 0);
  setObjectName("HomePanel");

  setStyleSheet(R"(
    HomePanel {
      background-color: white;
    }
    QWidget#panelHeader {
      background-color: #eeeeee;
      border-bottom: 1px solid #cccccc;
    }
    [qApp_deviceIsTrilogy=true]  QWidget#panelHeader { min-height: 52px;  max-height: 52px;  }
    [qApp_deviceIsPhoenix=true]  QWidget#panelHeader { min-height: 65px;  max-height: 65px;  }
    [qApp_deviceIsDragon=true]   QWidget#panelHeader { min-height: 88px;  max-height: 88px;  }
    [qApp_deviceIsStorm=true]    QWidget#panelHeader { min-height: 100px; max-height: 100px; }
    [qApp_deviceIsDaylight=true] QWidget#panelHeader { min-height: 113px; max-height: 113px; }

    QLabel#panelTitle {
      font-weight: bold;
    }
    [qApp_deviceIsTrilogy=true]  QLabel#panelTitle { font-size: 16px; padding: 0 12px; }
    [qApp_deviceIsPhoenix=true]  QLabel#panelTitle { font-size: 19px; padding: 0 16px; }
    [qApp_deviceIsDragon=true]   QLabel#panelTitle { font-size: 24px; padding: 0 22px; }
    [qApp_deviceIsStorm=true]    QLabel#panelTitle { font-size: 27px; padding: 0 25px; }
    [qApp_deviceIsDaylight=true] QLabel#panelTitle { font-size: 30px; padding: 0 28px; }
  )");

  QVBoxLayout *outerLayout = new QVBoxLayout(this);
  outerLayout->setContentsMargins(0, 0, 0, 0);
  outerLayout->setSpacing(0);

  // Header: title + close button
  QWidget *header = new QWidget(this);
  header->setObjectName("panelHeader");
  QHBoxLayout *headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(0, 0, 8, 0);
  headerLayout->setSpacing(0);

  QLabel *titleLabel = new QLabel(title, header);
  titleLabel->setObjectName("panelTitle");
  titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  headerLayout->addWidget(titleLabel, 1);

  N3ButtonLabel *closeBtn = construct_N3ButtonLabel(header);
  closeBtn->setText("X");
  QObject::connect(closeBtn, SIGNAL(tapped(bool)), this, SLOT(closePanel()));
  headerLayout->addWidget(closeBtn, 0);

  outerLayout->addWidget(header);

  // Content area — subclasses add their widgets here
  QWidget *contentWidget = new QWidget(this);
  contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(16, 12, 16, 12);
  contentLayout->setSpacing(0);
  outerLayout->addWidget(contentWidget, 1);

  raise();
  show();
}

void HomePanel::closePanel() {
  deleteLater();
}
