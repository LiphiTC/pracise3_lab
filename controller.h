#pragma once
#include <QString>

class FlightController {
public:
    virtual double calculate_control(double error, double dt) = 0;
    virtual QString controller_name() const = 0;
    virtual void reset() = 0;
    virtual ~FlightController() = default;
};
class PIDController final : public FlightController {
public:
    PIDController(double kp = 0.5, double ki = 0.015, double kd = 0.05);
    double calculate_control(double error, double dt) override;
    QString controller_name() const override;
    void reset() override;
private:
    double kp_, ki_, kd_;
    double integral_ = 0;
    double previous_ = 0;
    bool first_ = true;
};
