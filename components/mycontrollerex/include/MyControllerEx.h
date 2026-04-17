#ifndef MYCONTROLLEREX_H
#define MYCONTROLLEREX_H

extern "C" {
#include "uni.h"              // für uni_gamepad_t
}

#include <stdint.h>

class MyControllerEx {
public:
    MyControllerEx();

    // neuen Gamepad-Status setzen (kopiert von Bluepad32)
    void update(const uni_gamepad_t& gp);

    // Hauptverarbeitung (State-Machine, Pins, etc.)
    void process();

    // Normalisierte Sticks
    void getLeftStick(float &nx, float &ny);
    void getRightStick(float &nx, float &ny);

    // Normalisierte Trigger
    float getL2();
    float getR2();

    // Events
    virtual void onPress(const char* name);
    virtual void onRelease(const char* name, float duration);

    // Debug-Ausgabe
    void printSticks();
    void printTriggers();

protected:
    uni_gamepad_t gamepad{};
    bool hasGamepad = false;
    bool connected = false;

    // Rohwerte
    int lx = 0, ly = 0;
    int rx = 0, ry = 0;
    int l2 = 0, r2 = 0;

    // Deadzone & Normalisierung
    void applyDeadzone(int &x, int &y);
    float norm(int v, int maxv);

    struct ButtonMapEntry {
        const char* name;
        int id;
    };

    ButtonMapEntry buttonMap[14];
    bool buttonStates[14];
    uint64_t buttonTimes[14];   // µs

    bool isPressed(int id);
    void updateButton(int index, bool pressed);
};

#endif
