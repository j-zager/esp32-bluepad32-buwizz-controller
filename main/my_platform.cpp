// Example file - Public Domain
// Need help? https://tinyurl.com/bluepad32-help

extern "C" {
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
}


#define LED_PIN GPIO_NUM_2

#include <string.h>

#include <uni.h>

#include "MyControllerConfig.h"
#include "slot_helpers.h"

#include "controllerEventManager.h"
#include "buwizz.h"


extern MyControllerConfig controllers[4];
extern ControllerEventManager eventManager;

// // HIER wird das Objekt physikalisch erzeugt
// Definition der Adressen
static bd_addr_t BuWizz_ADDR1 = {0x50, 0xFA, 0xAB, 0x6D, 0x03, 0x4C};
static bd_addr_t BuWizz_ADDR2  = {0x50, 0xFA, 0xAB, 0x6D, 0x48, 0x70}; // Beispiel
static bd_addr_t BuWizz_ADDR3  = {0x50, 0xFA, 0xAB, 0xBA, 0x2A, 0x96}; // Beispiel
static bd_addr_t BuWizz_ADDR4  = {0x50, 0xFA, 0xAB, 0x6D, 0x51, 0xDE}; // Beispiel

// Die Objekte werden mit den Adressen erstellt
BuWizz bwz1(BuWizz_ADDR1);
BuWizz bwz2(BuWizz_ADDR2);
BuWizz bwz3(BuWizz_ADDR3);
BuWizz bwz4(BuWizz_ADDR4);

// Wir machen sie in einem Array für den Handler zugänglich (in buwizz.cpp oder platform.cpp)
BuWizz* mulBuWizz[] = { &bwz2, &bwz1, &bwz4 };
// BuWizz* mulBuWizz[] = { &bwz2, &bwz1 };

// const int NUM_BRICKS = 2;
const int NUM_BRICKS = sizeof(mulBuWizz) / sizeof(mulBuWizz[0]);


void myMainTask(void* p);
static void initPins();

// Custom "instance"
typedef struct my_platform_instance_s {
    uni_gamepad_seat_t gamepad_seat;  // which "seat" is being used
} my_platform_instance_t;

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t* d);
static my_platform_instance_t* get_my_platform_instance(uni_hid_device_t* d);

//
// Platform Overrides
//
static void my_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("custom: init()\n");

#if 0
    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // Inverted axis with inverted Y in RY.
    mappings.axis_x = UNI_GAMEPAD_MAPPINGS_AXIS_RX;
    mappings.axis_y = UNI_GAMEPAD_MAPPINGS_AXIS_RY;
    mappings.axis_ry_inverted = true;
    mappings.axis_rx = UNI_GAMEPAD_MAPPINGS_AXIS_X;
    mappings.axis_ry = UNI_GAMEPAD_MAPPINGS_AXIS_Y;

    // Invert A & B
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;

    uni_gamepad_set_mappings(&mappings);
#endif
    //    uni_bt_service_set_enabled(true);
}

static void my_platform_on_init_complete(void) {
    logi("custom: on_init_complete()\n");

    // Safe to call "unsafe" functions since they are called from BT thread

    // Start scanning
    uni_bt_start_scanning_and_autoconnect_unsafe();
    uni_bt_allow_incoming_connections(true);

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (1)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();


    // Pins initialisieren
    initPins();
    // Deine Task starten
    xTaskCreate(myMainTask, "my_task", 4096, NULL, 5, NULL);

    // buwizz.init();// eventuell vor xtaskCreate fürs timing.

    for (int i = 0; i < NUM_BRICKS; i++) {
        mulBuWizz[i]->init();
    }


}

static uni_error_t my_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    // You can filter discovered devices here.
    // Just return any value different from UNI_ERROR_SUCCESS;
    // @param addr: the Bluetooth address
    // @param name: could be NULL, could be zero-length, or might contain the name.
    // @param cod: Class of Device. See "uni_bt_defines.h" for possible values.
    // @param rssi: Received Signal Strength Indicator (RSSI) measured in dBms. The higher (255) the better.

    // As an example, if you want to filter out keyboards, do:
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
        logi("Ignoring keyboard\n");
        return UNI_ERROR_IGNORE_DEVICE;
    }

    return UNI_ERROR_SUCCESS;
}

static void my_platform_on_device_connected(uni_hid_device_t* d) {
    logi("custom: device connected: %p\n", d);
}

