#include "mainwindow.h"
#include "altitudegraph.h"
#include "flightaudio.h"
#include "routeview.h"
#include "scene3d.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QPushButton* button(const QString& text, const char* name, QBoxLayout* layout)
{
    auto* result = new QPushButton(text);
    result->setObjectName(name);
    layout->addWidget(result);
    return result;
}
const std::array<QColor, 2> colors{QColor("#278bc4"), QColor("#8c56c4")};
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Практика №3 — Вариант 3 — Наземная станция БПЛА");
    setMinimumSize(1050, 760);
    resize(1400, 960);
    vehicles_[0] = std::make_unique<Quadcopter>();
    vehicles_[1] = std::make_unique<FixedWingUAV>();
    cycle_timer_ = new QTimer(this);
    cycle_timer_->setObjectName("cycleTimer");
    cycle_timer_->setInterval(100);
    cycle_timer_->setTimerType(Qt::PreciseTimer);
    audio_ = new FlightAudio(this);
    for (int i = 0; i < 2; ++i) {
        navigation_[i] = new NavigationManager(this);
        const std::vector<Sensor*> sensors{&gyroscopes_[i], &accelerometers_[i], &barometers_[i]};
        autopilots_[i] = new Autopilot(i == 0 ? "Quad-1" : "Wing-2", vehicles_[i].get(),
            sensors, navigation_[i], &controllers_[i], this);
        autopilots_[i]->setObjectName(QString("autopilot%1").arg(i));
    }

    auto* root = new QWidget;
    auto* layout = new QVBoxLayout(root);
    layout->addWidget(new QLabel("<b>Практика №3 · Вариант 3</b> — Наземная станция управления БПЛА"));
    auto* split = new QSplitter;
    split->setChildrenCollapsible(false);
    layout->addWidget(split, 1);
    auto* panel = new QWidget;
    auto* left = new QVBoxLayout(panel);
    auto* controls = new QGroupBox("Командный контур");
    auto* commands = new QVBoxLayout(controls);
    auto* run_buttons = new QHBoxLayout;
    auto* start_button = button("Старт · оба БПЛА", "btnStart", run_buttons);
    auto* pause_button = button("Пауза · оба", "btnPause", run_buttons);
    commands->addLayout(run_buttons);
    aircraft_ = new QComboBox;
    aircraft_->setObjectName("aircraftInput");
    aircraft_->addItems({"Quad-1 · Quadcopter", "Wing-2 · Fixed-wing UAV"});
    auto* form = new QFormLayout;
    form->addRow("Команды аппарату:", aircraft_);
    target_ = new QDoubleSpinBox;
    target_->setObjectName("spinTarget");
    target_->setRange(0, 500);
    target_->setSingleStep(10);
    target_->setSuffix(" м");
    target_->setValue(200);
    form->addRow("Целевая высота:", target_);
    commands->addLayout(form);
    auto* flight_buttons = new QHBoxLayout;
    auto* takeoff = button("Взлёт", "btnTakeoff", flight_buttons);
    auto* land = button("Посадка", "btnLand", flight_buttons);
    auto* stop = button("Стоп", "btnStop", flight_buttons);
    commands->addLayout(flight_buttons);
    auto* note = new QLabel("Взлёт: цель 200 м. Посадка: цель 0 м.\nСтоп фиксирует выбранный аппарат на месте.");
    note->setWordWrap(true);
    commands->addWidget(note);
    left->addWidget(controls);

    for (int i = 0; i < 2; ++i) {
        auto* group = new QGroupBox(autopilots_[i]->name());
        auto* telemetry_form = new QFormLayout(group);
        const QStringList titles{"Высота:", "Ошибка:", "Управление:"};
        const QStringList names{"labelAltitude", "labelError", "labelOutput"};
        for (int field = 0; field < 3; ++field) {
            telemetry_[i][field] = new QLabel("—");
            telemetry_[i][field]->setObjectName(names[field] + QString::number(i));
            telemetry_[i][field]->setStyleSheet("color: " + colors[i].name() + "; font-weight: bold;");
            telemetry_form->addRow(titles[field], telemetry_[i][field]);
        }
        modes_[i] = new QLabel("Stopped");
        modes_[i]->setObjectName(QString("labelMode%1").arg(i));
        telemetry_form->addRow("Режим:", modes_[i]);
        navigation_labels_[i] = new QLabel("Active system: GPS");
        navigation_labels_[i]->setObjectName(QString("labelNavigation%1").arg(i));
        telemetry_form->addRow("Навигация:", navigation_labels_[i]);
        left->addWidget(group);
    }
    auto* failures = new QGroupBox("Отказы выбранного аппарата");
    auto* failures_layout = new QVBoxLayout(failures);
    auto* fault_button = button("Отказ сенсора · Gyroscope", "btnFaultSensor", failures_layout);
    auto* gps_buttons = new QHBoxLayout;
    auto* gps_fault = button("Отказ GPS", "btnFaultGPS", gps_buttons);
    auto* gps_restore = button("Восстановить GPS", "btnRestoreGPS", gps_buttons);
    failures_layout->addLayout(gps_buttons);
    auto* reset_button = button("Сброс · восстановить и продолжить", "btnReset", failures_layout);
    left->addWidget(failures);
    status_ = new QLabel("Stopped");
    status_->setObjectName("labelStatus");
    status_->setWordWrap(true);
    left->addWidget(status_);
    auto* sound = new QCheckBox("Звук полёта и предупреждений");
    sound->setObjectName("soundEnabled");
    sound->setChecked(true);
    left->addWidget(sound);
    left->addStretch();
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(panel);
    scroll->setMinimumWidth(370);
    split->addWidget(scroll);

    auto* visual_panel = new QWidget;
    auto* right = new QVBoxLayout(visual_panel);
    fleet_ = new QTableWidget(2, 5);
    fleet_->setObjectName("tableFleet");
    fleet_->setHorizontalHeaderLabels({"Имя", "Тип", "Заряд", "Статус", "Навигация"});
    fleet_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fleet_->verticalHeader()->hide();
    fleet_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fleet_->setSelectionBehavior(QAbstractItemView::SelectRows);
    fleet_->setMaximumHeight(110);
    for (int i = 0; i < 2; ++i) {
        const QStringList values{autopilots_[i]->name(), vehicles_[i]->type(), "100.0 %", "Stopped", "GPS"};
        for (int column = 0; column < values.size(); ++column)
            fleet_->setItem(i, column, new QTableWidgetItem(values[column]));
        fleet_->item(i, 0)->setForeground(colors[i]);
    }
    right->addWidget(fleet_);
    auto* views = new QTabWidget;
    views->setObjectName("viewTabs");
    route_ = new RouteView;
    route_->setObjectName("routeView");
    views->addTab(route_, "Маршрут и крен");
    auto* scene_page = new QWidget;
    auto* scene_layout = new QVBoxLayout(scene_page);
    scene_ = new Scene3D;
    scene_layout->addWidget(scene_, 1);
    auto* camera = new QHBoxLayout;
    auto* zoom_in = button("Приблизить", "zoomInButton", camera);
    auto* zoom_out = button("Отдалить", "zoomOutButton", camera);
    auto* focus = button("К БПЛА", "focusButton", camera);
    auto* overview = button("Общий вид", "overviewButton", camera);
    scene_layout->addLayout(camera);
    views->addTab(scene_page, "3D-полёт");
    right->addWidget(views, 3);
    auto* plots = new QTabWidget;
    plots->setObjectName("graphTabs");
    altitude_graph_ = new AltitudeGraph("Высота, м");
    altitude_graph_->setObjectName("altitudeGraph");
    error_graph_ = new AltitudeGraph("Ошибка регулирования, м");
    error_graph_->setObjectName("errorGraph");
    plots->addTab(altitude_graph_, "Высота");
    plots->addTab(error_graph_, "Ошибка");
    right->addWidget(plots, 2);
    right->addWidget(new QLabel("Журнал событий · время работы модели"));
    log_ = new QListWidget;
    log_->setObjectName("eventLog");
    log_->setMinimumHeight(70);
    log_->setMaximumHeight(115);
    right->addWidget(log_);
    split->addWidget(visual_panel);
    split->setSizes({400, 970});
    setCentralWidget(root);

    for (int i = 0; i < 2; ++i) {
        auto* pilot = autopilots_[i];
        const Point3D position = vehicles_[i]->position();
        route_->add_aircraft(i, pilot->name(), vehicles_[i]->image_resource(), colors[i], {position.x, position.y});
        scene_->setDrone(pilot->name(), {position}, colors[i]);
        scene_->setDroneImage(pilot->name(), vehicles_[i]->image_resource());
        // Один timeout — два независимых получателя, один сигнал — три представления.
        connect(cycle_timer_, &QTimer::timeout, pilot, &Autopilot::run_cycle);
        connect(pilot, &Autopilot::telemetry_updated, this, &MainWindow::on_telemetry_updated);
        connect(pilot, &Autopilot::telemetry_updated, this, &MainWindow::update_fleet);
        connect(pilot, &Autopilot::telemetry_updated, this, &MainWindow::update_graph);
        connect(pilot, &Autopilot::position_updated, this, &MainWindow::update_position);
        connect(pilot, &Autopilot::fault_detected, this, &MainWindow::on_fault_detected);
        connect(pilot, &Autopilot::mode_changed, this, [this, i](const QString& mode) {
            modes_[i]->setText(mode);
            log(autopilots_[i]->name(), "Режим: " + mode);
            refresh_status();
        });
        connect(navigation_[i], &NavigationManager::active_system_changed, this, [this, i](const QString& name) {
            navigation_labels_[i]->setText("Active system: " + name);
            navigation_labels_[i]->setStyleSheet(name == "GPS" ? "color: #27946f;" : "color: #d07618; font-weight: bold;");
            fleet_->item(i, 4)->setText(name);
            log(autopilots_[i]->name(), "Навигация: " + name);
        });
    }
    // Выполняется после обоих модельных циклов.
    connect(cycle_timer_, &QTimer::timeout, this, &MainWindow::update_audio);
    connect(start_button, &QPushButton::clicked, this, &MainWindow::start);
    connect(pause_button, &QPushButton::clicked, this, &MainWindow::pause);
    connect(reset_button, &QPushButton::clicked, this, &MainWindow::reset);
    connect(takeoff, &QPushButton::clicked, this, [this] { execute_command(&Autopilot::command_takeoff); });
    connect(land, &QPushButton::clicked, this, [this] { execute_command(&Autopilot::command_land); });
    connect(stop, &QPushButton::clicked, this, [this] { execute_command(&Autopilot::command_stop); });
    connect(fault_button, &QPushButton::clicked, this, [this] {
        gyroscopes_[selected()].set_ready(false);
        log(autopilots_[selected()]->name(), "Gyroscope: Not ready (проверка на следующем цикле)");
    });
    connect(gps_fault, &QPushButton::clicked, this, [this] { navigation_[selected()]->set_gps_valid(false); });
    connect(gps_restore, &QPushButton::clicked, this, [this] { navigation_[selected()]->set_gps_valid(true); });
    connect(target_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double target) {
        auto* pilot = autopilots_[selected()];
        if (pilot->set_target_altitude(target))
            log(pilot->name(), QString("Целевая высота: %1 м").arg(target));
        refresh_target();
    });
    connect(aircraft_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this] {
        refresh_target();
        scene_->setSelected(autopilots_[selected()]->name());
    });
    connect(sound, &QCheckBox::toggled, audio_, &FlightAudio::set_enabled);
    connect(zoom_in, &QPushButton::clicked, this, [this] { scene_->zoom(0.8f); });
    connect(zoom_out, &QPushButton::clicked, this, [this] { scene_->zoom(1.25f); });
    connect(focus, &QPushButton::clicked, scene_, &Scene3D::focusDrone);
    connect(overview, &QPushButton::clicked, scene_, &Scene3D::resetCamera);
    scene_->setSelected(autopilots_[0]->name());
    refresh_target();
    refresh_status();
    log("Станция", "Готова. Два БПЛА · общий таймер 100 мс · цель 200 м");
}

