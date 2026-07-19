#include <dlfcn.h>

#include <QApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QWidgetAction>

#include <NickelHook.h>

#include "files.h"
#include "menucontroller.h"
#include "feed/feeddialog.h"
#include "goals/goalsdialog.h"
#include "reading/readingdialog.h"
#include "settings.h"
#include "settings/settingsdialog.h"
#include "synccontroller.h"

typedef void ReadingController;
typedef void Volume;
typedef void Bookmark;
static void (*ReadingController__setVolume)(ReadingController *_this, Volume *volume, Bookmark *bookmark);
static QString (*Content__getId)(Volume *_this);
static QString (*Content__getTitle)(Volume *_this);
static QString (*Content__getAttribution)(Volume *_this);

typedef QList<QPair<QString, QString>> SupportedLocales;
static SupportedLocales *(*SupportedLocales__supportedLocales)(SupportedLocales *_this, bool b1);

MainWindowController *(*MainWindowController__sharedInstance)();
QWidget *(*MainWindowController__currentView)(MainWindowController *mwc);
QWidget *(*MainWindowController__pushView)(MainWindowController *mwc, QWidget *view);
WirelessManager *(*WirelessManager__sharedInstance)();

void (*ConfirmationDialogFactory__showErrorDialog)(QString const &title, QString const &content);
ConfirmationDialog *(*ConfirmationDialogFactory__getConfirmationDialog)(QWidget *parent);
void (*ConfirmationDialog__setTitle)(ConfirmationDialog *_this, QString const &);
void (*ConfirmationDialog__setText)(ConfirmationDialog *_this, QString const &);
void (*ConfirmationDialog__setAcceptButtonText)(ConfirmationDialog *_this, QString const &);
void (*ConfirmationDialog__setRejectButtonText)(ConfirmationDialog *_this, QString const &);
void (*ConfirmationDialog__showCloseButton)(ConfirmationDialog *_this, bool enabled);

int (*ReadingView__getCalculatedReadProgress)(QWidget *_this);

WirelessWorkflowManager *(*WirelessWorkflowManager__sharedInstance)();
void (*WirelessWorkflowManager__connectWirelessSilently)(WirelessWorkflowManager *_this);
void (*WirelessWorkflowManager__connectWireless)(WirelessWorkflowManager *_this, bool, bool);
bool (*WirelessWorkflowManager__isInternetAccessible)(WirelessWorkflowManager *_this);

void (*TouchLabel__constructor)(TouchLabel *_this, QWidget *parent, QFlags<Qt::WindowType>);
void (*TouchLabel__setHitStateEnabled)(TouchLabel *_this, bool enabled);

TouchLabel *construct_TouchLabel(QWidget *parent) {
  TouchLabel *label = reinterpret_cast<TouchLabel *>(calloc(1, 2048));
  TouchLabel__constructor(label, parent, 0);
  return label;
}

void (*N3ButtonLabel__constructor)(N3ButtonLabel *_this, QWidget *parent);

N3ButtonLabel *construct_N3ButtonLabel(QWidget *parent) {
  N3ButtonLabel *button = reinterpret_cast<N3ButtonLabel *>(calloc(1, 2048));
  N3ButtonLabel__constructor(button, parent);
  return button;
}

void (*TouchCheckBox__constructor)(TouchCheckBox *_this, QWidget *parent);

TouchCheckBox *construct_TouchCheckBox(QWidget *parent) {
  TouchCheckBox *checkbox = reinterpret_cast<TouchCheckBox *>(calloc(1, 512));
  TouchCheckBox__constructor(checkbox, parent);
  return checkbox;
}

void (*NickelTouchMenu__constructor)(NickelTouchMenu *_this, QWidget *parent, int pos);
void (*NickelTouchMenu__showDecoration)(NickelTouchMenu *_this, bool show);

NickelTouchMenu *construct_NickelTouchMenu(QWidget *parent, int pos) {
  NickelTouchMenu *menu = reinterpret_cast<NickelTouchMenu *>(calloc(1, 2048));
  NickelTouchMenu__constructor(menu, parent, pos);
  return menu;
}

void (*MenuTextItem__constructor)(MenuTextItem *_this, QWidget *parent, bool checkable, bool italic);
void (*MenuTextItem__setText)(MenuTextItem *_this, QString const &text);
void (*MenuTextItem__setSelected)(MenuTextItem *_this, bool selected);
void (*MenuTextItem__registerForTapGestures)(MenuTextItem *_this);

