#include "autopilot.h"
#include <QCoreApplication>
#include <QSignalSpy>
#include <cmath>
#include <limits>

namespace {
void check(bool condition, const char* message) { if (!condition) qFatal("FAILED: %s", message); }
}
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    Quadcopter quad;
    FixedWingUAV wing;
    Gyroscope gyro;
    Accelerometer accel;
    Barometer baro;
    NavigationManager navigation;
    PIDController controller;
    Autopilot pilot("Quad-1", &quad, {&gyro, &accel, &baro}, &navigation, &controller);
    QSignalSpy telemetry(&pilot, &Autopilot::telemetry_updated);
    QSignalSpy position(&pilot, &Autopilot::position_updated);
    QSignalSpy faults(&pilot, &Autopilot::fault_detected);
    QSignalSpy modes(&pilot, &Autopilot::mode_changed);
    QSignalSpy switches(&navigation, &NavigationManager::active_system_changed);
    pilot.run_cycle();
    check(telemetry.isEmpty() && pilot.mode() == "Stopped", "initial state is stopped");
    pilot.command_takeoff();
    check(pilot.target_altitude() == 200 && pilot.mode() == "Takeoff", "takeoff command and mode");
    pilot.run_cycle();
    check(telemetry.size() == 1 && telemetry.last()[0] == "Quad-1", "named telemetry");
    check(quad.position().z > 0 && position.size() == 1, "motion and position signal");
    check(std::abs(telemetry.last()[2].toDouble() - (200 - quad.position().z)) < 1e-9, "coherent telemetry snapshot");
    for (int i = 0; i < 1600; ++i) pilot.run_cycle();
    check(std::abs(quad.position().z - 200) < 0.1, "PID converges to 200 m");
    check(pilot.mode() == "Mission" && !modes.isEmpty(), "takeoff completes in model");
    const double time = pilot.time();
    const auto before_pause = quad.position();
    pilot.pause();
    pilot.run_cycle();
    check(pilot.time() == time && quad.position().z == before_pause.z, "pause freezes state");
    pilot.resume();
    pilot.run_cycle();
    check(pilot.time() > time, "resume preserves trajectory and time");
    check(!pilot.set_target_altitude(-1) && !pilot.set_target_altitude(501)
        && !pilot.set_target_altitude(std::numeric_limits<double>::quiet_NaN()), "invalid target rejected");
    pilot.set_target_altitude(300);
    pilot.run_cycle();
    check(telemetry.last()[2].toDouble() > 90, "target change immediately changes error");
    for (int i = 0; i < 1600; ++i) pilot.run_cycle();
    check(std::abs(quad.position().z - 300) < 0.1, "PID converges to 300 m");
    navigation.set_gps_valid(false);
    pilot.run_cycle();
    check(navigation.system_name() == "INS" && !pilot.has_fault(), "GPS fallback keeps mission alive");
    navigation.set_gps_valid(true);
    pilot.run_cycle();
    check(navigation.system_name() == "GPS" && switches.size() == 2, "GPS priority restored");
    gyro.set_ready(false);
    const int count = telemetry.size();
    pilot.run_cycle();
    pilot.run_cycle();
    check(faults.size() == 1 && faults.last()[1].toString().contains("Gyroscope"), "fault reason and latch");
    check(telemetry.size() == count && pilot.has_fault(), "no telemetry after failure");
    pilot.command_takeoff();
    check(!pilot.is_running(), "command cannot bypass fault");
    gyro.set_ready(true);
    pilot.reset_fault();
    pilot.resume();
    pilot.run_cycle();
    check(telemetry.size() == count + 1 && !pilot.has_fault(), "recovery resumes same model");
    navigation.set_gps_valid(false);
    navigation.set_backups_valid(false);
    pilot.run_cycle();
    check(pilot.has_fault() && faults.last()[1].toString() == "navigation data invalid", "all navigation lost");
    navigation.set_gps_valid(true);
    pilot.reset_fault();
    pilot.command_land();
    check(pilot.target_altitude() == 0 && pilot.mode() == "Landing", "landing command");
    for (int i = 0; i < 2000; ++i) pilot.run_cycle();
    check(quad.position().z == 0 && pilot.mode() == "Landed", "landing reaches ground");
    const auto landed = quad.position();
    pilot.run_cycle();
    check(quad.position().x == landed.x && quad.position().y == landed.y && quad.roll() == 0, "landed aircraft stationary");
    pilot.command_takeoff();
    pilot.run_cycle();
    pilot.command_stop();
    const double stopped_time = pilot.time();
    pilot.run_cycle();
    check(pilot.time() == stopped_time && pilot.mode() == "Stopped", "stop freezes model");

    Autopilot missing("Missing", nullptr, {&gyro}, &navigation, &controller);
    QSignalSpy missing_fault(&missing, &Autopilot::fault_detected);
    missing.resume();
    missing.run_cycle();
    check(missing_fault.size() == 1, "missing vehicle fails safely");
    Autopilot null_sensor("Null", &quad, {nullptr}, &navigation, &controller);
    null_sensor.resume();
    null_sensor.run_cycle();
    check(null_sensor.has_fault(), "null sensor fails safely");
    Autopilot no_controller("No controller", &quad, {&gyro}, &navigation, nullptr);
    no_controller.resume();
    no_controller.run_cycle();
    check(no_controller.has_fault(), "missing controller fails safely");
    PIDController invalid(std::numeric_limits<double>::quiet_NaN());
    check(!std::isfinite(invalid.calculate_control(1, 0.1))
        && !std::isfinite(controller.calculate_control(1, 0)), "invalid PID input rejected");
    check(wing.type() != quad.type() && wing.cruise_speed() > quad.cruise_speed(), "polymorphic aircraft");
    qInfo("All model checks passed");
}
