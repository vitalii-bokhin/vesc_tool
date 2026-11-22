#ifndef HALLCHART_H
#define HALLCHART_H

#include <QWidget>
#include <QVector>

class HallChart : public QWidget
{
    Q_OBJECT

public:
    explicit HallChart(QWidget *parent = nullptr);
    ~HallChart();

    void setYellowValues(const QVector<int> &values);  // detectFocHall
    void setBlueValues(const QVector<int> &values);    // hallTab
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<int> mYellowValues;
    QVector<int> mBlueValues;
    static const int Diameter = 200;
    static const int Radius = 100;
};

#endif // HALLCHART_H
