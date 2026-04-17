#ifndef MYCONTROLLEREX_H
#define MYCONTROLLEREX_H

extern "C" {
#include "uni.h"              // für uni_gamepad_t
}

#include <stdint.h>

class MyControllerEx {
public:
    static constexpr int BUTTON_COUNT = 14;
    static constexpr float TRIGGER_THRESHOLD = 0.1f;
    static constexpr int DEADZONE_RADIUS = 80;

    MyControllerEx();

    // neuen Gamepad-Status setzen (kopiert von Bluepad32)
    void update(const uni_gamepad_t& gp);

    // Hauptverarbeitung (State-Machine, Pins, etc.)
    void process();
    void processPrint();

    // Normalisierte Sticks
    void getLeftStick(float &nx, float &ny);
    void getRightStick(float &nx, float &ny);

    // Normalisierte Trigger
    float getL2();
    float getR2();

    // Events
    virtual void onPress(const char* name);
    virtual void onRelease(const char* name, float duration);
    virtual void onStick(const char* name, float x, float y);
    virtual void onTrigger(const char* name, float value, bool pressed);

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

    // vorherige Werte für Sticks/Trigger
    float prevLX = 0.0f;
    float prevLY = 0.0f;
    float prevRX = 0.0f;
    float prevRY = 0.0f;
    float prevL2 = 0.0f;
    float prevR2 = 0.0f;

    struct ButtonMapEntry {
        const char* name;
        int id;
    };

        // Button-Zustände
    ButtonMapEntry buttonMap[BUTTON_COUNT];
    bool buttonStates[BUTTON_COUNT];
    uint64_t buttonTimes[BUTTON_COUNT]; // µs

    // Deadzone & Normalisierung
    void applyDeadzone(int &x, int &y);
    float norm(int v, int maxv);

    bool isPressed(int id);
    void updateButton(int index, bool pressed);
};

#endif
