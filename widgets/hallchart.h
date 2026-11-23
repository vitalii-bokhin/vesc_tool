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

    void setHallTabValues(const QVector<int> &values);    // hall sensors table
    void setDetectFocHallValues(const QVector<int> &values);  // detect foc hall
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<int> mHallTabValues;
    QVector<int> mDetectFocHallValues;
    static const int Diameter = 200;
    static const int Radius = 100;
};

#endif // HALLCHART_H
