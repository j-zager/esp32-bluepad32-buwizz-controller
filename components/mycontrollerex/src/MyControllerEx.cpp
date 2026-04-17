#include "MyControllerEx.h"

extern "C" {
#include "esp_timer.h"
#include "esp_log.h"
}
#include <cmath>


static const char* TAG = "MyControllerEx";

MyControllerEx::MyControllerEx() {
    buttonMap[0]  = {"cross",     0};
    buttonMap[1]  = {"circle",    1};
    buttonMap[2]  = {"square",    2};
    buttonMap[3]  = {"triangle",  3};

    buttonMap[4]  = {"l1",        4};
    buttonMap[5]  = {"r1",        5};

    buttonMap[6]  = {"l3",        6};
    buttonMap[7]  = {"r3",        7};

    // --- MISC Buttons ---
    buttonMap[8]  = {"options",   8};   // START
    buttonMap[9]  = {"share",     9};   // SELECT
    buttonMap[10] = {"ps",        10};  // SYSTEM
    buttonMap[11] = {"touchpad",  11};  // CAPTURE

    // --- D-Pad ---
    buttonMap[12] = {"up",        12};
    buttonMap[13] = {"down",      13};
    buttonMap[14] = {"left",      14};
    buttonMap[15] = {"right",     15};


    for (int i = 0; i < BUTTON_COUNT; i++) {
        buttonStates[i] = false;
        buttonTimes[i] = 0;
        lastPressTime[i] = 0;
    }

    prevLX = 0.0f;
    prevLY = 0.0f;
    prevRX = 0.0f;
    prevRY = 0.0f;
    prevL2 = 0.0f;
    prevR2 = 0.0f;

    prevGX = prevGY = prevGZ = 0;
    prevAX = prevAY = prevAZ = 0;

    filteredGX = filteredGY = filteredGZ = 0;
    filteredAX = filteredAY = filteredAZ = 0;

}

void MyControllerEx::update(const uni_gamepad_t& gp) {
    gamepad = gp;
    hasGamepad = true;
}

void MyControllerEx::applyDeadzone(int &x, int &y) {
    long mag = (long)x * x + (long)y * y;
    if (mag < (long)DEADZONE_RADIUS * DEADZONE_RADIUS) {
        x = 0;
        y = 0;
    }
}

float MyControllerEx::norm(int v, int maxv) {
    return (float)v / (float)maxv;
}

void MyControllerEx::getLeftStick(float &nx, float &ny) {
    if (!hasGamepad) { nx = ny = 0; return; }
    int x = gamepad.axis_x;
    int y = gamepad.axis_y;
    applyDeadzone(x, y);
    nx = norm(x, 512);
    ny = norm(y, 512);
}

void MyControllerEx::getRightStick(float &nx, float &ny) {
    if (!hasGamepad) { nx = ny = 0; return; }
    int x = gamepad.axis_rx;
    int y = gamepad.axis_ry;
    applyDeadzone(x, y);
    nx = norm(x, 512);
    ny = norm(y, 512);
}

float MyControllerEx::getL2() {
    if (!hasGamepad) return 0;
    return norm(gamepad.brake, 1023);
}

float MyControllerEx::getR2() {
    if (!hasGamepad) return 0;
    return norm(gamepad.throttle, 1023);
}

void MyControllerEx::getGyro(float& gx, float& gy, float& gz) {
    if (!hasGamepad) { gx = gy = gz = 0; return; }
    gx = gamepad.gyro[0];
    gy = gamepad.gyro[1];
    gz = gamepad.gyro[2];
}

void MyControllerEx::getAccel(float& ax, float& ay, float& az) {
    if (!hasGamepad) { ax = ay = az = 0; return; }
    ax = gamepad.accel[0];
    ay = gamepad.accel[1];
    az = gamepad.accel[2];
}

void MyControllerEx::processGyro(uint64_t now) {
    float gx_raw, gy_raw, gz_raw;
    getGyro(gx_raw, gy_raw, gz_raw);

    // 1. Skalieren (PS4: 16.4 LSB = 1 °/s)
    float gx = gx_raw / 16.4f;
    float gy = gy_raw / 16.4f;
    float gz = gz_raw / 16.4f;

    // 2. Kalibrierung läuft?
    if (gyroCalibrating) {
        updateGyroCalibration(gx, gy, gz);
        return; // während Kalibrierung keine Events
    }

    // 3. Noch nicht kalibriert → ignorieren
    if (!gyroCalibrated)
        return;

    // 4. Bias abziehen
    gx -= gyroBiasX;
    gy -= gyroBiasY;
    gz -= gyroBiasZ;

    // 5. Deadzone
    if (fabs(gx) < 1) gx = 0;
    if (fabs(gy) < 1) gy = 0;
    if (fabs(gz) < 1) gz = 0;

    // 6. Low‑Pass
    filteredGX = filteredGX * 0.9f + gx * 0.1f;
    filteredGY = filteredGY * 0.9f + gy * 0.1f;
    filteredGZ = filteredGZ * 0.9f + gz * 0.1f;

    // 7. Threshold
    if (fabs(filteredGX - prevGX) < 0.5f &&
        fabs(filteredGY - prevGY) < 0.5f &&
        fabs(filteredGZ - prevGZ) < 0.5f)
        return;

    // 8. Rate‑Limit
    static uint64_t lastEvent = 0;
    if (now - lastEvent < 20000)
        return;

    lastEvent = now;

    // 9. Event
    onGyro(filteredGX, filteredGY, filteredGZ);

    prevGX = filteredGX;
    prevGY = filteredGY;
    prevGZ = filteredGZ;
}


