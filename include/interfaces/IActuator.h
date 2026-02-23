#ifndef I_ACTUATOR_H
#define I_ACTUATOR_H

/**
 * @brief Interface มาตรฐานสำหรับอุปกรณ์สั่งการ (Output)
 * ฉบับปรับปรุงตามมาตรฐาน Const Correctness
 */
class IActuator {
public:
    virtual ~IActuator() {}

    virtual bool begin() = 0;
    virtual void turnOn() = 0;
    virtual void turnOff() = 0;

    // เติม const เพื่อบอกว่าฟังก์ชันนี้ "อ่านอย่างเดียว"
    virtual bool isOn() const = 0;
    virtual const char* getName() const = 0;
};

#endif