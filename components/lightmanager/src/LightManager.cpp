#include "LightManager.h"
// #include "esp_timer.h"

LightManager::LightManager(gpio_num_t dataPin, gpio_num_t shiftClockPin, gpio_num_t latchClockPin, gpio_num_t oePin)
    : pinData(dataPin), pinShiftClock(shiftClockPin), pinLatchClock(latchClockPin), pinOE(oePin) {}

void LightManager::init() {
    // 1. GPIO-Struktur befüllen (identisch zu Ihrem LED-Test-Konzept)
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << pinData) | (1ULL << pinShiftClock) | (1ULL << pinLatchClock) | (1ULL << pinOE);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // 2. Hardware-Sicherheit beim Booten: OE auf HIGH blockiert die Ausgänge der Register
    gpio_set_level(pinOE, 1);         
    gpio_set_level(pinShiftClock, 0); 
    gpio_set_level(pinLatchClock, 0);

    clearAll();
    writeRegister(); // Schreibt einen leeren Zustand (alles 0)

    // 3. Erst jetzt Ausgänge über Active-Low freigeben
    gpio_set_level(pinOE, 0);         
}

void LightManager::setLight(LightChannel channel, bool state) {
    if (state) {
        registerState |= (1ULL << channel);  // Bit setzen
    } else {
        registerState &= ~(1ULL << channel); // Bit löschen
    }
}

void LightManager::toggleLight(LightChannel channel) {
    registerState ^= (1ULL << channel);      // Bit invertieren (An <-> Aus)
    writeRegister();
}

void LightManager::clearAll() {
    registerState = 0; 
}

void LightManager::setBlinkerLeftActive(bool active) {
    blinkerLeftActive = active;
    if (!active) {
        registerState &= ~MASK_INDICATOR_LEFT;
        writeRegister();
    }
}

void LightManager::setBlinkerRightActive(bool active) {
    blinkerRightActive = active;
    if (!active) {
        registerState &= ~MASK_INDICATOR_RIGHT; 
        writeRegister();
    }
}

void LightManager::setHazardLightsActive(bool active) {
    hazardLightsActive = active;
    if (!active) {
        // Löscht alle Blinker-Pins (Links und Rechts) gleichzeitig aus dem Speicher
        registerState &= ~MASK_INDICATOR_LEFT;
        registerState &= ~MASK_INDICATOR_RIGHT;
        
        writeRegister(); // Zustand sofort an die Schieberegister senden
    }
}

void LightManager::setTrailerConnected(bool connected) {
    trailerConnected = connected;
    if (!connected) {
        registerState &= ~(1ULL << PIN_BEACON_TRAILER);
        writeRegister();
    }
}

void LightManager::setBeaconActive(bool active) {
    beaconActive = active;
    
    if (!active) {
        // Sofort alle 3 Dachleuchten-Pins im Speicher löschen
        for (int i = 0; i < 3; i++) {
            registerState &= ~(1ULL << beaconPins[i]);
        }
        // Sofort an die Schieberegister senden -> Lampen gehen augenblicklich aus!
        writeRegister();
    } else {
        lastBeaconTime = esp_timer_get_time();
        beaconStep = 0;
    }
}


// Bit-Streaming ohne blockierende Delays
void LightManager::writeRegister() {
    int totalBits = numRegisters * 8; // 24 Takte

    for (int i = totalBits - 1; i >= 0; i--) {
        bool bitValue = (registerState >> i) & 1;

        gpio_set_level(pinData, bitValue ? 1 : 0);

        // Impuls für Shift Clock (SH_CP) -> Bit weiterrücken
        gpio_set_level(pinShiftClock, 1);
        gpio_set_level(pinShiftClock, 0);
    }

    // Impuls für Latch Clock (ST_CP) -> Daten parallel ausgeben
    gpio_set_level(pinLatchClock, 1);
    gpio_set_level(pinLatchClock, 0);
}

// Takt-Verarbeitung für Blinker (völlig ohne Delays)
void LightManager::process() {
    uint64_t now = esp_timer_get_time();

     // --- 1. HIER LÄUFT IHR BESTEHENDER BLINKER-CODE ---
    // Globaler Herzschlag-Taktgeber für alle Blinker (läuft alle 500ms)
    if (now - lastBlinkTime >= 500000) {
        lastBlinkTime = now;
        blinkState = !blinkState; // Wechselt zwischen true und false

        bool changed = false;

        // --- LINKER BLINKER ---
        if (blinkerLeftActive || hazardLightsActive) {
            if (blinkState) registerState |= MASK_INDICATOR_LEFT;  // Nur links einschalten
            else            registerState &= ~MASK_INDICATOR_LEFT; // Nur links ausschalten
            changed = true;
        }

        // --- RECHTER BLINKER ---
        if (blinkerRightActive || hazardLightsActive) {
            if (blinkState) registerState |= MASK_INDICATOR_RIGHT;  // Nur rechts einschalten
            else            registerState &= ~MASK_INDICATOR_RIGHT; // Nur rechts ausschalten
            changed = true;
        }

        // Nur wenn sich wirklich etwas an den Blinkern geändert hat,
        // schieben wir das Muster an die Hardware raus. Das spart CPU-Zeit!
        if (changed) {
            writeRegister();
        }
    }


    // --- 2. VORNE/HINTEN WECHSEL-BLITZMUSTER (Takt: 80ms) ---
// --- 2. Taktsteuerung für die Rundumleuchten (alle 80ms) ---
    if (beaconActive && (now - lastBeaconTime >= 80000)) {
        lastBeaconTime = now;

        // Alle Beacon-Bits temporär im Speicher löschen
        for (int i = 0; i < 3; i++) {
            registerState &= ~(1ULL << beaconPins[i]);
        }

        // Ablaufsteuerung über Modulo-Schritte
        switch (beaconStep) {
            case 0: 
                registerState |= (1ULL << PIN_BEACON_FRONT); 
                break; // 1. Blitz vorne
            case 1: break;
            case 2: 
                registerState |= (1ULL << PIN_BEACON_FRONT); 
                break; // 2. Blitz vorne
            case 3: break; // Umschaltpause

            case 4: 
                registerState |= (1ULL << PIN_BEACON_REAR); // Blitz Heck Zugmaschine
                if (trailerConnected) registerState |= (1ULL << PIN_BEACON_TRAILER); // Synchroner Blitz Anhänger
                break; 
            case 5: break;
            case 6: 
                registerState |= (1ULL << PIN_BEACON_REAR); 
                if (trailerConnected) registerState |= (1ULL << PIN_BEACON_TRAILER);
                break; 
            case 7: break; // Gesamtpause
        }

        writeRegister();
        beaconStep = (beaconStep + 1) % 8; // Zählt im Kreis von 0 bis 7
    }


}
