#pragma once

#include "point3d.h"
#include <QString>

// Учебная кинематика; координаты площадки — метры, высота — ось Z.
class UAV {
public:
    virtual QString type() const = 0;
    virtual QString image_resource() const = 0;
    virtual double cruise_speed() const = 0;
    virtual ~UAV() = default;
    Point3D position() const;
    double battery() const;
    double roll() const;
    void advance(double control, double dt, bool airborne);
protected:
    explicit UAV(double radius, double phase);
private:
    double radius_;
    double phase_;
    Point3D position_;
    double battery_ = 100;
    double roll_ = 0;
};

class Quadcopter final : public UAV {
public:
    Quadcopter();
    QString type() const override;
    QString image_resource() const override;
    double cruise_speed() const override;
};

class FixedWingUAV final : public UAV {
public:
    FixedWingUAV();
    QString type() const override;
    QString image_resource() const override;
    double cruise_speed() const override;
};