MenuTextItem *construct_MenuTextItem(QWidget *parent, bool checkable, bool italic) {
  MenuTextItem *item = reinterpret_cast<MenuTextItem *>(calloc(1, 1024));
  MenuTextItem__constructor(item, parent, checkable, italic);
  return item;
}

typedef QWidget ReadingMenuView;
void (*ReadingMenuView__constructor)(ReadingMenuView *_this, QWidget *parent, bool unknown);
void (*ReadingMenuView__constructor_2)(ReadingMenuView *_this, QWidget *, QByteArray const &unknownArray,
                                       bool unknownBool);

typedef QWidget MainNavView;
typedef QWidget MainNavButton;
static void (*MainNavView__constructor)(MainNavView *_this, QWidget *parent);
static void (*MainNavButton__constructor)(MainNavButton *_this, QWidget *parent);
static void (*MainNavButton__setPixmap)(MainNavButton *_this, QString const &pixmapName);
static void (*MainNavButton__setActivePixmap)(MainNavButton *_this, QString const &pixmapName);
static void (*MainNavButton__setText)(MainNavButton *_this, QString const &text);

N3Dialog *(*N3DialogFactory__getDialog)(QWidget *content, bool unknown);
void (*N3Dialog__disableCloseButton)(N3Dialog *__this);
void (*N3Dialog__enableBackButton)(N3Dialog *__this, bool enable);
void (*N3Dialog__setTitle)(N3Dialog *__this, QString const &);
KeyboardFrame *(*N3Dialog__keyboardFrame)(N3Dialog *__this);
void (*N3Dialog__enableFullViewMode)(N3Dialog *__this);
void (*N3Dialog__showKeyboard)(N3Dialog *__this);
void (*N3Dialog__hideKeyboard)(N3Dialog *__this);

void (*KeyboardReceiver__constructor)(KeyboardReceiver *__this, QLineEdit *lineEdit, bool unknown);
void (*KeyboardReceiver__TextEdit_constructor)(KeyboardReceiver *__this, QTextEdit *parent, bool unknown);

KeyboardReceiver *construct_KeyboardReceiver(QLineEdit *lineEdit) {
  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 512));
  KeyboardReceiver__constructor(receiver, lineEdit, false);
  return receiver;
}

KeyboardReceiver *construct_KeyboardReceiver(QTextEdit *textEdit) {
  KeyboardReceiver *receiver = reinterpret_cast<KeyboardReceiver *>(calloc(1, 512));
  KeyboardReceiver__TextEdit_constructor(receiver, textEdit, false);
  return receiver;
}

SearchKeyboardController *(*KeyboardFrame__createKeyboard)(KeyboardFrame *__this, int keyboardScript, QLocale locale);
void (*SearchKeyboardController__setReceiver)(SearchKeyboardController *__this, KeyboardReceiver *receiver, bool idk);
void (*SearchKeyboardController__setGoText)(SearchKeyboardController *__this, QString const &text);

void (*TouchLineEdit__constructor)(TouchLineEdit *__this, QWidget *parent);

TouchLineEdit *construct_TouchLineEdit(QWidget *parent) {
  TouchLineEdit *lineEdit = reinterpret_cast<TouchLineEdit *>(calloc(1, 512));
  TouchLineEdit__constructor(lineEdit, parent);
  return lineEdit;
}

void (*SettingContainer__constructor)(SettingContainer *__this, QWidget *parent);

SettingContainer *construct_SettingContainer(QWidget *parent) {
  SettingContainer *container = reinterpret_cast<SettingContainer *>(calloc(1, 512));
  SettingContainer__constructor(container, parent);
  return container;
}

void (*TouchTextEdit__constructor)(TouchTextEdit *__this, QWidget *parent);
void (*TouchTextEdit__setCustomPlaceholderText)(TouchTextEdit *__this, QString const &text);

TouchTextEdit *construct_TouchTextEdit(QWidget *parent) {
  TouchTextEdit *touchText = reinterpret_cast<TouchTextEdit *>(calloc(1, 512));
  TouchTextEdit__constructor(touchText, parent);
  return touchText;
}

void (*PowerTimer__constructor)(PowerTimer *__this, QString const &name, QObject *parent);
void (*PowerTimer__fireAt)(PowerTimer *__this, QDateTime const &datatime);
int (*PowerTimer__timeRemaining)(PowerTimer *__this);

