#ifndef MYCONTROLLERCONFIG_H
#define MYCONTROLLERCONFIG_H

extern "C" {
#include "uni.h"
}

#include "MyControllerEx.h"

class MyControllerConfig : public MyControllerEx {
public:
    MyControllerConfig();

    void update(const uni_gamepad_t& gp, uni_hid_device_t* dev, int battery);

    void onPress(const char* name) override;
    void onRelease(const char* name, float duration) override;

    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void rumble(uint8_t weak, uint8_t strong, uint16_t duration_ms);
    
    int getBattery() const;
    void updateBatteryLED(uint64_t now);


private:
    uni_hid_device_t* device = nullptr;
     int battery = -1;   // 0–100
};

#endif
