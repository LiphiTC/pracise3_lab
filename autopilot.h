#pragma once

#include "controller.h"
#include "navigation.h"
#include "sensor.h"
#include "uav.h"
#include <QObject>

class Autopilot : public QObject {
    Q_OBJECT
public:
    // Модули принадлежат вызывающему коду и живут дольше автопилота.
    Autopilot(QString name, UAV* vehicle, const std::vector<Sensor*>& sensors,
              NavigationManager* navigation, FlightController* controller,
              QObject* parent = nullptr);
    QString name() const;
    QString mode() const;
    double time() const;
    double target_altitude() const;
    bool has_fault() const;
    bool is_running() const;
    bool set_target_altitude(double altitude);
    void resume();
    void pause();
    void reset_fault();
    static constexpr double time_step = 0.1;
public slots:
    void run_cycle();
    void command_takeoff();
    void command_land();
    void command_stop();
signals:
    void telemetry_updated(QString name, double altitude, double error, double control);
    void position_updated(QString name, double x, double y, double roll);
    void fault_detected(QString name, QString reason);
    void mode_changed(QString mode);
private:
    enum class Mode { Stopped, Takeoff, Mission, Landing, Landed };
    void set_mode(Mode mode);
    void fail(const QString& reason);
    QString name_;
    UAV* vehicle_;
    std::vector<Sensor*> sensors_;
    NavigationManager* navigation_;
    FlightController* controller_;
    Mode mode_ = Mode::Stopped;
    double target_altitude_ = 200;
    double time_ = 0;
    bool running_ = false;
    bool fault_ = false;
};