PowerTimer *construct_PowerTimer(QString const &name, QObject *parent) {
  PowerTimer *timer = reinterpret_cast<PowerTimer *>(calloc(1, 512));
  PowerTimer__constructor(timer, name, parent);
  return timer;
}

static struct nh_info NickelStorygraph = (struct nh_info){.name = "NickelStorygraph",
                                                         .desc = "Updates reading progress on StoryGraph",
                                                         .uninstall_flag = "/mnt/onboard/NickelStorygraph_uninstall",
                                                         .uninstall_xflag = "/mnt/onboard/.adds/NickelStorygraph",
                                                         .failsafe_delay = 30};

// clang-format off
static struct nh_hook NickelStorygraphHook[] = {
  { .sym = "_ZN17ReadingController9setVolumeERK6VolumeRK8Bookmark", .sym_new = "_nh_ReadingController__setVolume",       .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(ReadingController__setVolume),       .desc = "The main entry point" },
  { .sym = "_ZN15ReadingMenuViewC1EP7QWidgetb",                     .sym_new = "_nh_ReadingMenuView__constructor",       .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(ReadingMenuView__constructor),       .desc = "Used to inject menu items", .optional = true },
  { .sym = "_ZN15ReadingMenuViewC1EP7QWidgetRK10QByteArrayb",       .sym_new = "_nh_ReadingMenuView__constructor_2",     .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(ReadingMenuView__constructor_2),     .desc = "Used to inject menu items", .optional = true }, // Version 4.44+
  { .sym = "_ZN16SupportedLocales16supportedLocalesEb",             .sym_new = "_nh_SupportedLocales__supportedLocales", .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(SupportedLocales__supportedLocales), .desc = "Add support for 'English (UK)'", .optional = true },
  { .sym = "_ZN11MainNavViewC1EP7QWidget",                         .sym_new = "_nh_MainNavView__constructor",          .lib = "libnickel.so.1.0.0", .out = nh_symoutptr(MainNavView__constructor),          .desc = "Home screen StoryGraph menu button", .optional = true },
  {0},
};

static struct nh_dlsym NickelStorygraphDlsym[] = {
  { .name = "_ZNK7Content5getIdEv",                                            .out = nh_symoutptr(Content__getId) },
  { .name = "_ZNK7Content8getTitleEv",                                         .out = nh_symoutptr(Content__getTitle) },
  { .name = "_ZNK7Content14getAttributionEv",                                  .out = nh_symoutptr(Content__getAttribution) },

  { .name = "_ZN20MainWindowController14sharedInstanceEv",                     .out = nh_symoutptr(MainWindowController__sharedInstance) },
  { .name = "_ZNK20MainWindowController11currentViewEv",                       .out = nh_symoutptr(MainWindowController__currentView) },
  { .name = "_ZN20MainWindowController8pushViewEP7QWidget",                    .out = nh_symoutptr(MainWindowController__pushView) },
  { .name = "_ZN11ReadingView25getCalculatedReadProgressEv",                   .out = nh_symoutptr(ReadingView__getCalculatedReadProgress) },

  { .name = "_ZN25ConfirmationDialogFactory15showErrorDialogERK7QStringS2_",   .out = nh_symoutptr(ConfirmationDialogFactory__showErrorDialog) },
  { .name = "_ZN25ConfirmationDialogFactory21getConfirmationDialogEP7QWidget", .out = nh_symoutptr(ConfirmationDialogFactory__getConfirmationDialog) },
  { .name = "_ZN18ConfirmationDialog8setTitleERK7QString",                     .out = nh_symoutptr(ConfirmationDialog__setTitle) },
  { .name = "_ZN18ConfirmationDialog7setTextERK7QString",                      .out = nh_symoutptr(ConfirmationDialog__setText) },
  { .name = "_ZN18ConfirmationDialog19setAcceptButtonTextERK7QString",         .out = nh_symoutptr(ConfirmationDialog__setAcceptButtonText) },
  { .name = "_ZN18ConfirmationDialog19setRejectButtonTextERK7QString",         .out = nh_symoutptr(ConfirmationDialog__setRejectButtonText) },
  { .name = "_ZN18ConfirmationDialog15showCloseButtonEb",                      .out = nh_symoutptr(ConfirmationDialog__showCloseButton) },

