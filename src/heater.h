#ifndef HEATER_H
#define HEATER_H

#include "transducer.h"

class Heater {
private:
  int relay_pin;
  Transducer* PT;
  bool blanketOn = false;
  float target_pressure;
  float threshold_error;

public:
  Heater(int relay_pin, Transducer* PT);
  void begin();
  void setTarget(float target, float threshold_error = 10);
  bool isOn();
  void on();
  void off();
  void update();
};

#endif