#pragma once
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <array>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

// Цвета, сетка и скользящая история — как SignalGraph предыдущей работы.
class AltitudeGraph : public QChartView {
public:
    explicit AltitudeGraph(const QString& title, QWidget* parent = nullptr);
    void add_sample(int aircraft, double time, double value);
    const QLineSeries* series(int aircraft) const;
private:
    std::array<QLineSeries*, 2> series_;
    QValueAxis* time_axis_;
    QValueAxis* value_axis_;
};