static void my_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("custom: device disconnected: %p\n", d);

    int slot = findSlotForDevice(d);
    if (slot >= 0) {
        logi("Freeing slot %d\n", slot);
        deviceForSlot[slot] = nullptr;
        controllers[slot].reset();   // optional, falls du eine Reset-Funktion hast
    }

}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t* d) {
    logi("custom: device ready: %p\n", d);
    // my_platform_instance_t* ins = get_my_platform_instance(d);
    // ins->gamepad_seat = GAMEPAD_SEAT_A;

    int slot = findFreeSlot();
    if (slot < 0) {
        loge("No free controller slots!\n");
        return UNI_ERROR_SUCCESS;
    }

    deviceForSlot[slot] = d;

    // Controller initialisieren
    controllers[slot].setDevice(d); 
    controllers[slot].startGyroCalibration();

    logi("Assigned controller to slot %d\n", slot);

    trigger_event_on_gamepad(d);
    return UNI_ERROR_SUCCESS;

}

static void my_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    int slot;
    // // logi("(%p), id=%d, \n", d, uni_hid_device_get_idx_for_instance(d));
    // // uni_controller_dump(ctl);

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            slot = findSlotForDevice(d);
            if (slot < 0)
                return;

                // Einmalige Initialisierung
            if (!controllers[slot].hasGpActive()) {
                controllers[slot].setGamepad(&ctl->gamepad, d, ctl->battery);
            }

            controllers[slot].update(ctl->gamepad, d, ctl->battery);


        break;
            // // Toggle Bluetooth connections
            //     logi("*** Stop scanning\n");
            //     uni_bt_stop_scanning_safe();
            //     logi("*** Start scanning\n");
            //     uni_bt_start_scanning_and_autoconnect_safe();

        case UNI_CONTROLLER_CLASS_NONE:
        case UNI_CONTROLLER_CLASS_MOUSE:
        case UNI_CONTROLLER_CLASS_KEYBOARD:
        case UNI_CONTROLLER_CLASS_BALANCE_BOARD:
        case UNI_CONTROLLER_CLASS_COUNT:
            break;
        default:
            break;
    }
}

static const uni_property_t* my_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON: {
            // uni_hid_device_t* d = data;
            uni_hid_device_t* d = (uni_hid_device_t*)data;


            if (d == NULL) {
                loge("ERROR: my_platform_on_oob_event: Invalid NULL device\n");
                return;
            }
            logi("custom: on_device_oob_event(): %d\n", event);

            my_platform_instance_t* ins = get_my_platform_instance(d);
            ins->gamepad_seat = ins->gamepad_seat == GAMEPAD_SEAT_A ? GAMEPAD_SEAT_B : GAMEPAD_SEAT_A;

            trigger_event_on_gamepad(d);
            break;
        }

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            logi("custom: Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("my_platform_on_oob_event: unsupported event: 0x%04x\n", event);
            break;
    }
}

//
// Helpers
//
static my_platform_instance_t* get_my_platform_instance(uni_hid_device_t* d) {
    return (my_platform_instance_t*)&d->platform_data[0];
}

static void trigger_event_on_gamepad(uni_hid_device_t* d) {
    my_platform_instance_t* ins = get_my_platform_instance(d);

    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 150 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        d->report_parser.set_player_leds(d, ins->gamepad_seat);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        uint8_t red = (ins->gamepad_seat & 0x01) ? 0xff : 0;
        uint8_t green = (ins->gamepad_seat & 0x02) ? 0xff : 0;
        uint8_t blue = (ins->gamepad_seat & 0x04) ? 0xff : 0;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}


extern "C" struct uni_platform* get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "custom",
        .init = my_platform_init,
        .on_init_complete = my_platform_on_init_complete,
        .on_device_discovered = my_platform_on_device_discovered,
        .on_device_connected = my_platform_on_device_connected,
        .on_device_disconnected = my_platform_on_device_disconnected,
        .on_device_ready = my_platform_on_device_ready,
        .on_gamepad_data = NULL,                     // deprecated
        .on_controller_data = my_platform_on_controller_data,
        .get_property = my_platform_get_property,
        .on_oob_event = my_platform_on_oob_event,
        .device_dump = NULL,
        .register_console_cmds = NULL,
    };

    return &plat;
}



