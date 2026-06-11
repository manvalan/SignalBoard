#ifndef SIGNALFS_H
#define SIGNALFS_H

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

enum SignalType { TYPE_MAIN, TYPE_SHUNT };
enum SignalAspect { ASPECT_RED, ASPECT_GREEN, ASPECT_YELLOW, ASPECT_STOP, ASPECT_GO, ASPECT_OBLIQUE };

class SignalFS {
  private:
    SignalType type;
    Adafruit_PWMServoDriver* pca;
    int pinR, pinG, pinV;
    int brR, brG, brV; // Livelli di luminosità (0-4095)

  public:
    // Costruttore aggiornato: accetta le luminosità (se non fornite, di default usa 4095)
    SignalFS(SignalType t, Adafruit_PWMServoDriver* pca_ptr, int r, int g, int v, int bR = 4095, int bG = 4095, int bV = 4095) {
        type = t; pca = pca_ptr; pinR = r; pinG = g; pinV = v;
        brR = bR; brG = bG; brV = bV;
    }

    void begin() {}

    void setAspect(SignalAspect aspect) {
        if (type == TYPE_MAIN) {
            // Usa brR, brG, brV invece del fisso 4095
            if (aspect == ASPECT_RED)         { pca->setPWM(pinR, 0, brR); pca->setPWM(pinG, 0, 0);   pca->setPWM(pinV, 0, 0); }
            else if (aspect == ASPECT_GREEN)  { pca->setPWM(pinR, 0, 0);   pca->setPWM(pinG, 0, 0);   pca->setPWM(pinV, 0, brV); }
            else if (aspect == ASPECT_YELLOW) { pca->setPWM(pinR, 0, 0);   pca->setPWM(pinG, 0, brG); pca->setPWM(pinV, 0, 0); }
        } else if (type == TYPE_SHUNT) {
            // STOP = orizzontale (B+A), GO = verticale (C+B), OBLIQUO = diagonale (C+A)
            if (aspect == ASPECT_STOP)        { pca->setPWM(pinR, 0, 0);   pca->setPWM(pinG, 0, brG); pca->setPWM(pinV, 0, brV); }
            else if (aspect == ASPECT_GO)     { pca->setPWM(pinR, 0, brR); pca->setPWM(pinG, 0, brG); pca->setPWM(pinV, 0, 0); }
            else if (aspect == ASPECT_OBLIQUE){ pca->setPWM(pinR, 0, brR); pca->setPWM(pinG, 0, 0);   pca->setPWM(pinV, 0, brV); }
        }
    }
};
#endif