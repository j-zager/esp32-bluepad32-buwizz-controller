#ifndef MYCONTROLLERCONFIG_H
#define MYCONTROLLERCONFIG_H

extern "C" {
#include "uni.h"
}

#include "MyControllerEx.h"

class MyControllerConfig : public MyControllerEx {
public:
    MyControllerConfig();

    void setGamepad(const uni_gamepad_t* gp, uni_hid_device_t* dev, int bat);
    void setDevice(uni_hid_device_t* dev);
    void reset();
    bool hasGpActive() const;

    void update(const uni_gamepad_t& gp, uni_hid_device_t* dev, int battery);

    void onPress(const char* name) override;
    void onRelease(const char* name, float duration) override;

    void onStick(const char* name, float x, float y) override;
    void onTrigger(const char* name, float value, bool pressed) override;

    void onGyro(float gx, float gy, float gz);
    void onAccel(float ax, float ay, float az);

    void onLongPress(const char* name, float duration);
    void onDoublePress(const char* name);

    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void rumble(uint8_t weak, uint8_t strong, uint16_t duration_ms);
    
    int getBattery() const;
    void updateBatteryLED(uint64_t now);

    static bool isAnyControllerConnected(const MyControllerConfig* array, int size);
   
private:
    uni_hid_device_t* device = nullptr;
     int battery = -1;   // 0–100
     uint64_t lastBatteryUpdate = 0;
};

#endif
