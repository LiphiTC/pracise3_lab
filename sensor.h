#pragma once
#include <QString>
#include <vector>

class Sensor {
public:
    virtual double read() const = 0;
    virtual QString name() const = 0;
    virtual ~Sensor() = default;
    bool is_ready() const;
    void set_ready(bool ready);
private:
    bool ready_ = true;
};
class Gyroscope final : public Sensor {
public:
    double read() const override;
    QString name() const override;
};
class Accelerometer final : public Sensor {
public:
    double read() const override;
    QString name() const override;
};
class Barometer final : public Sensor {
public:
    double read() const override;
    QString name() const override;
};
