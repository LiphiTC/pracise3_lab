#include "uav.h"
#include <algorithm>
#include <cmath>

UAV::UAV(double radius, double phase)
    : radius_(radius), phase_(phase),
      position_{300 + radius * std::cos(phase), 200 + radius * std::sin(phase), 0} {}
Point3D UAV::position() const { return position_; }
double UAV::battery() const { return battery_; }
double UAV::roll() const { return roll_; }
void UAV::advance(double control, double dt, bool airborne)
{
    position_.z = std::max(0.0, position_.z + control * dt);
    if (airborne && position_.z > 0.05) {
        phase_ = std::fmod(phase_ + cruise_speed() / radius_ * dt, 6.283185307179586);
        position_.x = 300 + radius_ * std::cos(phase_);
        position_.y = 200 + radius_ * std::sin(phase_);
        // Наглядная имитация крена, а не аэродинамическая модель.
        roll_ = 20 * std::sin(phase_);
        battery_ = std::max(0.0, battery_ - 0.02 * dt);
    } else {
        roll_ = 0;
    }
}
Quadcopter::Quadcopter() : UAV(100, 0) {}
QString Quadcopter::type() const { return "Quadcopter"; }
QString Quadcopter::image_resource() const { return ":/assets/drone.jpg"; }
double Quadcopter::cruise_speed() const { return 12; }
FixedWingUAV::FixedWingUAV() : UAV(150, 3.141592653589793) {}
QString FixedWingUAV::type() const { return "Fixed-wing UAV"; }
QString FixedWingUAV::image_resource() const { return ":/assets/fixedwing.svg"; }
double FixedWingUAV::cruise_speed() const { return 22; }
