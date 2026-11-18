#include "autopilot.h"
#include <cmath>
#include <utility>

Autopilot::Autopilot(QString name, UAV* vehicle, const std::vector<Sensor*>& sensors,
                     NavigationManager* navigation, FlightController* controller, QObject* parent)
    : QObject(parent), name_(std::move(name)), vehicle_(vehicle), sensors_(sensors),
      navigation_(navigation), controller_(controller) {}
QString Autopilot::name() const { return name_; }
double Autopilot::time() const { return time_; }
double Autopilot::target_altitude() const { return target_altitude_; }
bool Autopilot::has_fault() const { return fault_; }
bool Autopilot::is_running() const { return running_; }
QString Autopilot::mode() const
{
    switch (mode_) {
    case Mode::Stopped: return "Stopped";
    case Mode::Takeoff: return "Takeoff";
    case Mode::Mission: return "Mission";
    case Mode::Landing: return "Landing";
    case Mode::Landed: return "Landed";
    }
    return {};
}
void Autopilot::set_mode(Mode mode)
{
    if (mode_ == mode) return;
    mode_ = mode;
    emit mode_changed(this->mode());
}
bool Autopilot::set_target_altitude(double altitude)
{
    if (fault_ || !std::isfinite(altitude) || altitude < 0 || altitude > 500) return false;
    target_altitude_ = altitude;
    if (controller_) controller_->reset();
    if (mode_ != Mode::Stopped) set_mode(altitude == 0 ? Mode::Landing : Mode::Mission);
    return true;
}
void Autopilot::command_takeoff()
{
    if (fault_) return;
    set_target_altitude(200);
    set_mode(Mode::Takeoff);
    running_ = true;
}
void Autopilot::command_land()
{
    if (fault_) return;
    set_target_altitude(0);
    set_mode(Mode::Landing);
    running_ = true;
}
void Autopilot::command_stop()
{
    running_ = false;
    if (controller_) controller_->reset();
    set_mode(Mode::Stopped);
}
void Autopilot::resume()
{
    if (fault_) return;
    if (mode_ == Mode::Stopped) set_mode(target_altitude_ == 0 ? Mode::Landing : Mode::Mission);
    running_ = true;
}
void Autopilot::pause() { running_ = false; }
void Autopilot::reset_fault()
{
    fault_ = false;
    if (controller_) controller_->reset();
}
void Autopilot::fail(const QString& reason)
{
    fault_ = true;
    running_ = false;
    emit fault_detected(name_, reason);
}
void Autopilot::run_cycle()
{
    if (!running_ || fault_) return;
    if (!vehicle_) { fail("vehicle is not set"); return; }
    if (sensors_.empty()) { fail("sensors are not set"); return; }
    for (const Sensor* sensor : sensors_) {
        if (!sensor || !sensor->is_ready() || !std::isfinite(sensor->read())) {
            fail("sensor failure: " + (sensor ? sensor->name() : QString("unknown")));
            return;
        }
    }
    if (!navigation_ || !navigation_->update(vehicle_->position())) {
        fail("navigation data invalid"); return;
    }
    if (!controller_) { fail("flight controller is not set"); return; }
    const double error = target_altitude_ - navigation_->get_position().z;
    double control = mode_ == Mode::Landed ? 0 : controller_->calculate_control(error, time_step);
    if (!std::isfinite(control)) { fail("invalid controller output"); return; }
    if (mode_ == Mode::Landing && vehicle_->position().z < 0.1) {
        control = -vehicle_->position().z / time_step;
        set_mode(Mode::Landed);
        controller_->reset();
    }
    vehicle_->advance(control, time_step, mode_ != Mode::Landed);
    time_ += time_step;
    const Point3D position = vehicle_->position();
    if (mode_ == Mode::Takeoff && std::abs(target_altitude_ - position.z) < 0.5)
        set_mode(Mode::Mission);
    emit telemetry_updated(name_, position.z, target_altitude_ - position.z, control);
    emit position_updated(name_, position.x, position.y, vehicle_->roll());
}