  { .name = "_ZN23WirelessWorkflowManager14sharedInstanceEv",                  .out = nh_symoutptr(WirelessWorkflowManager__sharedInstance) },
  { .name = "_ZN23WirelessWorkflowManager20isInternetAccessibleEv",            .out = nh_symoutptr(WirelessWorkflowManager__isInternetAccessible) },
  { .name = "_ZN23WirelessWorkflowManager15connectWirelessEbb",                .out = nh_symoutptr(WirelessWorkflowManager__connectWireless) },
  { .name = "_ZN23WirelessWorkflowManager23connectWirelessSilentlyEv",         .out = nh_symoutptr(WirelessWorkflowManager__connectWirelessSilently) },
  { .name = "_ZN15WirelessManager14sharedInstanceEv",                          .out = nh_symoutptr(WirelessManager__sharedInstance) },

  { .name = "_ZN10TouchLabelC1EP7QWidget6QFlagsIN2Qt10WindowTypeEE",           .out = nh_symoutptr(TouchLabel__constructor) },
  { .name = "_ZN10TouchLabel18setHitStateEnabledEb",                           .out = nh_symoutptr(TouchLabel__setHitStateEnabled) },

  { .name = "_ZN13N3ButtonLabelC1EP7QWidget",                                  .out = nh_symoutptr(N3ButtonLabel__constructor) },

  { .name = "_ZN13TouchCheckBoxC1EP7QWidget",                                  .out = nh_symoutptr(TouchCheckBox__constructor) },

  { .name = "_ZN15NickelTouchMenuC1EP7QWidget18DecorationPosition",            .out = nh_symoutptr(NickelTouchMenu__constructor) },
  { .name = "_ZN15NickelTouchMenu14showDecorationEb",                          .out = nh_symoutptr(NickelTouchMenu__showDecoration) },
  { .name = "_ZN12MenuTextItemC1EP7QWidgetbb",                                 .out = nh_symoutptr(MenuTextItem__constructor) },
  { .name = "_ZN12MenuTextItem7setTextERK7QString",                            .out = nh_symoutptr(MenuTextItem__setText) },
  { .name = "_ZN12MenuTextItem11setSelectedEb",                                .out = nh_symoutptr(MenuTextItem__setSelected) },
  { .name = "_ZN12MenuTextItem22registerForTapGesturesEv",                     .out = nh_symoutptr(MenuTextItem__registerForTapGestures) },

  { .name = "_ZN15N3DialogFactory9getDialogEP7QWidgetb",                       .out = nh_symoutptr(N3DialogFactory__getDialog) },
  { .name = "_ZN8N3Dialog18disableCloseButtonEv",                              .out = nh_symoutptr(N3Dialog__disableCloseButton) },
  { .name = "_ZN8N3Dialog16enableBackButtonEb",                                .out = nh_symoutptr(N3Dialog__enableBackButton) },
  { .name = "_ZN8N3Dialog8setTitleERK7QString",                                .out = nh_symoutptr(N3Dialog__setTitle) },
  { .name = "_ZN8N3Dialog13keyboardFrameEv",                                   .out = nh_symoutptr(N3Dialog__keyboardFrame) },
  { .name = "_ZN8N3Dialog18enableFullViewModeEv",                              .out = nh_symoutptr(N3Dialog__enableFullViewMode) },
  { .name = "_ZN8N3Dialog12hideKeyboardEv",                                    .out = nh_symoutptr(N3Dialog__hideKeyboard) },
  { .name = "_ZN8N3Dialog12showKeyboardEv",                                    .out = nh_symoutptr(N3Dialog__showKeyboard) },

  { .name = "_ZN13TouchLineEditC1EP7QWidget",                                  .out = nh_symoutptr(TouchLineEdit__constructor) },
  { .name = "_ZN16KeyboardReceiverC1EP9QLineEditb",                            .out = nh_symoutptr(KeyboardReceiver__constructor) },
  { .name = "_ZN16KeyboardReceiverC1EP9QTextEditb",                            .out = nh_symoutptr(KeyboardReceiver__TextEdit_constructor) },
  { .name = "_ZN13KeyboardFrame14createKeyboardE14KeyboardScriptRK7QLocale",   .out = nh_symoutptr(KeyboardFrame__createKeyboard) },
  { .name = "_ZN24SearchKeyboardController11setReceiverEP16KeyboardReceiverb", .out = nh_symoutptr(SearchKeyboardController__setReceiver) },
  { .name = "_ZN24SearchKeyboardController9setGoTextERK7QString",              .out = nh_symoutptr(SearchKeyboardController__setGoText) },

