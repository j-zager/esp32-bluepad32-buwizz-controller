// #pragma once
// #include "esp_gattc_api.h"
// #include "esp_gap_ble_api.h"
// #include "esp_bt_defs.h"

// class BuwizzClient {
// public:
//     BuwizzClient();
//     bool connect(const esp_bd_addr_t mac);
//     void sendMotorData(int8_t m1, int8_t m2, int8_t m3, int8_t m4, uint8_t brakeMask);
//     void setPowerLevel(uint8_t level);
//     void requestStatus();
//     bool isConnected() const;

// private:
//     esp_gatt_if_t gattc_if = 0;
//     uint16_t conn_id = 0;
//     uint16_t char_handle = 0;
//     bool connected = false;

//     static void gapCallback(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
//     static void gattcCallback(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);
// };


#pragma once

extern "C" {
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"

#include "esp_bt_main.h"
}
#include "esp_log.h"

void test_buwizz_connect();
