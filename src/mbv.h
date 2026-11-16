#ifndef MBV_H
#define MBV_H

#include <Arduino.h>

class MBV {
private:
    int speed_pin;
    int dir_pin_1;
    int dir_pin_2;
    int open_limit_pin;
    int close_limit_pin;

    int speed;
    bool forward;
    // enum Target { OPEN, CLOSE, NONE };
    // Target target = NONE;
    int target;

public:
    MBV(int speed_pin, int dir_pin_1, int dir_pin_2, int open_limit_pin, int close_limit_pin);
    void setSpeed(int speed);
    void setDirection(bool forward);
    void brake();
    void coast();
    void run();
    void open();
    void close();
    void jog(int speed);
    void check_limits();
};

#endif // MBV_H