#pragma once


extern "C" {
    #include "driver/gpio.h"
    #include <stdint.h>
    #include "esp_timer.h"
}

class LightManager {
public:
    // Kanäle entsprechend den physikalischen Ausgängen Ihrer 3 Schieberegister (0 bis 23)
    enum LightChannel {
        // Schieberegister 1 (Bits 0 - 7)
        PIN_DAYTIME_RUNNING     = 0,
        PIN_FRONT_INDICATOR_L   = 1,
        PIN_FRONT_INDICATOR_R   = 2,
        PIN_REAR_INDICATOR_L    = 3,
        PIN_REAR_INDICATOR_R    = 4,
        PIN_REVERSE_LIGHT       = 5,
        
        // Schieberegister 2 (Bits 8 - 15)
        PIN_SIDE_INDICATOR_L    = 8,
        PIN_SIDE_INDICATOR_R    = 9,
        PIN_TRAILER_INDICATOR_L = 10,
        PIN_TRAILER_INDICATOR_R = 11,
        PIN_TRAILER_LIGHT       = 12,
        // Bis zu 23 fortführen
        PIN_BEACON_FRONT   = 16, // Beide vorderen Leuchten der Zugmaschine
        PIN_BEACON_REAR    = 17, // Hintere Leuchte der Zugmaschine
        PIN_BEACON_TRAILER = 18  // NEU: Rundumleuchte ganz hinten auf dem Anhänger
    };


    // Konstruktor nimmt die ESP32-Hardware-Pins entgegen
    LightManager(gpio_num_t dataPin, gpio_num_t shiftClockPin, gpio_num_t latchClockPin, gpio_num_t oePin);

    // Initialisiert die GPIO-Konfiguration (ohne blockierende Delays)
    void init();
    
    // Verarbeitet zeitgesteuerte Effekte wie das Blinken (Non-Blocking)
    void process();

    // Direkte Zustandskontrolle
    void setLight(LightChannel channel, bool state);
    void clearAll();

    // Komfortfunktionen für die PS4-Controller-Events
    void toggleLight(LightChannel channel);
    void setBlinkerLeftActive(bool active);
    void setBlinkerRightActive(bool active);
    void setHazardLightsActive(bool active);
    void setBeaconActive(bool active);
    void setTrailerConnected(bool connected); // Zu Testzwecken über den Controller schaltbar

    // Schiebt das 24-Bit-Muster physikalisch in maximaler Geschwindigkeit raus
    void writeRegister();

private:
    gpio_num_t pinData;
    gpio_num_t pinShiftClock; // SH_CP
    gpio_num_t pinLatchClock; // ST_CP
    gpio_num_t pinOE;         // Output Enable (Active-Low)

    uint32_t registerState = 0; // Speicher für 24 Bits (3 Schieberegister * 8 Bit)
    const int numRegisters = 3;

    // Zustandsvariablen für zeitgesteuerte Effekte
    bool blinkerLeftActive = false;
    bool blinkerRightActive = false;
    bool hazardLightsActive = false;
    
    bool blinkState = false;
    uint64_t lastBlinkTime = 0; // In Mikrosekunden

    // Definition der Lichtgruppen über Bitmasken
    const uint32_t MASK_INDICATOR_LEFT = (1ULL << PIN_FRONT_INDICATOR_L) | 
                                         (1ULL << PIN_REAR_INDICATOR_L) | 
                                         (1ULL << PIN_SIDE_INDICATOR_L) |
                                         (1ULL << PIN_TRAILER_INDICATOR_L);

    const uint32_t MASK_INDICATOR_RIGHT = (1ULL << PIN_FRONT_INDICATOR_R) | 
                                          (1ULL << PIN_REAR_INDICATOR_R) | 
                                          (1ULL << PIN_SIDE_INDICATOR_R) |
                                          (1ULL << PIN_TRAILER_INDICATOR_R);

                                          bool beaconActive = false;
    int beaconStep = 0;          // Zählt die Phasen (0 bis 7)
    uint64_t lastBeaconTime = 0; // Zeitstempel für den Takt

    // Hilfsarray, um alle 3 Pins beim Ausschalten schnell zu löschen
    const uint8_t beaconPins[3] = { PIN_BEACON_FRONT, PIN_BEACON_REAR, PIN_BEACON_TRAILER };

    bool trailerConnected = false; 

};
