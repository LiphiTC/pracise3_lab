#include "controller.h"
#include <algorithm>
#include <cmath>
#include <limits>

PIDController::PIDController(double kp, double ki, double kd) : kp_(kp), ki_(ki), kd_(kd) {}
double PIDController::calculate_control(double error, double dt)
{
    if (!std::isfinite(error) || !std::isfinite(dt) || dt <= 0
        || !std::isfinite(kp_) || !std::isfinite(ki_) || !std::isfinite(kd_))
        return std::numeric_limits<double>::quiet_NaN();
    const double derivative = first_ ? 0 : (error - previous_) / dt;
    const double candidate = std::clamp(integral_ + error * dt, -100.0, 100.0);
    const double raw = kp_ * error + ki_ * candidate + kd_ * derivative;
    // Не накапливаем интегратор при насыщении скорости набора/снижения.
    if (std::abs(raw) <= 25 || raw * error < 0) integral_ = candidate;
    previous_ = error;
    first_ = false;
    return std::clamp(raw, -25.0, 25.0);
}
QString PIDController::controller_name() const { return "PID"; }
void PIDController::reset() { integral_ = previous_ = 0; first_ = true; }
