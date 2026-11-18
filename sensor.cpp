#include "sensor.h"
#include <limits>

bool Sensor::is_ready() const { return ready_; }
void Sensor::set_ready(bool ready) { ready_ = ready; }
double Gyroscope::read() const { return is_ready() ? 1.25 : std::numeric_limits<double>::quiet_NaN(); }
QString Gyroscope::name() const { return "Gyroscope"; }
double Accelerometer::read() const { return is_ready() ? 9.81 : std::numeric_limits<double>::quiet_NaN(); }
QString Accelerometer::name() const { return "Accelerometer"; }
double Barometer::read() const { return is_ready() ? 1012.8 : std::numeric_limits<double>::quiet_NaN(); }
QString Barometer::name() const { return "Barometer"; }
