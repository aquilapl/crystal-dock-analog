/*
 * This file is part of Crystal Dock.
 * Copyright (C) 2022 Viet Dang (dangvd@gmail.com)
 *
 * Crystal Dock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Crystal Dock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Crystal Dock.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "clock.h"

#include <QColor>
#include <QDate>
#include <QFont>
#include <QIcon>
#include <QTime>
#include <QTimer>

#include "dock_panel.h"
#include <utils/draw_utils.h>
#include <utils/font_utils.h>

namespace crystaldock {

constexpr float Clock::kWhRatio;
constexpr float Clock::kDelta;

Clock::Clock(DockPanel* parent, MultiDockModel* model,
             Qt::Orientation orientation, int minSize, int maxSize)
    : IconlessDockItem(parent, model, "" /* label */, orientation, minSize, maxSize,
                       kWhRatio),
      calendar_(parent),
      fontFamilyGroup_(this) {
  createMenu();
  loadConfig();

  QTimer* timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(updateTime()));
  timer->start(1000);  // update the time every second.

  connect(&menu_, &QMenu::aboutToHide, this,
          [this]() {
            parent_->setShowingPopup(false);
          });
}

void Clock::draw(QPainter *painter) const {
  if (model_->useAnalogClock()) { 
    drawAnalogClock(painter);      
    return;                         
  }                                 
  const QString timeFormat = model_->use24HourClock() ? "hh:mm" : "hh:mm AP";
  const QString time = QTime::currentTime().toString(timeFormat);
  // The reference time used to calculate the font size.
  const QString referenceTime = QTime(8, 8).toString(timeFormat);

  const int margin = parent_->isHorizontal() ? getHeight() * 0.1 : 0;
  const auto x = left_ + margin;
  const auto y = top_;
  const auto w = getWidth() - margin;
  const auto h = getHeight();
  painter->setFont(adjustFontSize(w, h, referenceTime,
                                  model_->clockFontScaleFactor(),
                                  model_->clockFontFamily()));
  painter->setRenderHint(QPainter::TextAntialiasing);
  if (size_ > minSize_) {
    drawBorderedText(x, y , w, h, Qt::AlignCenter, time,
                     2 /* borderWidth */, Qt::black, Qt::white, painter,
                     /*simplified=*/ true);
  } else {
    painter->setPen(Qt::white);
    painter->drawText(x, y, w, h, Qt::AlignCenter, time);
  }
}

void Clock::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton) {
    calendar_.showCalendar();
  } else if (e->button() == Qt::RightButton) {
    // In case other docks have changed the config.
    loadConfig();
    showPopupMenu(&menu_);
  }
}

QString Clock::getLabel() const {
  return QLocale::system().toString(QDate::currentDate(), QLocale::LongFormat);
}

void Clock::updateTime() {
  parent_->update();
}

void Clock::setFontScaleFactor(float fontScaleFactor) {
  largeFontAction_->setChecked(
      fontScaleFactor > kLargeClockFontScaleFactor - kDelta);
  mediumFontAction_->setChecked(
      fontScaleFactor > kMediumClockFontScaleFactor - kDelta &&
      fontScaleFactor < kMediumClockFontScaleFactor + kDelta);
  smallFontAction_->setChecked(
      fontScaleFactor < kSmallClockFontScaleFactor + kDelta);
}

void Clock::setLargeFont() {
  setFontScaleFactor(kLargeClockFontScaleFactor);
  saveConfig();
}

void Clock::setMediumFont() {
  setFontScaleFactor(kMediumClockFontScaleFactor);
  saveConfig();
}

void Clock::setSmallFont() {
  setFontScaleFactor(kSmallClockFontScaleFactor);
  saveConfig();
}

