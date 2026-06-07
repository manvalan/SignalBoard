#ifndef MAINSIGNAL_H
#define MAINSIGNAL_H

#include <Arduino.h>

// Enum degli stati del segnale alto di linea (Vela singola)
enum MainSignalState {
    MAIN_OFF,         // Tutto spento
    MAIN_STOP,        // Via impedita (Rosso)
    MAIN_CAUTION,     // Via libera con avviso di riduzione velocità (Giallo)
    MAIN_PROCEED,     // Via libera ordinaria (Verde)
    MAIN_BLINK_YELLOW // Giallo lampeggiante (Avviso avanzato / Riduzione a 30 o 60 km/h)
};

class MainSignal {
  private:
    int _pinRed;    // LED Rosso
    int _pinYellow; // LED Giallo
    int _pinGreen;  // LED Verde
    
    MainSignalState _currentState;
    unsigned long _lastBlinkTime;
    bool _blinkState;
    const int _blinkInterval = 500; // Frequenza del lampeggio (500 ms)

  public:
    // Costruttore: si inseriscono i pin nell'ordine cromatico standard (Rosso, Giallo, Verde)
    MainSignal(int pinRed, int pinYellow, int pinGreen) {
        _pinRed = pinRed;
        _pinYellow = pinYellow;
        _pinGreen = pinGreen;
        _currentState = MAIN_OFF;
        _lastBlinkTime = 0;
        _blinkState = false;
    }

    // Inizializzazione dei pin (da mettere nel void setup)
    void begin() {
        pinMode(_pinRed, OUTPUT);
        pinMode(_pinYellow, OUTPUT);
        pinMode(_pinGreen, OUTPUT);
        updateHardware(); 
    }

    // Imposta lo stato del segnale
    void setState(MainSignalState newState) {
        _currentState = newState;
        if (_currentState != MAIN_BLINK_YELLOW) {
            updateHardware(); 
        }
    }

    // Restituisce lo stato attuale
    MainSignalState getState() {
        return _currentState;
    }

    // Aggiornamento logico (da mettere nel void loop per gestire il lampeggio)
    void update() {
        if (_currentState == MAIN_BLINK_YELLOW) {
            unsigned long currentMillis = millis();
            if (currentMillis - _lastBlinkTime >= _blinkInterval) {
                _lastBlinkTime = currentMillis;
                _blinkState = !_blinkState;
                
                if (_blinkState) {
                    digitalWrite(_pinRed, LOW);
                    digitalWrite(_pinYellow, HIGH); // Giallo acceso
                    digitalWrite(_pinGreen, LOW);
                } else {
                    digitalWrite(_pinYellow, LOW);  // Giallo spento nella pausa
                }
            }
        }
    }

  private:
    // Traduce lo stato logico nell'accensione fisica dei singoli LED
    void updateHardware() {
        switch (_currentState) {
            case MAIN_OFF:
                digitalWrite(_pinRed, LOW);  digitalWrite(_pinYellow, LOW);  digitalWrite(_pinGreen, LOW);
                break;
            case MAIN_STOP:
                digitalWrite(_pinRed, HIGH); digitalWrite(_pinYellow, LOW);  digitalWrite(_pinGreen, LOW);
                break;
            case MAIN_CAUTION:
                digitalWrite(_pinRed, LOW);  digitalWrite(_pinYellow, HIGH); digitalWrite(_pinGreen, LOW);
                break;
            case MAIN_PROCEED:
                digitalWrite(_pinRed, LOW);  digitalWrite(_pinYellow, LOW);  digitalWrite(_pinGreen, HIGH);
                break;
            case MAIN_BLINK_YELLOW:
                // Gestito dall'update() per il lampeggio dinamico
                break;
        }
    }
};

#endif