MainWindow::~MainWindow()
{
    cycle_timer_->stop();
    audio_->stop();
    for (auto* pilot : autopilots_) delete pilot;
}
int MainWindow::selected() const { return aircraft_->currentIndex(); }
int MainWindow::index_of(const QString& name) const { return name == autopilots_[0]->name() ? 0 : 1; }
bool MainWindow::faulted() const { return autopilots_[0]->has_fault() || autopilots_[1]->has_fault(); }
void MainWindow::start()
{
    if (faulted()) return;
    for (auto* pilot : autopilots_) pilot->resume();
    cycle_timer_->start();
    refresh_status();
    log("Станция", "Running · 10 Гц");
}
void MainWindow::pause()
{
    cycle_timer_->stop();
    for (auto* pilot : autopilots_) pilot->pause();
    audio_->stop();
    refresh_status();
    log("Станция", "Paused");
}
void MainWindow::reset()
{
    for (int i = 0; i < 2; ++i) {
        gyroscopes_[i].set_ready(true);
        accelerometers_[i].set_ready(true);
        barometers_[i].set_ready(true);
        navigation_[i]->set_gps_valid(true);
        navigation_[i]->set_backups_valid(true);
        autopilots_[i]->reset_fault();
    }
    for (auto* dialog : findChildren<QMessageBox*>()) dialog->close();
    first_fault_shown_ = false;
    fault_message_.clear();
    log("Станция", "Сброс: сенсоры и навигация восстановлены");
    start();
}
void MainWindow::execute_command(void (Autopilot::*command)())
{
    if (faulted()) return;
    (autopilots_[selected()]->*command)();
    if (autopilots_[0]->is_running() || autopilots_[1]->is_running()) cycle_timer_->start();
    else cycle_timer_->stop();
    refresh_target();
    refresh_status();
    update_audio();
}
void MainWindow::refresh_target()
{
    const QSignalBlocker blocker(target_);
    target_->setValue(autopilots_[selected()]->target_altitude());
    const Point3D position = vehicles_[selected()]->position();
    scene_->setTarget(position.x, position.y, target_->value());
}
void MainWindow::refresh_status()
{
    for (int i = 0; i < 2; ++i) {
        auto* pilot = autopilots_[i];
        const QString state = pilot->has_fault() ? "FAULT" : pilot->mode();
        fleet_->item(i, 3)->setText(state);
        fleet_->item(i, 3)->setForeground(pilot->has_fault() ? QColor("#d43b3b") : colors[i]);
    }
    if (faulted()) {
        status_->setText("FAULT: " + fault_message_);
        status_->setStyleSheet("color: #d43b3b; font-weight: bold;");
    } else {
        const bool stopped = autopilots_[0]->mode() == "Stopped" && autopilots_[1]->mode() == "Stopped";
        const QString state = cycle_timer_->isActive() ? "Mission status: Active · Running" : stopped ? "Stopped" : "Paused";
        status_->setText(state + "\nQuad-1: " + autopilots_[0]->mode() + " · Wing-2: " + autopilots_[1]->mode());
        status_->setStyleSheet(cycle_timer_->isActive() ? "color: #27946f; font-weight: bold;" : "");
    }
}
void MainWindow::on_telemetry_updated(QString name, double altitude, double error, double control)
{
    const int i = index_of(name);
    telemetry_[i][0]->setText(QString::number(altitude, 'f', 1) + " м");
    telemetry_[i][1]->setText(QString::number(error, 'f', 1) + " м");
    telemetry_[i][2]->setText(QString::number(control, 'f', 2) + " м/с");
}
void MainWindow::update_fleet(QString name, double, double, double)
{
    const int i = index_of(name);
    const double battery = vehicles_[i]->battery();
    fleet_->item(i, 2)->setText(QString::number(battery, 'f', 1) + " %");
    fleet_->item(i, 2)->setForeground(QColor(battery >= 50 ? "#27946f" : battery >= 20 ? "#d07618" : "#d43b3b"));
    fleet_->item(i, 4)->setText(navigation_[i]->system_name());
}
void MainWindow::update_graph(QString name, double altitude, double error, double)
{
    const int i = index_of(name);
    altitude_graph_->add_sample(i, autopilots_[i]->time(), altitude);
    error_graph_->add_sample(i, autopilots_[i]->time(), error);
}
void MainWindow::update_position(QString name, double x, double y, double roll)
{
    const int i = index_of(name);
    route_->update_position(i, x, y, roll);
    const Point3D position = vehicles_[i]->position();
    scene_->setDronePosition(name, position);
    scene_->setDroneRoll(name, roll);
    // 3D-отрезки раз в секунду; положение и 2D-след обновляются на каждом тике.
    if (autopilots_[i]->time() - last_route_time_[i] >= 1) {
        scene_->appendRoutePoint(name, position);
        last_route_time_[i] = autopilots_[i]->time();
    }
}
void MainWindow::on_fault_detected(QString name, QString reason)
{
    fault_message_ = name + ": " + reason;
    cycle_timer_->stop();
    // Уже испускаемый timeout ещё вызовет второй слот: блокируем его модель явно.
    for (auto* pilot : autopilots_) pilot->pause();
    refresh_status();
    log(name, reason, true);
    audio_->alert();
    if (!first_fault_shown_) {
        first_fault_shown_ = true;
        auto* warning = new QMessageBox(QMessageBox::Warning, "Авария БПЛА", fault_message_, QMessageBox::Ok, this);
        warning->setObjectName("faultDialog");
        warning->setAttribute(Qt::WA_DeleteOnClose);
        warning->setModal(false);
        warning->show();
    }
}
void MainWindow::update_audio()
{
    if (faulted()) return;
    bool flying = false;
    for (int i = 0; i < 2; ++i)
        flying = flying || (autopilots_[i]->is_running() && vehicles_[i]->position().z > 0);
    if (flying && cycle_timer_->isActive()) audio_->start();
    else audio_->stop();
}
void MainWindow::log(const QString& name, const QString& message, bool error)
{
    const double time = qMax(autopilots_[0]->time(), autopilots_[1]->time());
    auto* item = new QListWidgetItem(QString("[%1 с] %2: %3").arg(time, 0, 'f', 1).arg(name, message), log_);
    if (error) item->setForeground(QColor("#d43b3b"));
    if (log_->count() > 600) delete log_->takeItem(0);
    log_->scrollToBottom();
}
