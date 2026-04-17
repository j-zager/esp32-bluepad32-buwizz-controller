#include "MyControllerEx.h"

extern "C" {
#include "esp_timer.h"
#include "esp_log.h"
}

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
    }
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

    if (pressed && !prev) {
        buttonStates[index] = true;
        buttonTimes[index] = esp_timer_get_time();
        onPress(buttonMap[index].name);
    } else if (!pressed && prev) {
        buttonStates[index] = false;
        uint64_t now = esp_timer_get_time();
        float duration = (now - buttonTimes[index]) / 1e6f;
        onRelease(buttonMap[index].name, duration);
    }
}

void MyControllerEx::process() {
    if (!hasGamepad)
        return;

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
