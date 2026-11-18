#include "altitudegraph.h"
#include <QtCharts/QLegend>
#include <algorithm>
#include <cmath>

AltitudeGraph::AltitudeGraph(const QString& title, QWidget* parent) : QChartView(new QChart, parent)
{
    setMinimumSize(430, 220);
    setRenderHint(QPainter::Antialiasing);
    chart()->setTitle(title);
    chart()->setAnimationOptions(QChart::NoAnimation);
    chart()->setBackgroundBrush(palette().base());
    chart()->setTitleBrush(palette().text());
    chart()->legend()->setLabelColor(palette().text().color());
    chart()->legend()->setVisible(true);
    chart()->legend()->setAlignment(Qt::AlignBottom);
    chart()->setMargins(QMargins(8, 4, 8, 4));
    time_axis_ = new QValueAxis(chart());
    value_axis_ = new QValueAxis(chart());
    time_axis_->setTitleText("Время модели, с");
    time_axis_->setRange(0, 10);
    value_axis_->setRange(0, 220);
    chart()->addAxis(time_axis_, Qt::AlignBottom);
    chart()->addAxis(value_axis_, Qt::AlignLeft);
    for (auto* axis : {time_axis_, value_axis_}) {
        axis->setLabelsColor(palette().text().color());
        axis->setTitleBrush(palette().text());
        axis->setGridLineColor(palette().mid().color());
        axis->setLabelFormat("%.1f");
    }
    const std::array<QColor, 2> colors{QColor("#278bc4"), QColor("#8c56c4")};
    for (int i = 0; i < 2; ++i) {
        series_[i] = new QLineSeries(chart());
        series_[i]->setName(i == 0 ? "Quad-1" : "Wing-2");
        series_[i]->setPen(QPen(colors[i], 2));
        chart()->addSeries(series_[i]);
        series_[i]->attachAxis(time_axis_);
        series_[i]->attachAxis(value_axis_);
    }
}
const QLineSeries* AltitudeGraph::series(int aircraft) const { return series_.at(aircraft); }
void AltitudeGraph::add_sample(int aircraft, double time, double value)
{
    if (!std::isfinite(time) || !std::isfinite(value)) return;
    auto* series = series_.at(aircraft);
    series->append(time, value);
    if (series->count() > 600) series->remove(0);
    double begin = time, end = 10, low = 0, high = 1;
    for (auto* line : series_) {
        for (const QPointF& point : line->points()) {
            begin = std::min(begin, point.x());
            end = std::max(end, point.x());
            low = std::min(low, point.y());
            high = std::max(high, point.y());
        }
    }
    time_axis_->setRange(begin, std::max(begin + 1, end));
    const double margin = (high - low) * 0.1;
    value_axis_->setRange(low < 0 ? low - margin : 0, high + margin);
}
