#include "mbv.h"

MBV::MBV(int speed_pin, int dir_pin_1, int dir_pin_2, int open_limit_pin, int close_limit_pin) {
    this->speed_pin = speed_pin;
    this->dir_pin_1 = dir_pin_1;
    this->dir_pin_2 = dir_pin_2;
    this->open_limit_pin = open_limit_pin;
    this->close_limit_pin = close_limit_pin;

    this->speed = 0;
    this->moving = false;
}

void MBV::begin() {
    pinMode(speed_pin, OUTPUT);
    pinMode(dir_pin_1, OUTPUT);
    pinMode(dir_pin_2, OUTPUT);
    pinMode(open_limit_pin, INPUT_PULLUP);
    pinMode(close_limit_pin, INPUT_PULLUP);
}

void MBV::setSpeed(int speed) {
    this->speed = constrain(speed, -255, 255);
}

void MBV::brake() {
    digitalWrite(dir_pin_1, LOW);
    digitalWrite(dir_pin_2, LOW);
    analogWrite(speed_pin, 255);
    speed = 0;
    moving = false;
}

void MBV::coast() {
    digitalWrite(dir_pin_1, LOW);
    digitalWrite(dir_pin_2, LOW);
    analogWrite(speed_pin, 0);
    speed = 0;
    moving = false;
}

void MBV::update() {
    if (moving) {
        if (speed > 0) {
            digitalWrite(dir_pin_1, HIGH);
            digitalWrite(dir_pin_2, LOW);
        } else if (speed < 0) {
            digitalWrite(dir_pin_1, LOW);
            digitalWrite(dir_pin_2, HIGH);
        }
        analogWrite(speed_pin, abs(speed));

        if (speed > 0) {
            if (digitalRead(open_limit_pin) == LOW) {
                Serial.println("MBV: Open limit reached");
                brake();
                speed = 0;
            }
        } else if (speed < 0) {
            if (digitalRead(close_limit_pin) == LOW) {
                Serial.println("MBV: Close limit reached");
                brake();
                speed = 0;
            }
        }
    }
}

void MBV::open() {
    speed = 255;
    moving = true;
    update();
}

void MBV::close() {
    speed = -255;
    moving = true;
    update();
}

void MBV::jog(int speed) {
    moving = true;
    if (speed > 0) {
    digitalWrite(dir_pin_1, HIGH);
    digitalWrite(dir_pin_2, LOW);
    }
    else if (speed < 0) {
    digitalWrite(dir_pin_1, LOW);
    digitalWrite(dir_pin_2, HIGH);
    }
    analogWrite(speed_pin, abs(speed));
}

void MBV::checkLimits() {
    Serial.print("Open switch: ");
    Serial.println(digitalRead(open_limit_pin) ? "HIGH" : "LOW");
    Serial.print("Close switch: ");
    Serial.println(digitalRead(close_limit_pin) ? "HIGH" : "LOW");
}

bool MBV::isOpen() {
    if (digitalRead(open_limit_pin) == LOW) {
        return true;
    }
    return false;
}

bool MBV::isClosed() {
    if (digitalRead(close_limit_pin) == LOW) {
        return true;
    }
    return false;
}

void MBV::setNextActuation(int delay, bool open) {
    if (count < MAX_SCHEDULE) {
    schedule[tail] = { millis() + (unsigned long)delay, open };
    tail = (tail + 1) % MAX_SCHEDULE; // Wrap around if we hit the end of the array
    count++;
    } else {
    Serial.println("Warning: Actuation queue is full!");
    }
}

void MBV::checkScheduledActuation() {
    if (count > 0 && millis() >= schedule[head].trigger_ms) {
    if (schedule[head].open) {
        open();
    } else {
        close();
    }
    head = (head + 1) % MAX_SCHEDULE; // Move the head forward to "erase" the task
    count--;
    }
}

void MBV::clearSchedule() {
    head = 0;
    tail = 0;
    count = 0;
}

void MBV::neutralize() {
    clearSchedule();
    brake();
}