#pragma once
#include <QGraphicsView>
#include <array>

class QGraphicsPathItem;
class QGraphicsPixmapItem;
class RouteView : public QGraphicsView {
public:
    explicit RouteView(QWidget* parent = nullptr);
    void add_aircraft(int index, const QString& name, const QString& image, const QColor& color,
                      const QPointF& position);
    void update_position(int index, double x, double y, double roll);
    const QGraphicsPixmapItem* marker(int index) const;
    const QGraphicsPathItem* trajectory(int index) const;
protected:
    void resizeEvent(QResizeEvent* event) override;
private:
    std::array<QGraphicsPixmapItem*, 2> markers_{};
    std::array<QGraphicsPathItem*, 2> trails_{};
};