void myMainTask(void* p) {
    uint64_t last = esp_timer_get_time(); // µs
    static uint64_t last_scan_retry = 0;
    static bool missing = false;

    static bool all_ready = true;
    static bool any_trigger = false;

    // Statische Variablen für das Timing
    static uint64_t last_scheduler_tick = 0;
    int current_brick_idx = 0;

    // --- KONFIGURATION ---
    const uint64_t TARGET_CYCLE_TIME = 40000; // Ziel: Jeder Stein alle 40ms (25Hz)
    const uint64_t MIN_GAP_TIME = 50000;      // Sicherheit: Min. 10ms zwischen zwei Paketen

    // Dynamische Berechnung des Sende-Intervalls
    // Wir nehmen entweder die geteilte Zeit ODER die Mindestpause (je nachdem, was größer ist)
    uint64_t step_interval = TARGET_CYCLE_TIME / NUM_BRICKS;
    if (step_interval < MIN_GAP_TIME) step_interval = MIN_GAP_TIME;

    // Motoren test
    for (int bu = 0; bu < NUM_BRICKS; bu++) {
        mulBuWizz[bu]->saveMotorAll(60,60,60,60,0);
    }

    while (1) {
        uint64_t now = esp_timer_get_time();

        // -----------------------------
        // MULTI-Controller HANDLING
        // -----------------------------
        // alle 10 ms
        if (now - last > 10000) {
            last = now;
            //logi("Main loop 10 ms\n");
            for (int i = 0; i < 4; i++) {
                controllers[i].process();          // Press/Release, Sticks, Gyro, Accel
            }
        }
        // Batterie-LED-Logik für alle Controller
        for (int i = 0; i < 4; i++) {
            controllers[i].updateBatteryLED(now);
        }

        eventManager.process();
        // -----------------------------
        // MULTI-BUWIZZ HANDLING
        // -----------------------------


        // 1. Verbinde und Reverbinde Logik
        if (now - last_scan_retry > 2000000) { // Alle 4 -> 3s Sekunden prüfen
            // all_connected = true;
            missing = false;
            all_ready = true;
            any_trigger = false;
            for(int i=0; i<NUM_BRICKS; i++) {
                // Falls jemand weder verbunden ist noch gerade versucht zu verbinden
                //  if(!mulBuWizz[i]->isConnected() && !mulBuWizz[i]->isConnectTriggered())missing = true;
                if(!mulBuWizz[i]->isConnected() && !mulBuWizz[i]->getMotorHandle()!=0){ 
                    printf("Missing id %d 0x%02X\n",i,mulBuWizz[i]->getAddr(5));
                    missing = true;
                } 
                

                // Wer noch nicht fahrbereit ist (Discovery läuft noch)
                if(!mulBuWizz[i]->isReady()) {
                    all_ready = false;
                }
                // ein Buwizz ist im Verbinde Prozess
                if(mulBuWizz[i]->isConnectTriggered()){
                    any_trigger = true;
                }

            }
            if(missing && !any_trigger) {
                printf("Main: Jemand fehlt, starte Scan...\n");
                uni_bt_le_scan_start();
                missing = false;
            }
            else  if(all_ready){
                printf("Main: Alle Steine verbunden stoppe Scan\n");
                for (int idx = 0; idx < NUM_BRICKS; idx++) {
                    printf("loop Id:%d Buwizz id: 0x%02X motor conhandle 0x%04X\n ",idx,mulBuWizz[idx]->getAddr(5), mulBuWizz[idx]->getConnectHandle());
                }
                uni_bt_le_scan_stop();
            }

            last_scan_retry = now;
        }

        // 2. STATUS-UPDATES (Discovery/Mode) immer so schnell wie möglich
        for (int b = 0; b < NUM_BRICKS; b++) {
            mulBuWizz[b]->triggerConnect(now); 
        }




        // --- 3. ECHTZEIT-PRÜFUNG DER FLOTTE (Jeden Loop-Durchgang!) ---
        // Das hier darf NICHT im 4-Sekunden-Block stehen
        all_ready = true;
        for(int i=0; i<NUM_BRICKS; i++) {
            if(!mulBuWizz[i]->isReady()) {
                all_ready = false;
                break;
            }
        }

        // 4. MOTOR-BEFEHLE gedrosselt auf min 10ms Pause
        // --- SCHEDULER AUSFÜHRUNG ---
        if ((now - last_scheduler_tick) > step_interval) {
            last_scheduler_tick = now;

            // Den aktuellen Stein aus dem Array holen
            // BuWizz* b = mulBuWizz[current_brick_idx];

            // Sicherheits-Check: Nur senden, wenn Stein initialisiert
            if (all_ready && NUM_BRICKS >0) {
                // Hier kommen später die Stick-Werte rein
                // mulBuWizz[current_brick_idx]->setMotors(60, 60, 60, 60);
                mulBuWizz[current_brick_idx]->useMotors();
            }
            //  else if (mulBuWizz[current_brick_idx]->isConnected()) {
            //     // Not-Aus solange die Flotte noch nicht bereit ist
            //     mulBuWizz[current_brick_idx]->setMotors(0, 0, 0, 0);
            // }

            // Index für den nächsten Durchlauf hochzählen
            current_brick_idx = (current_brick_idx + 1) % NUM_BRICKS;
        }
        // CPU freigeben (wichtig!)
        vTaskDelay(1);
    }
}

        // if (buwizz.isConnected() && (now - last_battery_check > 5000000)) {
        //     buwizz.requestBattery();
        //     last_battery_check = now;
        // }




static void initPins() {
    gpio_config_t io = {};
    io.intr_type = GPIO_INTR_DISABLE;
    io.mode = GPIO_MODE_OUTPUT;
    io.pin_bit_mask = (1ULL << LED_PIN);
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io);

    gpio_set_level(LED_PIN, 0);   // LED aus
}
