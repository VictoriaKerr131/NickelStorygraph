#pragma once

#include <QJsonObject>

#include "../widgets/dialog.h"
#include "../widgets/pagedstack.h"
#include "checkboxrow.h"
#include "staticrow.h"

class SettingsDialog : public Dialog {
  Q_OBJECT

public:
  static void show(bool fromHomeMenu = false);

public Q_SLOTS:
  void buildPages();

  void setAutoSyncDefault(bool value);
  void setSyncBookmarks(QVariant value);
  void setSyncDaily(QVariant value);
  void setCloseThreshold(QVariant value);
  void setPageThreshold(QVariant value);

  void setSimpleReview(bool value);
  void setDebug(bool value);
  void saveLogs();

  void setHomeMenuEnabled(bool value);
  void setHomeMenuReading(bool value);
  void setHomeMenuFeed(bool value);
  void setHomeMenuGoals(bool value);

  void clearLastSynced();

  void setUsername(QJsonObject doc);

private:
  SettingsDialog(bool fromHomeMenu);

  // True when Settings was opened from the home screen menu rather than from
  // within a book - there's no contentId in that context, so the
  // "Book information" section is hidden entirely and the Auto-Sync section
  // edits the global defaults instead of a per-book override.
  bool fromHomeMenu = false;
  PagedStack *pages = nullptr;
  StaticRow *username = nullptr;
  CheckboxRow *homeMenuReadingRow = nullptr;
  CheckboxRow *homeMenuFeedRow = nullptr;
  CheckboxRow *homeMenuGoalsRow = nullptr;

  QFrame *buildGeneral();
  QFrame *buildAutoSync();
  QFrame *buildHomeMenu();
  QFrame *buildInformation();
  QFrame *buildAdvanced();
};
