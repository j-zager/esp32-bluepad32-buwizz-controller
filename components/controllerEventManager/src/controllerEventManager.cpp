#include "controllerEventManager.h"
#include "esp_log.h"
#include "slot_helpers.h"
#include "MyControllerConfig.h"
#include "driver/gpio.h"

extern MyControllerConfig controllers[4];

static const char* TAG = "ControllerEventManager";

ControllerEventManager::ControllerEventManager() {
    queue = xQueueCreate(64, sizeof(ControllerEvent));
}

void ControllerEventManager::push(const ControllerEvent& ev) {
    if (queue)
        xQueueSend(queue, &ev, 0);
}

void ControllerEventManager::process() {
    ControllerEvent ev;

    while (xQueueReceive(queue, &ev, 0)) {

        switch (ev.type) {

        case EVENT_PRESS:
            ESP_LOGI(TAG, "[PRESS] slot=%d %s", ev.slot, ev.name);

            if (strcmp(ev.name, "up") == 0)
                controllers[ev.slot].setColor(0, 255, 0);

            if (strcmp(ev.name, "down") == 0)
                controllers[ev.slot].setColor(255, 0, 0);

            if (strcmp(ev.name, "circle") == 0)
                controllers[ev.slot].rumble(255, 0, 200);

            if (strcmp(ev.name, "left") == 0)
                gpio_set_level(GPIO_NUM_2, 1);

            if (strcmp(ev.name, "right") == 0)
                gpio_set_level(GPIO_NUM_2, 0);

            break;

        case EVENT_RELEASE:
            ESP_LOGI(TAG, "[RELEASE] slot=%d %s (%.2f)", ev.slot, ev.name, ev.duration);
            break;

        case EVENT_STICK:
            ESP_LOGI(TAG, "[STICK] slot=%d %s x=%.2f y=%.2f", ev.slot, ev.name, ev.x, ev.y);
            break;

        case EVENT_TRIGGER:
            ESP_LOGI(TAG, "[TRIGGER] slot=%d %s value=%.2f", ev.slot, ev.name, ev.value);

            if (strcmp(ev.name, "L2") == 0 && ev.value > 0.8f)
                controllers[ev.slot].rumble(255, 0, 100);

            break;

        case EVENT_LONGPRESS:
            ESP_LOGI(TAG, "[LONG] slot=%d %s (%.2f)", ev.slot, ev.name, ev.duration);

            controllers[ev.slot].rumble(0, 255, 150);

            if (strcmp(ev.name, "options") == 0 && ev.duration > 4.0f)
                controllers[ev.slot].startGyroCalibration();

            break;

        case EVENT_DOUBLEPRESS:
            ESP_LOGI(TAG, "[DOUBLE] slot=%d %s", ev.slot, ev.name);
            controllers[ev.slot].setColor(255, 255, 0);
            break;
        case EVENT_ACCEL:
            break;
        case EVENT_GYRO:
            break;
        default:
            break;
        }
    }
}