void MyControllerEx::processAccel(uint64_t now) {
    float ax_raw, ay_raw, az_raw;
    getAccel(ax_raw, ay_raw, az_raw);

    // 1. Skalieren (PS4: 8192 LSB = 1g)
    float ax = ax_raw / 8192.0f;
    float ay = ay_raw / 8192.0f;
    float az = az_raw / 8192.0f;

    // 2. Deadzone (kleine Bewegungen ignorieren)
    if (fabs(ax) < 0.02f) ax = 0;
    if (fabs(ay) < 0.02f) ay = 0;
    if (fabs(az - 1.0f) < 0.02f) az = 1.0f;   // Ruheposition (1g nach unten)

    // 3. Low‑Pass‑Filter (Glättung)
    filteredAX = filteredAX * 0.9f + ax * 0.1f;
    filteredAY = filteredAY * 0.9f + ay * 0.1f;
    filteredAZ = filteredAZ * 0.9f + az * 0.1f;

    // 4. Threshold (nur echte Änderungen melden)
    if (fabs(filteredAX - prevAX) < 0.01f &&
        fabs(filteredAY - prevAY) < 0.01f &&
        fabs(filteredAZ - prevAZ) < 0.01f)
        return;

    // 5. Rate‑Limit (max. 50 Events/s)
    static uint64_t lastEvent = 0;
    if (now - lastEvent < 20000)   // 20 ms
        return;

    lastEvent = now;

    // 6. Event auslösen
    onAccel(filteredAX, filteredAY, filteredAZ);

    // 7. prev‑Werte aktualisieren
    prevAX = filteredAX;
    prevAY = filteredAY;
    prevAZ = filteredAZ;
}

void MyControllerEx::startGyroCalibration() {
    gyroCalibrating = true;
    gyroCalibrated = false;

    gyroCalibCount = 0;
    gyroSumX = gyroSumY = gyroSumZ = 0;
}

bool MyControllerEx::updateGyroCalibration(float gx, float gy, float gz) {
    if (!gyroCalibrating)
        return false;

    gyroSumX += gx;
    gyroSumY += gy;
    gyroSumZ += gz;
    gyroCalibCount++;

    if (gyroCalibCount >= GYRO_CALIB_SAMPLES) {
        gyroBiasX = gyroSumX / gyroCalibCount;
        gyroBiasY = gyroSumY / gyroCalibCount;
        gyroBiasZ = gyroSumZ / gyroCalibCount;

        gyroCalibrating = false;
        gyroCalibrated = true;
        return true;
    }

    return false;
}



bool MyControllerEx::isPressed(int id) {
    if (!hasGamepad) return false;

    switch (id) {

        // --- Face Buttons ---
        case 0:  return gamepad.buttons & BUTTON_A;  // cross
        case 1:  return gamepad.buttons & BUTTON_B;  // circle
        case 2:  return gamepad.buttons & BUTTON_X;  // square
        case 3:  return gamepad.buttons & BUTTON_Y;  // triangle

        // --- Shoulder Buttons ---
        case 4:  return gamepad.buttons & BUTTON_SHOULDER_L; // L1
        case 5:  return gamepad.buttons & BUTTON_SHOULDER_R; // R1

        // --- Stick Buttons ---
        case 6:  return gamepad.buttons & BUTTON_THUMB_L; // L3
        case 7:  return gamepad.buttons & BUTTON_THUMB_R; // R3

        // --- Misc Buttons ---
        case 8:  return gamepad.misc_buttons & MISC_BUTTON_START;   // options
        case 9:  return gamepad.misc_buttons & MISC_BUTTON_SELECT;  // share
        case 10: return gamepad.misc_buttons & MISC_BUTTON_SYSTEM;  // ps
        case 11: return gamepad.misc_buttons & MISC_BUTTON_CAPTURE; // touchpad

        // --- D-Pad ---
        case 12: return gamepad.dpad == DPAD_UP;
        case 13: return gamepad.dpad == DPAD_DOWN;
        case 14: return gamepad.dpad == DPAD_LEFT;
        case 15: return gamepad.dpad == DPAD_RIGHT;
    }

    return false;
}


