#ifndef TRANSDUCER_H
#define TRANSDUCER_H

#include <Arduino.h>
//#include <ArduinoSTL.h>
//#include <string>

class Transducer {
private:
  int pin;
  float V_REF; // V
  float P_MIN; // PSI
  float P_MAX; // PSI
  int R_SHUNT; // Ohms
  int I_MIN = 4; // mA
  int I_MAX = 20; // mA
  float V_MIN = I_MIN * R_SHUNT / 1000.0; // V
  float V_MAX = I_MAX * R_SHUNT / 1000.0; // V

  int raw;
  float v;
  float i_mA;
  float pressure;

public:
  Transducer(int pin, float P_MIN, float P_MAX, float V_REF = 5.0, int R_SHUNT = 250);
  static float barToPSI(float bar);
  static float PSIToBar(float psi);
  float readPressure();
  String status();
  String value();
};

#endif // TRANSDUCER_H