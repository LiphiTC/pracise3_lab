#pragma once
#include "autopilot.h"
#include <QMainWindow>
#include <array>
#include <memory>

class QLabel;
class QComboBox;
class QDoubleSpinBox;
class QListWidget;
class QPushButton;
class QTableWidget;
class QTimer;
class AltitudeGraph;
class RouteView;
class Scene3D;
class FlightAudio;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
private slots:
    void start();
    void pause();
    void reset();
    void on_telemetry_updated(QString name, double altitude, double error, double control);
    void update_fleet(QString name, double altitude, double error, double control);
    void update_graph(QString name, double altitude, double error, double control);
    void update_position(QString name, double x, double y, double roll);
    void on_fault_detected(QString name, QString reason);
private:
    int index_of(const QString& name) const;
    int selected() const;
    void refresh_status();
    void refresh_target();
    void update_audio();
    void log(const QString& name, const QString& message, bool error = false);
    void execute_command(void (Autopilot::*command)());
    bool faulted() const;

    std::array<std::unique_ptr<UAV>, 2> vehicles_;
    std::array<Gyroscope, 2> gyroscopes_;
    std::array<Accelerometer, 2> accelerometers_;
    std::array<Barometer, 2> barometers_;
    std::array<PIDController, 2> controllers_{PIDController(), PIDController(0.35, 0.01, 0.04)};
    std::array<NavigationManager*, 2> navigation_{};
    std::array<Autopilot*, 2> autopilots_{};
    std::array<std::array<QLabel*, 3>, 2> telemetry_{};
    std::array<QLabel*, 2> modes_{};
    std::array<QLabel*, 2> navigation_labels_{};
    std::array<double, 2> last_route_time_{};
    QComboBox* aircraft_;
    QDoubleSpinBox* target_;
    QLabel* status_;
    QTableWidget* fleet_;
    QListWidget* log_;
    QTimer* cycle_timer_;
    AltitudeGraph* altitude_graph_;
    AltitudeGraph* error_graph_;
    RouteView* route_;
    Scene3D* scene_;
    FlightAudio* audio_;
    bool first_fault_shown_ = false;
    QString fault_message_;
};
