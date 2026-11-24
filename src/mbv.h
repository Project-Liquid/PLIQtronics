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
    bool moving;
    
    // --- Circular Buffer Implementation ---
    static const int MAX_SCHEDULE = 10; // Adjust this limit as needed

    struct ScheduledActuation {
        unsigned long trigger_ms;
        bool open; // true for open, false for close
    };
    
    ScheduledActuation schedule[MAX_SCHEDULE];
    int head = 0;  // Index of the oldest task (read point)
    int tail = 0;  // Index of the newest task (write point)
    int count = 0; // Current number of tasks in the queue

public:
    MBV(int speed_pin, int dir_pin_1, int dir_pin_2, int open_limit_pin, int close_limit_pin);
    void begin();
    void setSpeed(int speed);
    void brake();
    void coast();
    void update();
    void open();
    void close();
    void jog(int speed);
    void checkLimits();
    bool isOpen();
    bool isClosed();
    void setNextActuation(int delay, bool open);
    void checkScheduledActuation();
    void clearSchedule();
    void neutralize();
};

#endif // MBV_H