void Clock::createMenu() {
  menu_.addSection("Clock");
  use24HourClockAction_ = menu_.addAction(
      QString("Use 24-hour Clock"), this,
      [this] {
        saveConfig();
      });
  use24HourClockAction_->setCheckable(true);
  
  useAnalogClockAction_ = menu_.addAction(  
      QString("Analog Clock"), this,        
      [this] {                               
        saveConfig();                        
      });                                    
  useAnalogClockAction_->setCheckable(true); 

  QMenu* fontFamily = menu_.addMenu(QString("Font Family"));
  for (const auto& family : getBaseFontFamilies()) {
    auto fontFamilyAction = fontFamily->addAction(family, this, [this, family]{
      model_->setClockFontFamily(family);
      model_->saveAppearanceConfig(true /* repaintOnly */);
    });
    fontFamilyAction->setCheckable(true);
    fontFamilyAction->setActionGroup(&fontFamilyGroup_);
    fontFamilyAction->setChecked(family == model_->clockFontFamily());
  }

  QMenu* fontSize = menu_.addMenu(QString("Font Size"));
  largeFontAction_ = fontSize->addAction(QString("Large Font"),
                                         this,
                                         SLOT(setLargeFont()));
  largeFontAction_->setCheckable(true);
  mediumFontAction_ = fontSize->addAction(QString("Medium Font"),
                                          this,
                                          SLOT(setMediumFont()));
  mediumFontAction_->setCheckable(true);
  smallFontAction_ = fontSize->addAction(QString("Small Font"),
                                         this,
                                         SLOT(setSmallFont()));
  smallFontAction_->setCheckable(true);

  menu_.addSeparator();
  parent_->addPanelSettings(&menu_);
}

void Clock::loadConfig() {
  use24HourClockAction_->setChecked(model_->use24HourClock());
  useAnalogClockAction_->setChecked(model_->useAnalogClock());
  // Ustawienie szerokości (1.0 = kwadrat dla analogowego)
  whRatio_ = model_->useAnalogClock() ? 1.0 : kWhRatio;
  setFontScaleFactor(model_->clockFontScaleFactor());
}

void Clock::saveConfig() {
  model_->setUse24HourClock(use24HourClockAction_->isChecked());
  model_->setUseAnalogClock(useAnalogClockAction_->isChecked());
  // Przełączanie szerokości na żywo
  whRatio_ = useAnalogClockAction_->isChecked() ? 1.0 : kWhRatio;
  model_->setClockFontScaleFactor(fontScaleFactor());
  model_->saveAppearanceConfig(false); 
}

void Clock::drawAnalogClock(QPainter* painter) const {
  const int h = getHeight();
  const int w = getWidth();
  const int radius = qMin(w, h) / 2 - 2;
  const int centerX = left_ + w / 2;
  const int centerY = top_ + h / 2;
  
  painter->setRenderHint(QPainter::Antialiasing);
  QColor cyan(0, 255, 255);
  
  // 1. Tarcza zegara (grubsza linia)
  painter->setPen(QPen(cyan, 3));
  painter->setBrush(Qt::NoBrush);
  painter->drawEllipse(QPoint(centerX, centerY), radius, radius);
  
  // 2. Kreseczki (godzinowe i minutowe)
  painter->save();
  painter->translate(centerX, centerY);
  for (int i = 0; i < 60; ++i) {
    if (i % 5 == 0) {
      // Godziny
      painter->setPen(QPen(cyan, 2.5));
      painter->drawLine(0, -radius + 2, 0, -radius + 8);
    } else {
      // Minuty
      painter->setPen(QPen(cyan, 1));
      painter->drawLine(0, -radius + 2, 0, -radius + 5);
    }
    painter->rotate(6.0);
  }
  painter->restore();
  
  QTime time = QTime::currentTime();
  
  // 3. Wskazówka godzinowa
  painter->save();
  painter->translate(centerX, centerY);
  painter->rotate(30.0 * ((time.hour() + time.minute() / 60.0)));
  painter->setPen(QPen(cyan, 4, Qt::SolidLine, Qt::RoundCap));
  painter->drawLine(0, 0, 0, -radius * 0.5);
  painter->restore();
  
  // 4. Wskazówka minutowa
  painter->save();
  painter->translate(centerX, centerY);
  painter->rotate(6.0 * (time.minute() + time.second() / 60.0));
  painter->setPen(QPen(cyan, 2.5, Qt::SolidLine, Qt::RoundCap));
  painter->drawLine(0, 0, 0, -radius * 0.85);
  painter->restore();
  
  // 5. Środek
  painter->setBrush(cyan);
  painter->setPen(Qt::NoPen);
  painter->drawEllipse(QPoint(centerX, centerY), 3, 3);
}

}  // namespace crystaldock
