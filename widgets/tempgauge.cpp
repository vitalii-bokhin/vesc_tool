/*
    Copyright 2016 - 2025 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */

#include "tempgauge.h"
#include "utility.h"
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

TempGauge::TempGauge(QWidget *parent) : QWidget(parent)
{
    mBoxH = 10;
    mBoxW = 10;
    mTxtOfs = 2;

    mValues.temp_mos = 0.0;
    mValues.temp_motor = 0.0;

    setMinimumHeight(60);
}

void TempGauge::setValues(const MC_VALUES &values)
{
    mValues.temp_mos = values.temp_mos;
    mValues.temp_motor = values.temp_motor;
    update();
    emit valuesChanged();
}

QSize TempGauge::sizeHint() const
{
    QString str = "T FET   : 0.00 \u00B0C\nT Motor : 0.00 \u00B0C\n";

    QFont font;
    font.setFamily("DejaVu Sans Mono");
    font.setPointSize(11);

    QFontMetrics fm(font);
    int width = fm.horizontalAdvance(QLatin1Char('T')); // fallback width
    int height = fm.height() * 2; // two lines

    // Get actual text bounds
    QRect textRect = fm.boundingRect(str);
    width = textRect.width();
    height = textRect.height();

    int txtOfs = 5;
    return QSize(width + 2 * txtOfs, height + 2 * txtOfs);
}

void TempGauge::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Paint begins here
    painter.fillRect(event->rect(), QBrush(Qt::transparent));

    QFont font;
    font.setFamily("DejaVu Sans Mono");
    font.setPointSize(11);
    painter.setFont(font);

    QRectF br = painter.boundingRect(QRectF(0, 0, 4000, 4000),
                                    "T FET   : 0.00 \u00B0C\n"
                                    "T Motor : 0.00 \u00B0C\n");

    int boxh_new = br.height();
    int boxw_new = br.width();
    int txtofs_new = 5;

    if (mBoxH != boxh_new ||
            mBoxW != boxw_new ||
            mTxtOfs != txtofs_new) {
        mBoxH = boxh_new;
        mBoxW = boxw_new;
        mTxtOfs = txtofs_new;
        setMinimumWidth(mBoxW + 2 * mTxtOfs);
        setMinimumHeight(mBoxH + 2 * mTxtOfs);
        updateGeometry();
    }

    QString str;

    const double bbox_w = mBoxW + 2 * mTxtOfs;
    const double bbow_h = mBoxH + 2 * mTxtOfs;
    const double vidw = event->rect().width();

    // Temperature box (same as middle box in RtDataText)
    str = QString::asprintf("T FET   : %.2f \u00B0C\n"
                "T Motor : %.2f \u00B0C",
                mValues.temp_mos,
                mValues.temp_motor);

    painter.setOpacity(0.7);
    painter.fillRect(0, 0, bbox_w, bbow_h, Utility::getAppQColor("normalBackground"));
    painter.setOpacity(1.0);

    painter.setPen(Utility::getAppQColor("normalText"));
    painter.drawText(QRectF(mTxtOfs, mTxtOfs, mBoxW, mBoxH),
                     Qt::AlignLeft, str);
}