  { .name = "_ZN16SettingContainerC1EP7QWidget",                               .out = nh_symoutptr(SettingContainer__constructor) },

  { .name = "_ZN13TouchTextEditC1EP7QWidget",                                  .out = nh_symoutptr(TouchTextEdit__constructor) },
  { .name = "_ZN13TouchTextEdit24setCustomPlaceholderTextERK7QString",         .out = nh_symoutptr(TouchTextEdit__setCustomPlaceholderText) },

  { .name = "_ZN10PowerTimerC1ERK7QStringP7QObject",                           .out = nh_symoutptr(PowerTimer__constructor) },
  { .name = "_ZN10PowerTimer6fireAtERK9QDateTime",                             .out = nh_symoutptr(PowerTimer__fireAt) },
  { .name = "_ZNK10PowerTimer13timeRemainingEv",                               .out = nh_symoutptr(PowerTimer__timeRemaining) },

  { .name = "_ZN13MainNavButtonC1EP7QWidget",                .out = nh_symoutptr(MainNavButton__constructor),     .desc = "Home screen StoryGraph menu button", .optional = true },
  { .name = "_ZN13MainNavButton9setPixmapERK7QString",        .out = nh_symoutptr(MainNavButton__setPixmap),       .desc = "Home screen StoryGraph menu button", .optional = true },
  { .name = "_ZN13MainNavButton15setActivePixmapERK7QString", .out = nh_symoutptr(MainNavButton__setActivePixmap), .desc = "Home screen StoryGraph menu button", .optional = true },
  { .name = "_ZN13MainNavButton7setTextERK7QString",          .out = nh_symoutptr(MainNavButton__setText),         .desc = "Home screen StoryGraph menu button", .optional = true },

  {0},
};
// clang-format on

bool hardcover_uninstall() {
  nh_delete_file(Files::config);
  nh_delete_file(Files::settings);
  nh_delete_file(Files::cli);
  nh_delete_dir(Files::adds_directory);
  nh_delete_file(Files::nickelmenu_config_entry);

  return true;
}

// --- NickelMenu coexistence ---
//
// NickelMenu hooks the same MainNavView constructor we do to add its own tab
// bar button. NickelHook's GOT-patching has no chaining: whichever mod
// installs its hook last wins the slot outright and the other's button
// silently never appears. We detect NickelMenu (its hook symbol is visible
// process-wide once its plugin is loaded) and, when present, call its hook
// function directly instead of the true original - this runs its real
// button-adding logic (which itself calls through to the true original) and
// leaves us free to add our own button afterward, since both just land as
// separate widgets in the same tab bar layout.

static bool nickelMenuDetected = false;

// NickelMenu hooks the exact same MainNavView constructor symbol we do (its
// own tab bar button injection, "_nm_menu_hook2" in NickelMenu's source).
// NickelHook's GOT-patching has no chaining - only the mod whose library
// loads (and thus hooks) LAST is ever actually invoked by libnickel; the
// other mod's hook function is never called at all. Whenever nickelMenuDetected
// is true, we are deterministically the one libnickel calls (we load after
// NickelMenu alphabetically), and NickelMenu's own hook (and therefore its
// own button) would otherwise never run. Calling it ourselves - it calls
// through to the true original constructor itself before adding its button
// - restores it.
//
// Resolving "_nm_menu_hook2" isn't as simple as dlsym(RTLD_DEFAULT, ...):
// Nickel loads these .so files as generic Qt image-format plugins, and Qt's
// plugin loader dlopen()s them with the default RTLD_LOCAL (it only asks
// for RTLD_GLOBAL when QLibrary::ExportExternalSymbolsHint is set, which
// Nickel's plugin scan doesn't do). RTLD_LOCAL means NickelMenu's exported
// symbols are invisible to a dlsym(RTLD_DEFAULT, ...) call made from a
// different shared object, even once it's fully loaded - so that lookup
// always silently fails. Instead we reopen NickelMenu's library by its
// exact install path with RTLD_NOLOAD (glibc matches already-loaded objects
// by name before touching the filesystem, so this works even if NickelHook's
// own crash-failsafe has the file transiently renamed away) and resolve the
// symbol against that handle directly, which searches its symbol table
// regardless of RTLD_LOCAL.
typedef void (*NmMenuHook2)(QWidget *, QWidget *);
static NmMenuHook2 nmMenuHook2 = nullptr;

