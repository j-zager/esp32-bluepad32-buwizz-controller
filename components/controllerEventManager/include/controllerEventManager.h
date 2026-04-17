#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern "C" {
#include "uni.h"
}

enum EventType {
    EVENT_PRESS,
    EVENT_RELEASE,
    EVENT_STICK,
    EVENT_TRIGGER,
    EVENT_GYRO,
    EVENT_ACCEL,
    EVENT_LONGPRESS,
    EVENT_DOUBLEPRESS,
};

struct ControllerEvent {
    int slot;
    EventType type;

    const char* name = nullptr;
    float x = 0, y = 0;
    float value = 0;
    float duration = 0;
    float gx = 0, gy = 0, gz = 0;
    float ax = 0, ay = 0, az = 0;
};

class ControllerEventManager {
public:
    ControllerEventManager();
    void push(const ControllerEvent& ev);
    void process();

private:
    QueueHandle_t queue;
};
