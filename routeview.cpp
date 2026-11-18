#include "routeview.h"
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QResizeEvent>

RouteView::RouteView(QWidget* parent) : QGraphicsView(parent)
{
    auto* scene = new QGraphicsScene(this);
    scene->setSceneRect(-35, -35, 670, 470);
    setScene(scene);
    setMinimumSize(480, 320);
    setRenderHint(QPainter::Antialiasing);
    setBackgroundBrush(QColor(23, 32, 46));
    for (int x = 0; x <= 600; x += 50)
        scene->addLine(x, 0, x, 400, QPen(QColor(80, 95, 110), 0));
    for (int y = 0; y <= 400; y += 50)
        scene->addLine(0, y, 600, y, QPen(QColor(80, 95, 110), 0));
    for (int radius : {100, 150})
        scene->addEllipse(300 - radius, 200 - radius, 2 * radius, 2 * radius,
                          QPen(QColor("#7b899c"), 1, Qt::DashLine));
    auto* caption = scene->addText("Площадка 600 × 400 м · сетка 50 м · крен ±20°");
    caption->setDefaultTextColor(Qt::white);
    caption->setPos(0, 405);
}
void RouteView::add_aircraft(int index, const QString& name, const QString& image,
                            const QColor& color, const QPointF& position)
{
    QPainterPath path(position);
    trails_.at(index) = scene()->addPath(path, QPen(color, 2));
    const QPixmap pixmap = QPixmap(image).scaled(48, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    auto* marker = scene()->addPixmap(pixmap);
    marker->setOffset(-pixmap.width() / 2.0, -pixmap.height() / 2.0);
    marker->setPos(position);
    marker->setZValue(2);
    marker->setToolTip(name);
    auto* label = new QGraphicsTextItem(name, marker);
    label->setDefaultTextColor(color.lighter(140));
    label->setPos(-25, 20);
    markers_.at(index) = marker;
}
void RouteView::update_position(int index, double x, double y, double roll)
{
    auto* marker = markers_.at(index);
    auto* trail = trails_.at(index);
    const QPointF position(x, y);
    QPainterPath path = trail->path();
    // Qt может считать путь из одного moveTo пустым и не сохранить его в item.
    if (path.isEmpty()) path.moveTo(marker->pos());
    if (path.currentPosition() != position) {
        path.lineTo(position);
        trail->setPath(path);
    }
    marker->setPos(position);
    marker->setRotation(roll);
}
const QGraphicsPixmapItem* RouteView::marker(int index) const { return markers_.at(index); }
const QGraphicsPathItem* RouteView::trajectory(int index) const { return trails_.at(index); }
void RouteView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}
