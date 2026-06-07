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

  public:
    SignalFS(SignalType t, Adafruit_PWMServoDriver* pca_ptr, int r, int g, int v) {
        type = t; pca = pca_ptr; pinR = r; pinG = g; pinV = v;
    }

    void begin() {}

    void setAspect(SignalAspect aspect) {
        if (type == TYPE_MAIN) {
            if (aspect == ASPECT_RED)    { pca->setPWM(pinR, 0, 4095); pca->setPWM(pinG, 0, 0);    pca->setPWM(pinV, 0, 0); }
            else if (aspect == ASPECT_GREEN)  { pca->setPWM(pinR, 0, 0);    pca->setPWM(pinG, 0, 0);    pca->setPWM(pinV, 0, 4095); }
            else if (aspect == ASPECT_YELLOW) { pca->setPWM(pinR, 0, 0);    pca->setPWM(pinG, 0, 4095); pca->setPWM(pinV, 0, 0); }
        } else if (type == TYPE_SHUNT) {
            if (aspect == ASPECT_STOP)      { pca->setPWM(pinR, 0, 4095); pca->setPWM(pinG, 0, 4095); pca->setPWM(pinV, 0, 0); }
            else if (aspect == ASPECT_GO)        { pca->setPWM(pinR, 0, 0);    pca->setPWM(pinG, 0, 4095); pca->setPWM(pinV, 0, 4095); }
            else if (aspect == ASPECT_OBLIQUE)   { pca->setPWM(pinR, 0, 4095); pca->setPWM(pinG, 0, 0);    pca->setPWM(pinV, 0, 4095); }
        }
    }
};
#endif