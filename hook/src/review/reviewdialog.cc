#include <QHBoxLayout>
#include <QJsonObject>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QTextEdit>
#include <QVBoxLayout>

#include <NickelHook.h>

#include "../cli.h"
#include "../menucontroller.h"
#include "../synccontroller.h"
#include "../widgets/label.h"
#include "../widgets/rating.h"
#include "reviewdialog.h"

static const struct { int id; const char *name; } MOODS[] = {
  {1,  "adventurous"}, {2,  "challenging"}, {3,  "dark"},        {4,  "emotional"},
  {5,  "funny"},       {6,  "hopeful"},      {7,  "informative"}, {8,  "inspiring"},
  {9,  "lighthearted"},{10, "mysterious"},   {11, "reflective"},  {12, "relaxing"},
  {13, "sad"},         {14, "tense"},
};

void ReviewDialog::show() { new ReviewDialog(); }

ReviewDialog::ReviewDialog() : Dialog("Write your review") {
  setStyleSheet(R"(
    N3ButtonLabel[moodSelected=true]  { background-color: #262626; color: white; }
    N3ButtonLabel[moodSelected=false] { background-color: #d9d9d9; color: black; }
  )");

  QVBoxLayout *outer = new QVBoxLayout(this);
  outer->setContentsMargins(12, 0, 12, 0);
  outer->setSpacing(0);

  scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->verticalScrollBar()->setSingleStep(200);
  outer->addWidget(scroll, 1);

  QWidget *contentWidget = new QWidget();
  contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setSpacing(16);
  contentLayout->setContentsMargins(0, 8, 0, 8);

  Label *loading = new Label(Label::Small, "Loading. Please wait...");
  loading->setAlignment(Qt::AlignCenter);
  contentLayout->addWidget(loading, 1);

  scroll->setWidget(contentWidget);

  CLI *cli = CLI::getUserBook();
  QObject::connect(cli, &CLI::response, this, &ReviewDialog::response);
  QObject::connect(cli, &CLI::failure, dialog, &QDialog::deleteLater);
}

void ReviewDialog::response(QJsonObject doc) {
  QLayoutItem *item = contentLayout->takeAt(0);
  if (item) { item->widget()->deleteLater(); delete item; }

  SyncController *ctl = SyncController::getInstance();
  QWidget *cw = scroll->widget();

  if (ctl->title != nullptr) {
    Label *title = new Label(Label::ExtraLarge, ctl->title);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-weight: bold;");
    contentLayout->addWidget(title);
  }
  if (ctl->author != nullptr) {
    Label *author = new Label(Label::Medium, "by " + ctl->author);
    author->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(author);
  }

  // ── Rating ──────────────────────────────────────────────────────────────────
  rating = (float)doc.value("rating").toDouble(0);

  // Single centered row: [−¼] [★★★★★] [+¼] [value]
  QWidget *ratingWidget2 = new QWidget(cw);
  QHBoxLayout *ratingRow = new QHBoxLayout(ratingWidget2);
  ratingRow->setSpacing(8);
  ratingRow->addStretch(1);

  N3ButtonLabel *nudgeMinus = construct_N3ButtonLabel(ratingWidget2);
  nudgeMinus->setText("−¼");
  nudgeMinus->setProperty("borderedButton", true);
  ratingRow->addWidget(nudgeMinus);
  QObject::connect(nudgeMinus, SIGNAL(tapped(bool)), this, SLOT(nudgeDown()));

  ratingWidget = new Rating(rating, ratingWidget2);
  ratingRow->addWidget(ratingWidget);
  QObject::connect(ratingWidget, &Rating::tapped, this, &ReviewDialog::setRating);

  N3ButtonLabel *nudgePlus = construct_N3ButtonLabel(ratingWidget2);
  nudgePlus->setText("+¼");
  nudgePlus->setProperty("borderedButton", true);
  ratingRow->addWidget(nudgePlus);
  QObject::connect(nudgePlus, SIGNAL(tapped(bool)), this, SLOT(nudgeUp()));

  ratingRow->addStretch(1);
  contentLayout->addWidget(ratingWidget2);

  ratingLabel = new QLabel(cw);
  ratingLabel->setAlignment(Qt::AlignCenter);
  updateRatingLabel();
  contentLayout->addWidget(ratingLabel);

  // ── Review text ─────────────────────────────────────────────────────────────
  { QWidget *sep = new QWidget(cw); sep->setFixedHeight(16); contentLayout->addWidget(sep); }
  QWidget *thoughtsHeader = new QWidget(cw);
  QHBoxLayout *thoughtsHeaderRow = new QHBoxLayout(thoughtsHeader);
  thoughtsHeaderRow->setContentsMargins(0, 0, 0, 0);
  thoughtsHeaderRow->addWidget(new Label(Label::Medium, "YOUR THOUGHTS"));
  thoughtsHeaderRow->addStretch(1);
  N3ButtonLabel *clearBtn = construct_N3ButtonLabel(thoughtsHeader);
  clearBtn->setText("Clear");
  clearBtn->setProperty("borderedButton", true);
  thoughtsHeaderRow->addWidget(clearBtn);
  contentLayout->addWidget(thoughtsHeader);

  touchText = construct_TouchTextEdit(cw);
  TouchTextEdit__setCustomPlaceholderText(touchText, "Share your thoughts about this book with the world.");
  touchText->setMinimumHeight(300);
  QTextEdit *textEdit = touchText->findChild<QTextEdit *>();
  textEdit->setText(doc.value("review_raw").toString(""));
  contentLayout->addWidget(touchText);
  QObject::connect(touchText, SIGNAL(tapped()), this, SLOT(showKeyboard()));
  QObject::connect(clearBtn, SIGNAL(tapped(bool)), textEdit, SLOT(clear()));

  // ── Moods ── 4 columns to fit smaller screens ────────────────────────────────
  { QWidget *sep = new QWidget(cw); sep->setFixedHeight(16); contentLayout->addWidget(sep); }
  contentLayout->addWidget(new Label(Label::Medium, "MOODS"));

  static const int MOOD_COLS = 4;
  static const int MOOD_COUNT = 14;
  QWidget *moodsContainer = new QWidget(cw);
  QVBoxLayout *moodsRows = new QVBoxLayout(moodsContainer);
  moodsRows->setContentsMargins(0, 0, 0, 0);
  moodsRows->setSpacing(4);

  for (int row = 0; row * MOOD_COLS < MOOD_COUNT; row++) {
    int start = row * MOOD_COLS;
    int end   = qMin(start + MOOD_COLS, MOOD_COUNT);
    int count = end - start;

    QWidget *rowWidget = new QWidget(moodsContainer);
    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(4);

    if (count < MOOD_COLS) rowLayout->addStretch(1);
    for (int i = start; i < end; i++) {
      N3ButtonLabel *btn = construct_N3ButtonLabel(rowWidget);
      btn->setText(MOODS[i].name);
      btn->setProperty("moodId", MOODS[i].id);
      btn->setProperty("moodSelected", false);
      rowLayout->addWidget(btn, 1);
      QObject::connect(btn, SIGNAL(tapped(bool)), this, SLOT(moodTapped()));
    }
    if (count < MOOD_COLS) rowLayout->addStretch(1);

    moodsRows->addWidget(rowWidget);
  }
  contentLayout->addWidget(moodsContainer);

  // ── Book properties ── single-select toggle groups ───────────────────────────
  { QWidget *sep = new QWidget(cw); sep->setFixedHeight(16); contentLayout->addWidget(sep); }
  contentLayout->addWidget(new Label(Label::Medium, "BOOK PROPERTIES"));

  // Store field pointer on the container so questionTapped() can find it.
  auto addQuestionRow = [&](const char *labelText, QList<Item> options, QString *field) {
    contentLayout->addWidget(new Label(Label::Small, labelText));

    QWidget *container = new QWidget(cw);
    container->setProperty("fieldPtr", (qulonglong)(void *)field);
    QHBoxLayout *row = new QHBoxLayout(container);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(4);

    for (const Item &opt : options) {
      N3ButtonLabel *btn = construct_N3ButtonLabel(container);
      btn->setText(opt.text);
      btn->setProperty("questionValue", opt.value);
      btn->setProperty("moodSelected", false);
      row->addWidget(btn, 1);
      QObject::connect(btn, SIGNAL(tapped(bool)), this, SLOT(questionTapped()));
    }

    contentLayout->addWidget(container);
  };

  addQuestionRow("Pace",
    {{"Slow", "slow"}, {"Medium", "medium"}, {"Fast", "fast"}},
    &pace);

  addQuestionRow("Plot or character driven?",
    {{"Plot", "Plot"}, {"Character", "Character"}, {"A mix", "A mix"}, {"N/A", "N/A"}},
    &characterOrPlotDriven);

  QList<Item> yesNo = {{"Yes", "Yes"}, {"No", "No"}, {"Complicated", "It's complicated"}, {"N/A", "N/A"}};
  addQuestionRow("Strong character dev?",       yesNo, &strongCharacterDev);
  addQuestionRow("Characters loveable?",        yesNo, &loveableCharacters);
  addQuestionRow("Diverse cast?",               yesNo, &diverseCharacters);
  addQuestionRow("Character flaws main focus?", yesNo, &flawedCharacters);

  // ── Submit ───────────────────────────────────────────────────────────────────
  N3ButtonLabel *submitBtn = construct_N3ButtonLabel(cw);
  submitBtn->setText("Submit review");
  submitBtn->setProperty("primaryButton", true);
  contentLayout->addWidget(submitBtn);
  QObject::connect(submitBtn, SIGNAL(tapped(bool)), this, SLOT(commit()));

  contentLayout->addStretch(1);

  buildKeyboardFrameHideOnly(textEdit, "Done");
}

// ── Rating helpers ────────────────────────────────────────────────────────────

void ReviewDialog::setRating(float value) {
  rating = value;
  updateRatingLabel();
}

void ReviewDialog::nudgeDown() {
  rating = qMax(0.0f, rating - 0.25f);
  if (ratingWidget) ratingWidget->setValue(rating);
  updateRatingLabel();
}

void ReviewDialog::nudgeUp() {
  rating = qMin(5.0f, rating + 0.25f);
  if (ratingWidget) ratingWidget->setValue(rating);
  updateRatingLabel();
}

void ReviewDialog::updateRatingLabel() {
  if (!ratingLabel) return;
  ratingLabel->setText(rating == 0.0f ? "—" : QString::number(rating));
}

// ── Mood toggle (multi-select) ────────────────────────────────────────────────

void ReviewDialog::moodTapped() {
  N3ButtonLabel *btn = qobject_cast<N3ButtonLabel *>(sender());
  if (!btn) return;

  int id = btn->property("moodId").toInt();
  bool selected = !btn->property("moodSelected").toBool();
  btn->setProperty("moodSelected", selected);
  QStyle *s = btn->style(); s->unpolish(btn); s->polish(btn); btn->update();

  if (selected) moods.append(id);
  else          moods.removeOne(id);
}

// ── Question toggle (single-select) ──────────────────────────────────────────

void ReviewDialog::questionTapped() {
  N3ButtonLabel *btn = qobject_cast<N3ButtonLabel *>(sender());
  if (!btn) return;

  QWidget *container = btn->parentWidget();
  QString *field = (QString *)(void *)(quintptr)container->property("fieldPtr").toULongLong();

  bool wasSelected = btn->property("moodSelected").toBool();

  for (N3ButtonLabel *sibling : container->findChildren<N3ButtonLabel *>()) {
    sibling->setProperty("moodSelected", false);
    QStyle *s = sibling->style(); s->unpolish(sibling); s->polish(sibling); sibling->update();
  }

  if (!wasSelected) {
    btn->setProperty("moodSelected", true);
    QStyle *s = btn->style(); s->unpolish(btn); s->polish(btn); btn->update();
    if (field) *field = btn->property("questionValue").toString();
  } else {
    if (field) *field = "";
  }
}

// ── Submit ────────────────────────────────────────────────────────────────────

void ReviewDialog::commit() {
  QTextEdit *textEdit = findChild<QTextEdit *>();

  CLI *cli = CLI::setUserBook(rating, textEdit ? textEdit->toPlainText() : QString(),
                               moods, pace, characterOrPlotDriven,
                               strongCharacterDev, loveableCharacters,
                               diverseCharacters, flawedCharacters);
  QObject::connect(cli, &CLI::success, dialog, [this]() {
    dialog->deleteLater();

    ConfirmationDialog *prompt = ConfirmationDialogFactory__getConfirmationDialog(nullptr);
    ConfirmationDialog__setTitle(prompt, "Review Saved!");
    ConfirmationDialog__setText(prompt, "Would you like to update this book's reading status on Storygraph?");
    ConfirmationDialog__setAcceptButtonText(prompt, "Finished");
    ConfirmationDialog__setRejectButtonText(prompt, "Did Not Finish");
    ConfirmationDialog__showCloseButton(prompt, true);
    QObject::connect(prompt, &QDialog::accepted, prompt, [prompt]() {
      CLI::setUserBook(3);
      prompt->deleteLater();
    });
    QObject::connect(prompt, &QDialog::rejected, prompt, [prompt]() {
      CLI::setUserBook(5);
      prompt->deleteLater();
    });
    prompt->open();
  });
  QObject::connect(cli, &CLI::failure, dialog, &QDialog::deleteLater);
}
