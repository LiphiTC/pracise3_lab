#include "navigation.h"
#include <array>
#include <cmath>

void NavigationSystem::set_valid(bool valid) { valid_ = valid; }
bool NavigationSystem::valid() const { return valid_; }
bool GPSNavigation::is_valid() const { return valid(); }
QString GPSNavigation::system_name() const { return "GPS"; }
bool InertialNavigation::is_valid() const { return valid(); }
QString InertialNavigation::system_name() const { return "INS"; }
bool VisualNavigation::is_valid() const { return valid(); }
QString VisualNavigation::system_name() const { return "Visual"; }
NavigationManager::NavigationManager(QObject* parent) : QObject(parent) {}
void NavigationManager::set_gps_valid(bool valid) { gps_.set_valid(valid); }
void NavigationManager::set_backups_valid(bool valid)
{
    inertial_.set_valid(valid);
    visual_.set_valid(valid);
}
Point3D NavigationManager::get_position() const { return position_; }
QString NavigationManager::system_name() const { return active_ ? active_->system_name() : "Unavailable"; }
bool NavigationManager::update(const Point3D& position)
{
    NavigationSystem* next = nullptr;
    const std::array<NavigationSystem*, 3> systems{&gps_, &inertial_, &visual_};
    for (NavigationSystem* system : systems) {
        if (system->is_valid()) { next = system; break; }
    }
    if (next != active_) {
        active_ = next;
        emit active_system_changed(system_name());
    }
    if (!active_ || !std::isfinite(position.x) || !std::isfinite(position.y)
        || !std::isfinite(position.z) || position.z < 0)
        return false;
    position_ = position;
    return true;
}
