#include "transducer.h"

Transducer::Transducer(int pin, float P_MIN, float P_MAX, float V_REF, int R_SHUNT) {
  this->pin = pin;
  this->P_MIN = P_MIN;
  this->P_MAX = P_MAX;
  this->V_REF = V_REF;
  this->R_SHUNT = R_SHUNT;
}

static float Transducer::barToPSI(float bar) {
  return bar * 14.504;
}

static float Transducer::PSIToBar(float psi) {
  return psi / 14.504;
}

float Transducer::readPressure() {
    raw = analogRead(pin);
    v = raw * V_REF / 1023.0;
    i_mA = (v / R_SHUNT) * 1000.0;
    if (i_mA < 0) {
        i_mA = 0;
    } else if (i_mA > 25) {
        i_mA = 25;
    }
    pressure = (v - V_MIN) / (V_MAX - V_MIN) * (P_MAX - P_MIN) + P_MIN;

    return pressure;
}

String Transducer::status() {
  readPressure();
  String output = "";
  output += "raw = " + String(raw) + " V = " + String(v, 3) + "V  I = " + String(i_mA, 2) + " mA  P = " + String(pressure, 3) + " PSI";
  return output;
}

String Transducer::value() {
  return String(readPressure(), 3);
}