void MyControllerEx::updateButton(int index, bool pressed) {
    bool prev = buttonStates[index];
    uint64_t now = esp_timer_get_time();

    if (pressed && !prev) {
        buttonStates[index] = true;
        buttonTimes[index] = now;

        if (now - lastPressTime[index] < DOUBLE_PRESS_THRESHOLD * 1e6f) {
            onDoublePress(buttonMap[index].name);
        }

        lastPressTime[index] = now;
        onPress(buttonMap[index].name);
        return;
    }

    if (!pressed && prev) {
        buttonStates[index] = false;

        float duration = (now - buttonTimes[index]) / 1e6f;

        if (duration >= LONG_PRESS_THRESHOLD) {
            onLongPress(buttonMap[index].name, duration);
        } else {
            onRelease(buttonMap[index].name, duration);
        }
    }
}


// void MyControllerEx::updateButton(int index, bool pressed) {
//     bool prev = buttonStates[index];

//     if (pressed && !prev) {
//         buttonStates[index] = true;
//         buttonTimes[index] = esp_timer_get_time();
//         onPress(buttonMap[index].name);
//     } else if (!pressed && prev) {
//         buttonStates[index] = false;
//         uint64_t now = esp_timer_get_time();
//         float duration = (now - buttonTimes[index]) / 1e6f;
//         onRelease(buttonMap[index].name, duration);
//     }
// }


void MyControllerEx::process() {
    if (!hasGamepad)
        return;

    // Zeitstempel EINMAL berechnen
    uint64_t now = esp_timer_get_time();

    // --- Buttons ---
    for (int i = 0; i < BUTTON_COUNT; i++) {
        bool pressed = isPressed(buttonMap[i].id);
        updateButton(i, pressed);
    }

    // --- Left Stick ---
    float nlx, nly;
    getLeftStick(nlx, nly);

    if (nlx != prevLX || nly != prevLY) {
        onStick("left", nlx, nly);
        prevLX = nlx;
        prevLY = nly;
    }

    // --- Right Stick ---
    float nrx, nry;
    getRightStick(nrx, nry);

    if (nrx != prevRX || nry != prevRY) {
        onStick("right", nrx, nry);
        prevRX = nrx;
        prevRY = nry;
    }

    // --- Trigger L2 ---
    float nL2 = getL2();
    if (nL2 != prevL2) {
        onTrigger("L2", nL2, nL2 > TRIGGER_THRESHOLD);
        prevL2 = nL2;
    }

    // --- Trigger R2 ---
    float nR2 = getR2();
    if (nR2 != prevR2) {
        onTrigger("R2", nR2, nR2 > TRIGGER_THRESHOLD);
        prevR2 = nR2;
    }

    processGyro(now);
    processAccel(now);

}



void MyControllerEx::processPrint() {
    if (!hasGamepad)
        return;

    // Rohwerte
    lx = gamepad.axis_x;
    ly = gamepad.axis_y;
    rx = gamepad.axis_rx;
    ry = gamepad.axis_ry;
    l2 = gamepad.brake;
    r2 = gamepad.throttle;

    for (int i = 0; i < BUTTON_COUNT; i++) {
        bool pressed = isPressed(buttonMap[i].id);
        updateButton(i, pressed);
    }

    printSticks();
    printTriggers();
}

void MyControllerEx::printSticks() {
    float lxN, lyN, rxN, ryN;
    getLeftStick(lxN, lyN);
    getRightStick(rxN, ryN);

    if (lxN != 0 || lyN != 0 || rxN != 0 || ryN != 0) {
        ESP_LOGI(TAG, "LX: %.2f  LY: %.2f  |  RX: %.2f  RY: %.2f",
                 lxN, lyN, rxN, ryN);
    }
}

void MyControllerEx::printTriggers() {
    float nl2 = getL2();
    float nr2 = getR2();

    if (nl2 > 0) ESP_LOGI(TAG, "L2: %.2f", nl2);
    if (nr2 > 0) ESP_LOGI(TAG, "R2: %.2f", nr2);
}



void MyControllerEx::onPress(const char* name) {
    ESP_LOGI(TAG, "[PRESS] %s", name);
}

void MyControllerEx::onRelease(const char* name, float duration) {
    ESP_LOGI(TAG, "[RELEASE] %s (%.2fs)", name, duration);
}

void MyControllerEx::onStick(const char* name, float x, float y) {
    // default: nichts
}

void MyControllerEx::onTrigger(const char* name, float value, bool pressed) {
    // default: nichts
}

void MyControllerEx::onGyro(float gx, float gy, float gz) {
    // default: nothing
}

void MyControllerEx::onAccel(float ax, float ay, float az) {
    // default: nothing
}

void MyControllerEx::onLongPress(const char* name, float duration) {
    // default: nothing
}

void MyControllerEx::onDoublePress(const char* name) {
    // default: nothing
}
