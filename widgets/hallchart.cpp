#include "hallchart.h"
#include "utility.h"
#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <cmath>

const int HallChart::Diameter;
const int HallChart::Radius;

HallChart::HallChart(QWidget *parent) : QWidget(parent)
{
    mHallTabValues.fill(0, 8);
    mDetectFocHallValues.fill(0, 8);
    setMinimumSize(Diameter, Diameter);
}

HallChart::~HallChart()
{
}

QSize HallChart::sizeHint() const
{
    return QSize(Diameter, Diameter);
}

void HallChart::setHallTabValues(const QVector<int> &values)
{
    mHallTabValues = values;
    update();
}

void HallChart::setDetectFocHallValues(const QVector<int> &values)
{
    mDetectFocHallValues = values;
    update();
}

void HallChart::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int cx = width() / 2;
    int cy = height() / 2;

    // Circle with diameter 200 x 200 px
    QRect circleRect(cx - Radius, cy - Radius, Diameter, Diameter);
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.drawEllipse(circleRect);

    // Horizontal and vertical axes through the center
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.drawLine(cx - Radius, cy, cx + Radius, cy);
    painter.drawLine(cx, cy - Radius, cy, cy + Radius);

    // Circle labels (placed inside the circle, same color as the circle)
    QFont font;
    font.setPointSize(10);
    painter.setFont(font);
    painter.setPen(QColor(170, 170, 170));

    const int labelW = 44;
    const int labelH = 16;
    const int inset = 7;

    painter.drawText(cx - labelW / 2, cy - Radius + inset, labelW, labelH, Qt::AlignCenter, "0–200");
    painter.drawText(cx + Radius - inset - 28, cy - labelH / 2, 28, labelH, Qt::AlignCenter, "50");
    painter.drawText(cx - labelW / 2, cy + Radius - labelH - inset, labelW, labelH, Qt::AlignCenter, "100");
    painter.drawText(cx - Radius + inset, cy - labelH / 2, 28, labelH, Qt::AlignCenter, "150");

    // Draw a single value as a vector line from center to the circle.
    // value 0 = top, 50 = right, 100 = bottom, 150 = left (clockwise from top).
    auto drawVector = [&](int value, QColor color, int maxRadius) {
        if (value < 0 || value > 200) {
            return;
        }
        double angleDeg = -90.0 + (value / 200.0) * 360.0;
        double angleRad = angleDeg * M_PI / 180.0;
        int ex = cx + (int)(maxRadius * std::cos(angleRad));
        int ey = cy + (int)(maxRadius * std::sin(angleRad));
        painter.setPen(QPen(color, 2));
        painter.drawLine(cx, cy, ex, ey);
    };

    // Blue vectors (hallTab table) - reach the circle
    for (int i = 0; i < mHallTabValues.size(); ++i) {
        drawVector(mHallTabValues.at(i), Utility::getAppQColor("plot_graph1"), Radius);
    }

    // Green vectors (detect foc hall) - shorter, inside the blue ones,
    // so both series remain distinguishable when they overlap
    for (int i = 0; i < mDetectFocHallValues.size(); ++i) {
        drawVector(mDetectFocHallValues.at(i), Utility::getAppQColor("green"), Radius - 40);
    }
}
