#pragma once

#include <QJsonObject>
#include <QResizeEvent>
#include <QLabel>
#include <QList>
#include <QScrollArea>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include "../nickelstoregraph.h"
#include "../widgets/dialog.h"
#include "../widgets/rating.h"

class ReviewDialog : public Dialog {
  Q_OBJECT

public:
  static void show();
  void commit() override;

public Q_SLOTS:
  void response(QJsonObject doc);

  void setRating(float value);
  void nudgeDown();
  void nudgeUp();
  void moodTapped();
  void questionTapped();

  void scrollPageUp();
  void scrollPageDown();
  void updateScrollButtons();

protected:
  void resizeEvent(QResizeEvent *event) override;

private:
  ReviewDialog();

  void updateRatingLabel();
  void repositionScrollButtons();

  float rating = 0;
  QList<int> moods;

  QString pace;
  QString characterOrPlotDriven;
  QString strongCharacterDev;
  QString loveableCharacters;
  QString diverseCharacters;
  QString flawedCharacters;

  QScrollArea *scroll = nullptr;
  QVBoxLayout *contentLayout = nullptr;
  QLabel *ratingLabel = nullptr;
  Rating *ratingWidget = nullptr;
  TouchTextEdit *touchText = nullptr;
  TouchLabel *scrollUpBtn = nullptr;
  TouchLabel *scrollDownBtn = nullptr;
};
