#ifndef MYCONTROLLEREX_H
#define MYCONTROLLEREX_H

extern "C" {
#include "uni.h"              // für uni_gamepad_t
}

#include <stdint.h>

class MyControllerEx {
public:
    static constexpr int BUTTON_COUNT = 16;
    static constexpr float TRIGGER_THRESHOLD = 0.1f;
    static constexpr int DEADZONE_RADIUS = 80;
    static constexpr float LONG_PRESS_THRESHOLD = 0.5f;     // Sekunden
    static constexpr float DOUBLE_PRESS_THRESHOLD = 0.25f;  // Sekunden

    static constexpr float GYRO_DEADZONE = 6.0f;


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

    void getGyro(float& gx, float& gy, float& gz);
    void getAccel(float& ax, float& ay, float& az);

    // Events
    virtual void onPress(const char* name);
    virtual void onRelease(const char* name, float duration);
    virtual void onStick(const char* name, float x, float y);
    virtual void onTrigger(const char* name, float value, bool pressed);

    virtual void onGyro(float gx, float gy, float gz);
    virtual void onAccel(float ax, float ay, float az);
    virtual void onLongPress(const char* name, float duration);
    virtual void onDoublePress(const char* name);

    void processGyro(uint64_t now);
    void processAccel(uint64_t now);

    void startGyroCalibration();
    bool updateGyroCalibration(float gx, float gy, float gz, uint64_t now);


    // Debug-Ausgabe
    void printSticks();
    void printTriggers();

protected:
    uni_gamepad_t gamepad{};
    bool hasGamepad = false;

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

    float prevGX = 0, prevGY = 0, prevGZ = 0;
    float prevAX = 0, prevAY = 0, prevAZ = 0;

    float filteredGX = 0, filteredGY = 0, filteredGZ = 0;
    float filteredAX = 0, filteredAY = 0, filteredAZ = 0;

    bool gyroCalibrated = false;
    bool gyroCalibrating = false;

    //static constexpr int GYRO_CALIB_SAMPLES = 500;
    uint64_t gyroCalibStart = 0;
    int gyroCalibCount = 0;

    float gyroBiasX = 0;
    float gyroBiasY = 0;
    float gyroBiasZ = 0;

    float gyroSumX = 0;
    float gyroSumY = 0;
    float gyroSumZ = 0;



    struct ButtonMapEntry {
        const char* name;
        int id;
    };

        // Button-Zustände
    ButtonMapEntry buttonMap[BUTTON_COUNT];
    bool buttonStates[BUTTON_COUNT];
    uint64_t buttonTimes[BUTTON_COUNT]; // µs
    // --- für Double‑Press ---
    uint64_t lastPressTime[BUTTON_COUNT];

    // Deadzone & Normalisierung
    void applyDeadzone(int &x, int &y);
    float norm(int v, int maxv);

    bool isPressed(int id);
    void updateButton(int index, bool pressed);
};

#endif
