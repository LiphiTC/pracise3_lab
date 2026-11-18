#include "mainwindow.h"
#include "altitudegraph.h"
#include "routeview.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTableWidget>
#include <QTest>
#include <QTimer>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QFrameGraphNode>
#include <Qt3DRender/QNoDraw>
#include <Qt3DRender/QRenderCapture>
#include <cmath>

namespace {
void check(bool condition, const char* message) { if (!condition) qFatal("FAILED: %s", message); }
template<typename T> T* child(MainWindow& window, const char* name)
{
    auto* result = window.findChild<T*>(name);
    check(result, name);
    return result;
}
}
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MainWindow window;
    child<QCheckBox>(window, "soundEnabled")->setChecked(false);
    auto click = [&](const char* name) { child<QPushButton>(window, name)->click(); };
    auto* timer = child<QTimer>(window, "cycleTimer");
    auto* quad = child<Autopilot>(window, "autopilot0");
    auto* wing = child<Autopilot>(window, "autopilot1");
    auto* status = child<QLabel>(window, "labelStatus");
    auto* selection = child<QComboBox>(window, "aircraftInput");
    auto* target = child<QDoubleSpinBox>(window, "spinTarget");
    auto* fleet = child<QTableWidget>(window, "tableFleet");
    auto* route = static_cast<RouteView*>(child<QGraphicsView>(window, "routeView"));
    auto* graph = static_cast<AltitudeGraph*>(child<QChartView>(window, "altitudeGraph"));
    QSignalSpy quad_telemetry(quad, &Autopilot::telemetry_updated);
    QSignalSpy wing_telemetry(wing, &Autopilot::telemetry_updated);
    auto ticks = [&](int count) {
        for (int i = 0; i < count; ++i)
            check(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection), "timer timeout");
    };
    const QString screenshots = qEnvironmentVariable("GROUNDSTATION_SCREENSHOTS");
    auto screenshot = [&](const QString& name) {
        if (screenshots.isEmpty()) return;
        QDir().mkpath(screenshots);
        window.show();
        QApplication::processEvents();
        check(window.grab().save(screenshots + "/" + name + ".png"), "save screenshot");
    };
    check(QFile::exists(":/assets/drone_sound.mp3") && QFile::exists(":/assets/critical.mp3"), "audio embedded");
    check(!QPixmap(":/assets/drone.jpg").isNull() && !QPixmap(":/assets/fixedwing.svg").isNull(), "aircraft images embedded");
    check(fleet->rowCount() == 2 && fleet->item(0, 1)->text() != fleet->item(1, 1)->text(), "two polymorphic fleet rows");
    check(!timer->isActive() && status->text().startsWith("Stopped"), "initial stopped status");
    check(timer->interval() == 100, "10 Hz common timer");
    const auto* quad_marker = route->marker(0);
    const auto* quad_trail = route->trajectory(0);
    const QPointF initial = quad_marker->pos();
    click("btnStart");
    ticks(200);
    check(quad_telemetry.size() == 200 && wing_telemetry.size() == 200, "one timeout advances both pilots");
    check(graph->series(0)->count() == 200 && graph->series(1)->count() == 200, "both graph series update");
    check(child<QLabel>(window, "labelAltitude0")->text() != "—"
        && child<QLabel>(window, "labelAltitude1")->text() != "—", "both telemetry panels update");
    check(quad_marker == route->marker(0) && quad_trail == route->trajectory(0), "persistent marker and trajectory");
    check(quad_marker->pos() != initial && std::abs(quad_marker->rotation()) > 0.1
        && quad_trail->path().elementCount() > 100, "route motion, roll and growing trail");
    const auto first_point = quad_trail->path().elementAt(0);
    check(first_point.x == initial.x() && first_point.y == initial.y(), "trail begins at aircraft, not scene origin");
    screenshot("01_running");
    // Отдельный короткий проход с настоящим OpenGL, без offscreen.
    const QString capture_path = qEnvironmentVariable("GROUNDSTATION_3D_CAPTURE");
    if (!capture_path.isEmpty()) {
        click("btnPause");
        child<QTabWidget>(window, "viewTabs")->setCurrentIndex(1);
        window.show();
        Qt3DExtras::Qt3DWindow* view = nullptr;
        for (auto* candidate : QApplication::allWindows())
            if (auto* scene = qobject_cast<Qt3DExtras::Qt3DWindow*>(candidate)) view = scene;
        check(view, "3D view exists");
        Qt3DRender::QRenderCapture* capture = nullptr;
        for (auto* node : view->activeFrameGraph()->findChildren<Qt3DRender::QFrameGraphNode*>()) {
            if (!qobject_cast<Qt3DRender::QNoDraw*>(node)
                && node->findChildren<Qt3DRender::QFrameGraphNode*>(QString(), Qt::FindDirectChildrenOnly).isEmpty()) {
                capture = new Qt3DRender::QRenderCapture(node);
                break;
            }
        }
        check(capture, "3D capture branch");
        QTest::qWait(1500);
        auto* reply = capture->requestCapture();
        check(QTest::qWaitFor([reply] { return reply->isComplete(); }, 10000), "3D framebuffer rendered");
        check(reply->image().save(capture_path), "save 3D frame");
        delete reply;
        delete capture;
        window.close();
        qInfo("3D rendering check passed");
        return 0;
    }
    // Проверка реального QTimer, а не только прямого вызова timeout.
    const int before_wait = quad_telemetry.size();
    QTest::qWait(240);
    check(quad_telemetry.size() >= before_wait + 1 && quad_telemetry.size() <= before_wait + 4, "real timer delivers 10 Hz ticks");
    click("btnPause");
    const double paused = quad->time();
    QTest::qWait(150);
    check(!timer->isActive() && quad->time() == paused, "pause stops timer and telemetry");
    click("btnStart");
    ticks(1);
    check(quad->time() > paused, "start resumes existing model");

    click("btnFaultSensor");
    const int quad_before_fault = quad_telemetry.size();
    const int wing_before_fault = wing_telemetry.size();
    ticks(1);
    check(!timer->isActive() && status->text().contains("Quad-1: sensor failure: Gyroscope"), "red fault status with reason");
    check(status->styleSheet().contains("#d43b3b"), "fault color");
    check(quad_telemetry.size() == quad_before_fault && wing_telemetry.size() == wing_before_fault, "fault freezes both on same timeout");
    check(child<QListWidget>(window, "eventLog")->item(child<QListWidget>(window, "eventLog")->count() - 1)->text().contains("Gyroscope"), "fault logged");
    check(window.findChildren<QMessageBox*>().size() == 1, "first fault dialog");
    screenshot("02_sensor_fault");
    if (!screenshots.isEmpty())
        check(window.findChild<QMessageBox*>()->grab().save(screenshots + "/02_sensor_fault_dialog.png"), "save fault dialog");
    ticks(3);
    check(window.findChildren<QMessageBox*>().size() == 1, "no repeated dialog per tick");
    click("btnStart");
    check(!timer->isActive(), "start cannot bypass failure");
    click("btnReset");
    ticks(2);
    check(timer->isActive() && !quad->has_fault() && status->styleSheet().contains("#27946f"), "reset resumes with green status");
    screenshot("05_recovery");
    click("btnReset");
    check(timer->isActive() && !quad->has_fault(), "repeated reset remains safe");
    click("btnFaultGPS");
    ticks(1);
    auto* navigation = child<QLabel>(window, "labelNavigation0");
    check(timer->isActive() && navigation->text().contains("INS")
        && navigation->styleSheet().contains("#d07618"), "GPS fallback and amber indicator");
    screenshot("03_navigation_backup");
    click("btnRestoreGPS");
    ticks(1);
    check(navigation->text().contains("GPS"), "restore preferred navigation");
    target->setValue(300);
    ticks(1);
    check(quad_telemetry.last()[2].toDouble() > 90 && wing->target_altitude() == 200, "target command affects selected aircraft");
    screenshot("04_target_300");
    ticks(1600);
    check(std::abs(quad_telemetry.last()[1].toDouble() - 300) < 0.1, "GUI model converges to new target");
    check(graph->series(0)->count() == 600, "bounded chart history");
    screenshot("06_target_settled");
    child<QTabWidget>(window, "graphTabs")->setCurrentIndex(1);
    screenshot("07_error_settled");

    selection->setCurrentIndex(1);
    click("btnStop");
    const double wing_stopped = wing->time();
    const double quad_running = quad->time();
    ticks(5);
    check(wing->mode() == "Stopped" && wing->time() == wing_stopped
        && quad->time() > quad_running && timer->isActive(), "stop affects selected aircraft only");
    click("btnTakeoff");
    check(wing->target_altitude() == 200 && target->value() == 200 && wing->mode() == "Takeoff", "takeoff command and displayed target");
    click("btnLand");
    check(wing->target_altitude() == 0 && target->value() == 0 && wing->mode() == "Landing", "landing command and displayed target");
    ticks(1000);
    check(wing->mode() == "Landed" && wing_telemetry.last()[1].toDouble() == 0, "landing completes");
    click("btnStop");
    selection->setCurrentIndex(0);
    click("btnStop");
    check(!timer->isActive() && status->text().startsWith("Stopped"), "both stopped turns off timer");
    window.close();
    qInfo("All window checks passed");
}
