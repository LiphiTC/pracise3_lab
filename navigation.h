#pragma once
#include "point3d.h"
#include <QObject>

class NavigationSystem {
public:
    virtual bool is_valid() const = 0;
    virtual QString system_name() const = 0;
    virtual ~NavigationSystem() = default;
    void set_valid(bool valid);
protected:
    bool valid() const;
private:
    bool valid_ = true;
};
class GPSNavigation final : public NavigationSystem {
public:
    bool is_valid() const override;
    QString system_name() const override;
};
class InertialNavigation final : public NavigationSystem {
public:
    bool is_valid() const override;
    QString system_name() const override;
};
class VisualNavigation final : public NavigationSystem {
public:
    bool is_valid() const override;
    QString system_name() const override;
};

class NavigationManager : public QObject {
    Q_OBJECT
public:
    explicit NavigationManager(QObject* parent = nullptr);
    bool update(const Point3D& position);
    Point3D get_position() const;
    QString system_name() const;
    void set_gps_valid(bool valid);
    void set_backups_valid(bool valid);
signals:
    void active_system_changed(QString name);
private:
    GPSNavigation gps_;
    InertialNavigation inertial_;
    VisualNavigation visual_;
    NavigationSystem* active_ = &gps_;
    Point3D position_{0, 0, 0};
};
