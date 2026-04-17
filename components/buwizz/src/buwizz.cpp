// #include "buwizz.h"
// #include "esp_log.h"
// #include "esp_bt_main.h"
// #include "esp_bt_device.h"

// static const char* TAG = "BUWIZZ";

// // buwizz.cpp
// static const esp_bd_addr_t BUWIZZ_MAC = {0x50, 0xFA, 0xAB, 0x6D, 0x03, 0x4C};


// static const char* BUWIZZ_SERVICE_UUID   = "4e050000-74fb-4481-88b3-9919b1676e93";
// static const char* BUWIZZ_CHAR_UUID      = "000092d1-0000-1000-8000-00805f9b34fb";

// BuwizzClient::BuwizzClient() {}

// bool BuwizzClient::connect(const esp_bd_addr_t mac) {
//     ESP_LOGI(TAG, "Connecting to BuWizz...");

//     esp_err_t ret;

//     // GATT Client registrieren
//     ret = esp_ble_gattc_register_callback(BuwizzClient::gattcCallback);
//     if (ret) return false;

//     ret = esp_ble_gap_register_callback(BuwizzClient::gapCallback);
//     if (ret) return false;

//     ret = esp_ble_gattc_app_register(0x55);
//     if (ret) return false;

//     // Verbindung starten
//     ret = esp_ble_gattc_open(gattc_if,(uint8_t*)mac,BLE_ADDR_TYPE_PUBLIC,  true);
//     if (ret) return false;

//     return true;
// }

// bool BuwizzClient::isConnected() const {
//     return connected;
// }

// void BuwizzClient::sendMotorData(int8_t m1, int8_t m2, int8_t m3, int8_t m4, uint8_t brakeMask) {
//     if (!connected) return;

//     uint8_t payload[6] = { 0x10, (uint8_t)m1, (uint8_t)m2, (uint8_t)m3, (uint8_t)m4, brakeMask };

//     esp_ble_gattc_write_char(
//         gattc_if,
//         conn_id,
//         char_handle,
//         sizeof(payload),
//         payload,
//         ESP_GATT_WRITE_TYPE_NO_RSP,
//         ESP_GATT_AUTH_REQ_NONE
//     );
// }

// void BuwizzClient::setPowerLevel(uint8_t level) {
//     if (!connected) return;

//     uint8_t cmd[2] = { 0x11, level };

//     esp_ble_gattc_write_char(
//         gattc_if,
//         conn_id,
//         char_handle,
//         sizeof(cmd),
//         cmd,
//         ESP_GATT_WRITE_TYPE_NO_RSP,
//         ESP_GATT_AUTH_REQ_NONE
//     );
// }

// void BuwizzClient::requestStatus() {
//     if (!connected) return;

//     uint8_t cmd = 0x00;

//     esp_ble_gattc_write_char(
//         gattc_if,
//         conn_id,
//         char_handle,
//         1,
//         &cmd,
//         ESP_GATT_WRITE_TYPE_NO_RSP,
//         ESP_GATT_AUTH_REQ_NONE
//     );
// }

#include "buwizz.h"
#include "esp_log.h"

extern "C" {
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
}

static const char* TAG = "BUWIZZ";

// Deine BuWizz MAC
static const esp_bd_addr_t BUWIZZ_MAC = {0x50, 0xFA, 0xAB, 0x6D, 0x03, 0x4C};

// Wird gesetzt, sobald REG_EVT kommt
static esp_gatt_if_t gattc_if_global = ESP_GATT_IF_NONE;

// ---------------------------------------------------------
// GATT CALLBACK
// ---------------------------------------------------------
static void gattc_cb(esp_gattc_cb_event_t event,
                     esp_gatt_if_t gattc_if,
                     esp_ble_gattc_cb_param_t* param) {

    switch (event) {

    case ESP_GATTC_REG_EVT:
        ESP_LOGI(TAG, "GATTC_REG_EVT");
        gattc_if_global = gattc_if;

        // *** WICHTIG: OPEN erst hier! ***
        ESP_LOGI(TAG, "Opening BuWizz connection...");
        esp_ble_gattc_open(
            gattc_if_global,
            (uint8_t*)BUWIZZ_MAC,
            BLE_ADDR_TYPE_PUBLIC,
            true
        );
        break;

    case ESP_GATTC_OPEN_EVT:
        ESP_LOGI(TAG, "GATTC_OPEN_EVT: status=%d", param->open.status);
        if (param->open.status == ESP_GATT_OK) {
            ESP_LOGI(TAG, "✔ BuWizz connected!");
        } else {
            ESP_LOGE(TAG, "❌ Failed to connect");
        }
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------
// GAP CALLBACK
// ---------------------------------------------------------
static void gap_cb(esp_gap_ble_cb_event_t event,
                   esp_ble_gap_cb_param_t* param) {

    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        ESP_LOGI(TAG, "GAP scan param set");
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------
// TEST CONNECT
// ---------------------------------------------------------
void test_buwizz_connect() {
    ESP_LOGI(TAG, "Registering callbacks...");

    esp_ble_gattc_register_callback(gattc_cb);
    esp_ble_gap_register_callback(gap_cb);

    // App-ID frei wählbar
    esp_ble_gattc_app_register(0x55);
}
