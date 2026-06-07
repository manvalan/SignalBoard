#ifndef SHUNTSIGNAL_H
#define SHUNTSIGNAL_H

#include <Arduino.h>

// Enum degli stati del segnale basso di manovra
enum SignalState {
    SIGNAL_OFF,
    SIGNAL_STOP,
    SIGNAL_CLEAR_STRAIGHT,
    SIGNAL_CLEAR_DIAGONAL,
    SIGNAL_RESTRICTED
};

class ShuntSignal {
  private:
    int _pinC; // LED Alto (Vertice)
    int _pinB; // LED Basso Sinistro
    int _pinA; // LED Basso Destro
    
    SignalState _currentState;
    unsigned long _lastBlinkTime;
    bool _blinkState;
    const int _blinkInterval = 500; // Frequenza del lampeggio (500 ms)

  public:
    // Costruttore unico per connessione DIRETTA ai pin (Ordine geometrico C, B, A)
    ShuntSignal(int pinC, int pinB, int pinA) {
        _pinC = pinC;
        _pinB = pinB;
        _pinA = pinA;
        _currentState = SIGNAL_OFF;
        _lastBlinkTime = 0;
        _blinkState = false;
    }

    // Inizializzazione dei pin (da mettere nel void setup)
    void begin() {
        pinMode(_pinC, OUTPUT);
        pinMode(_pinB, OUTPUT);
        pinMode(_pinA, OUTPUT);
        updateHardware(); 
    }

    // Imposta lo stato del segnale
    void setState(SignalState newState) {
        _currentState = newState;
        if (_currentState != SIGNAL_RESTRICTED) {
            updateHardware(); 
        }
    }

    // Restituisce lo stato attuale
    SignalState getState() {
        return _currentState;
    }

    // Aggiornamento logico (da mettere nel void loop per il lampeggio)
    void update() {
        if (_currentState == SIGNAL_RESTRICTED) {
            unsigned long currentMillis = millis();
            if (currentMillis - _lastBlinkTime >= _blinkInterval) {
                _lastBlinkTime = currentMillis;
                _blinkState = !_blinkState;
                
                if (_blinkState) {
                    // Stato "Restricted": Luce diritta verticale (C + B accesi, A spento)
                    digitalWrite(_pinC, HIGH);
                    digitalWrite(_pinB, HIGH);
                    digitalWrite(_pinA, LOW);
                } else {
                    digitalWrite(_pinC, LOW);
                    digitalWrite(_pinB, LOW);
                    digitalWrite(_pinA, LOW);
                }
            }
        }
    }

  private:
    // Traduce lo stato logico nell'accensione fisica C, B, A
    void updateHardware() {
        switch (_currentState) {
            case SIGNAL_OFF:
                digitalWrite(_pinC, LOW);  digitalWrite(_pinB, LOW);  digitalWrite(_pinA, LOW);
                break;
            case SIGNAL_STOP:
                // Orizzontale (Stop): Accesi B e A alla base
                digitalWrite(_pinC, LOW);  digitalWrite(_pinB, HIGH); digitalWrite(_pinA, HIGH);
                break;
            case SIGNAL_CLEAR_STRAIGHT:
                // Verticale (Diritto): Accesi C e B
                digitalWrite(_pinC, HIGH); digitalWrite(_pinB, HIGH); digitalWrite(_pinA, LOW);
                break;
            case SIGNAL_CLEAR_DIAGONAL:
                // Diagonale (Inclinato): Accesi C e A
                digitalWrite(_pinC, HIGH); digitalWrite(_pinB, LOW);  digitalWrite(_pinA, HIGH);
                break;
            case SIGNAL_RESTRICTED:
                // Gestito dall'update() per il lampeggio
                break;
        }
    }
};

#endif