// The home screen tab bar button, cached so toggling "Show StoryGraph button
// on home screen" in Settings can show/hide it immediately rather than
// requiring Nickel to restart and reconstruct MainNavView.
static QPointer<QWidget> sgButtonWidget;

void setHomeButtonVisible(bool visible) {
  if (sgButtonWidget)
    sgButtonWidget->setVisible(visible);
}

// scaledNavIconPath sizes our tab bar icon to match Nickel's own native tab
// icons at runtime, instead of guessing per device tier from fixed PNGs.
// ":/images/home/main_nav_more.png" is Nickel's own built-in "more" tab
// icon - not a resource we own, but Qt's resource registry is global to the
// process once a .qrc is loaded, so it's visible from our .so too (same
// trick NickelMenu's nm_menu_pixmap() uses to size custom menu icons: see
// nickelmenu.cc's own reference to that exact path). Scaling our high-res
// master against its real size means we automatically match whatever the
// native tab icons actually look like on this device/firmware, rather than
// needing on-device measurements per tier.
//
// Returns an absolute file path MainNavButton__setPixmap can load, or an
// empty string on any failure (missing resource, write failure, etc.) - the
// caller falls back to the static nav_icon/nav_icon_hit assets in that case.
QString scaledNavIconPath() {
  QImage master;
  if (!master.load(Files::icon_master_outline)) {
    nh_log("scaledNavIconPath: could not load master icon");
    return QString();
  }

  QPixmap reference;
  if (!reference.load(QStringLiteral(":/images/home/main_nav_more.png"))) {
    nh_log("scaledNavIconPath: could not load native reference icon");
    return QString();
  }

  QImage scaled = master.scaled(reference.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
  nh_log("scaledNavIconPath: scaled master to match native icon (%dx%d): %dx%d", reference.width(),
         reference.height(), scaled.width(), scaled.height());

  QString tempOut = QStringLiteral("/tmp/nickelstoregraph_nav_icon.png");
  if (!scaled.save(tempOut, "PNG")) {
    nh_log("scaledNavIconPath: could not save scaled icon to %s", qPrintable(tempOut));
    return QString();
  }

  return tempOut;
}

void showStorygraphHomeMenu(QWidget *anchor) {
  Settings *settings = Settings::getInstance();
  if (!settings->getHomeMenuEnabled()) return;

  QList<Item> items;
  if (settings->getHomeMenuReading())
    items.append({"Currently Reading", 1});
  if (settings->getHomeMenuFeed())
    items.append({"Community Feed", 2});
  if (settings->getHomeMenuGoals())
    items.append({"Reading Goals", 3});
  items.append({"Settings", 4});

  if (!anchor) return;

  // pos=3 + decoration off matches how Nickel/NickelMenu construct the home
  // screen tab bar's own popup: a plain rectangle, not the pointer/"speech
  // bubble" triangle used for popups anchored to a small toolbar icon.
  NickelTouchMenu *menu = construct_NickelTouchMenu(anchor, 3);
  NickelTouchMenu__showDecoration(menu, false);
  QWidget::connect(menu, &QMenu::aboutToHide, menu, &QWidget::deleteLater);

  for (const Item &item : items) {
    int val = item.value.toInt();

    MenuTextItem *mti = construct_MenuTextItem(menu, false, true);
    MenuTextItem__setText(mti, item.text);
    MenuTextItem__registerForTapGestures(mti);

    QWidgetAction *action = new QWidgetAction(menu);
    action->setDefaultWidget(mti);
    action->setEnabled(true);
    action->setData(item.value);
    menu->addAction(action);
    menu->addSeparator();

    QObject::connect(action, &QAction::triggered, [menu, val] {
      menu->hide();
      if (val == 1) ReadingDialog::show();
      else if (val == 2) FeedDialog::show();
      else if (val == 3) GoalsDialog::show();
      // No book is open on the home screen, so there's no contentId to show
      // sync/progress info for - suppress the "Book information" section.
      else if (val == 4) SettingsDialog::show(true);
    });
    QObject::connect(mti, SIGNAL(tapped(bool)), action, SIGNAL(triggered()));
  }

  menu->ensurePolished();
  // Popup sits entirely above the anchor's top edge (negative Y), matching
  // NickelMenu's own tab bar popup — bottom-aligning against the anchor
  // (the old formula) covers the bottom nav bar whenever the anchor is the
  // full window rather than a small toolbar icon.
  QPoint pos = anchor->mapToGlobal(QPoint((anchor->width() - menu->sizeHint().width()) / 2, -menu->sizeHint().height()));
  menu->popup(pos);
}

int nickelstoregraph_init() {
  void *nmHandle = dlopen("/usr/local/Kobo/imageformats/libnm.so", RTLD_NOLOAD | RTLD_LAZY);
  if (nmHandle) {
    nmMenuHook2 = reinterpret_cast<NmMenuHook2>(dlsym(nmHandle, "_nm_menu_hook2"));
    dlclose(nmHandle);
  }
  nickelMenuDetected = nmMenuHook2 != nullptr;
  nh_log("nickelstoregraph_init: nickelMenuDetected=%s", nickelMenuDetected ? "true" : "false");

  return 0;
}

NickelHook(.init = &nickelstoregraph_init, .info = &NickelStorygraph, .hook = NickelStorygraphHook,
           .dlsym = NickelStorygraphDlsym, .uninstall = &hardcover_uninstall);

QStackedWidget *stackedWidget = nullptr;

void handleStackedWidgetDestroyed() { stackedWidget = nullptr; }

extern "C" __attribute__((visibility("default"))) void
_nh_ReadingController__setVolume(ReadingController *_this, Volume *volume, Bookmark *bookmark) {
  nh_log("ReadingController::setVolume(%p, %p, %p)", _this, volume, bookmark);

  SyncController *syncController = SyncController::getInstance();
  syncController->contentId = Content__getId(volume);
  syncController->title = Content__getTitle(volume);
  syncController->author = Content__getAttribution(volume);

  nh_log("ReadingController::setVolume contentId=%s", qPrintable(syncController->contentId));

  MainWindowController *mwc = MainWindowController__sharedInstance();
  QWidget *cv = MainWindowController__currentView(mwc);

  if (!stackedWidget) {
    QWidget *parent = cv ? cv->parentWidget() : nullptr;
    if (parent && QString(parent->metaObject()->className()) == "QStackedWidget") {
      stackedWidget = static_cast<QStackedWidget *>(parent);
      nh_log("ReadingController::setVolume connecting stackedWidget=%p", stackedWidget);
      QObject::connect(stackedWidget, &QStackedWidget::currentChanged, SyncController::getInstance(),
                       &SyncController::currentViewIndexChanged);
      QObject::connect(stackedWidget, &QObject::destroyed, handleStackedWidgetDestroyed);
    } else {
      nh_log("Error: expected QStackedWidget, got %s", parent ? parent->metaObject()->className() : "null");
    }
  }

  nh_log("ReadingController::setVolume calling original");
  ReadingController__setVolume(_this, volume, bookmark);
  nh_log("ReadingController::setVolume original returned");
}

void injectMenuWidget(ReadingMenuView *parent) {
  QHBoxLayout *childLayout = parent->findChild<QHBoxLayout *>("bottomHorizontalLayout");
  QLabel *settingsIcon = parent->findChild<QLabel *>("settingsIcon");

  nh_log("injectMenuWidget: childLayout=%p settingsIcon=%p", childLayout, settingsIcon);

  if (childLayout && settingsIcon) {
    // The bare settings icon height reads oversized next to the other toolbar icons.
    int h = static_cast<int>(settingsIcon->height() * 0.65);
    nh_log("injectMenuWidget: creating MenuController height=%d", h);
    MenuController *ctl = new MenuController(h, parent);
    nh_log("injectMenuWidget: inserting icon");
    childLayout->insertWidget(childLayout->count() - 1, ctl->icon);
    nh_log("injectMenuWidget: done");
  } else {
    nh_log("Error: unable to find bottomHorizontalLayout and settingsIcon");
  }
}

extern "C" __attribute__((visibility("default"))) void _nh_ReadingMenuView__constructor(ReadingMenuView *_this,
                                                                                        QWidget *parent, bool unknown) {
  nh_log("ReadingMenuView::ReadingMenuView(%p, %p, %s)", _this, parent, unknown ? "true" : "false");
  ReadingMenuView__constructor(_this, parent, unknown);
  nh_log("ReadingMenuView::ReadingMenuView original returned, injecting");
  injectMenuWidget(parent);
  nh_log("ReadingMenuView::ReadingMenuView inject done");
}

extern "C" __attribute__((visibility("default"))) void
_nh_ReadingMenuView__constructor_2(ReadingMenuView *_this, QWidget *parent, QByteArray const &unknownArray,
                                   bool unknownBool) {
  nh_log("ReadingMenuView::ReadingMenuView(%p, %p, %s)", _this, parent, unknownBool ? "true" : "false");
  ReadingMenuView__constructor_2(_this, parent, unknownArray, unknownBool);
  injectMenuWidget(parent);
}

extern "C" __attribute__((visibility("default"))) void
_nh_MainNavView__constructor(MainNavView *_this, QWidget *parent) {
  nh_log("MainNavView::MainNavView(%p, %p) nickelMenuDetected=%s nmMenuHook2=%p", _this, parent,
         nickelMenuDetected ? "true" : "false", (void *)nmMenuHook2);

  if (nmMenuHook2) {
    // We're guaranteed to be the hook libnickel actually calls whenever
    // NickelMenu is detected (see nmMenuHook2 comment above) - call its own
    // hook instead of the true original so its own tab bar button still
    // gets added; it calls through to the true original itself.
    nmMenuHook2(_this, parent);
  } else {
    MainNavView__constructor(_this, parent);
  }

  // Adds our own tab bar button even when NickelMenu is present - nmMenuHook2
  // above already ran NickelMenu's real button-adding logic against this same
  // MainNavView/QHBoxLayout, so this is just a second widget landing in the
  // same layout, not a rematch over the hook slot.
  if (!MainNavButton__constructor || !MainNavButton__setPixmap || !MainNavButton__setActivePixmap || !MainNavButton__setText) {
    nh_log("_nh_MainNavView__constructor: missing MainNavButton symbols, skipping");
    return;
  }

  QHBoxLayout *bl = _this->findChild<QHBoxLayout *>();
  if (!bl) {
    nh_log("_nh_MainNavView__constructor: could not find QHBoxLayout in MainNavView");
    return;
  }

  MainNavButton *btn = reinterpret_cast<MainNavButton *>(calloc(1, 256));
  if (!btn) return;

  MainNavButton__constructor(btn, parent);
  QString iconPath = scaledNavIconPath();
  MainNavButton__setPixmap(btn, iconPath.isEmpty() ? QString(Files::nav_icon) : iconPath);
  MainNavButton__setActivePixmap(btn, iconPath.isEmpty() ? QString(Files::nav_icon_hit) : iconPath);
  MainNavButton__setText(btn, "StoryGraph");
  btn->setObjectName("sgButton");

  // Adding a fifth/sixth button to the tab bar makes Nickel's own layout
  // shrink every button's label to keep them all fit, but "StoryGraph" is
  // still noticeably larger than the native labels next to it - find the
  // label the text landed on (by content, since we don't know its internal
  // object name) and knock it down a bit further to match.
  bool foundLabel = false;
  for (QLabel *label : btn->findChildren<QLabel *>()) {
    if (label->text() != "StoryGraph") continue;
    foundLabel = true;
    QFont f = label->font();
    if (f.pixelSize() > 0)
      f.setPixelSize(static_cast<int>(f.pixelSize() * 0.85));
    else if (f.pointSizeF() > 0)
      f.setPointSizeF(f.pointSizeF() * 0.85);
    label->setFont(f);
    break;
  }
  if (!foundLabel)
    nh_log("_nh_MainNavView__constructor: could not find StoryGraph label to shrink font");

  QPushButton *sh = new QPushButton(_this);
  sh->setVisible(false);

  if (!QWidget::connect(btn, SIGNAL(tapped()), sh, SIGNAL(pressed()))) {
    nh_log("_nh_MainNavView__constructor: could not connect tapped() signal");
    return;
  }

  QWidget::connect(sh, &QPushButton::pressed, [btn] { showStorygraphHomeMenu(btn); });

  // Built unconditionally (even if currently disabled) so setHomeButtonVisible()
  // can show/hide it live as the setting is toggled from Settings, without
  // needing Nickel to restart and reconstruct MainNavView.
  sgButtonWidget = btn;
  btn->setVisible(Settings::getInstance()->getHomeMenuEnabled());

  bl->addWidget(btn, 1);
  _this->ensurePolished();
}

extern "C" __attribute__((visibility("default"))) SupportedLocales *
_nh_SupportedLocales__supportedLocales(SupportedLocales *_this, bool b1) {
  SupportedLocales *items = SupportedLocales__supportedLocales(_this, b1);

  int index = 0;
  for (; index < items->count(); index++) {
    if (items->at(index).second == "en")
      break;
  }

  items->insert(index + 1, QPair<QString, QString>(QString("English (UK)"), QString("en_GB")));

  return items;
}
