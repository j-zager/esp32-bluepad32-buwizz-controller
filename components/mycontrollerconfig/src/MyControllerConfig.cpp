#include "MyControllerConfig.h"

extern "C" {
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
}

static const char* TAG = "MyControllerConfig";

MyControllerConfig::MyControllerConfig() {}

void MyControllerConfig::update(const uni_gamepad_t& gp, uni_hid_device_t* dev, int bat) {
    MyControllerEx::update(gp);
    device = dev;
    battery = bat;   // 0–100
}


void MyControllerConfig::setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!device) return;
    if (device->report_parser.set_lightbar_color)
        device->report_parser.set_lightbar_color(device, r, g, b);
}

void MyControllerConfig::rumble(uint8_t weak, uint8_t strong, uint16_t duration_ms) {
    if (!device) return;
    if (device->report_parser.play_dual_rumble)
        device->report_parser.play_dual_rumble(device, 0, duration_ms, weak, strong);
}

int MyControllerConfig::getBattery() const {
    return battery;   // 0–100 %
}

void MyControllerConfig::updateBatteryLED(uint64_t now) {
    static uint64_t lastUpdate = 0;
    if (now - lastUpdate < 5000000) return; // alle 5s
    lastUpdate = now;

    int bat = getBattery();
    if (bat < 0) return;

    // --- Kritische Stufen: Blinken ---
    if (bat < 5) {
        bool on = ((now / 250000) % 2) == 0;   // 2 Hz
        setColor(on ? 255 : 0, 0, 0);
        return;
    }

    if (bat < 10) {
        bool on = ((now / 500000) % 2) == 0;   // 1 Hz
        setColor(on ? 255 : 0, 0, 0);
        return;
    }

    // --- Sony-Stufen ---
    if (bat >= 75) {
        setColor(0, 0, 255);     // Blau
    }
    else if (bat >= 50) {
        setColor(0, 255, 0);     // Grün
    }
    else if (bat >= 25) {
        setColor(255, 255, 0);   // Gelb
    }
    else {
        setColor(255, 0, 0);     // Rot
    }
}

void MyControllerConfig::onPress(const char* name) {
    ESP_LOGI(TAG, "[PRESS] %s", name);


    if (strcmp(name, "up") == 0)
        setColor(0, 255, 0);

    if (strcmp(name, "down") == 0)
        setColor(255, 0, 0);

    if (strcmp(name, "circle") == 0)
        rumble(255, 0, 200);


    if (strcmp(name, "left") == 0){
        ESP_LOGI(TAG,"DPAD_LEFT LED an");
        gpio_set_level(GPIO_NUM_2, 1);   // LED an
    }
    if (strcmp(name, "right") == 0){
        ESP_LOGI(TAG,"DPAD_RIGHT LED aus");
        gpio_set_level(GPIO_NUM_2, 0);   // LED an
    }

}

void MyControllerConfig::onRelease(const char* name, float duration) {
    ESP_LOGI(TAG, "[RELEASE] %s (%.2fs)", name, duration);
}


void MyControllerConfig::onStick(const char* name, float x, float y) {
    ESP_LOGI(TAG, "[STICK] %s: x=%.2f y=%.2f", name, x, y);

    if (strcmp(name, "left") == 0) {
        // z.B. Motorsteuerung später
        // mappe x/y auf Motoren
    }
}

void MyControllerConfig::onTrigger(const char* name, float value, bool pressed) {
    ESP_LOGI(TAG, "[TRIGGER] %s: %.2f (pressed=%d)", name, value, pressed);

    if (strcmp(name, "L2") == 0 && pressed && value > 0.8f) {
        rumble(255, 0, 100);
    }

    if (strcmp(name, "R2") == 0 && value > 0.5f) {
        // z.B. LED heller machen, Motor schneller, etc.
    }
}

void MyControllerConfig::onGyro(float gx, float gy, float gz) {
    ESP_LOGI(TAG, "[GYRO] gx=%.2f gy=%.2f gz=%.2f", gx, gy, gz);
}

void MyControllerConfig::onAccel(float ax, float ay, float az) {
    ESP_LOGI(TAG, "[ACCEL] ax=%.2f ay=%.2f az=%.2f", ax, ay, az);
}

void MyControllerConfig::onLongPress(const char* name, float duration) {
    ESP_LOGI(TAG, "[LONG] %s (%.2fs)", name, duration);
    rumble(0, 255, 150);
}

void MyControllerConfig::onDoublePress(const char* name) {
    ESP_LOGI(TAG, "[DOUBLE] %s", name);
    setColor(255, 255, 0);
}





