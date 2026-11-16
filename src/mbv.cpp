#include "mbv.h"

MBV::MBV(int speed_pin, int dir_pin_1, int dir_pin_2, int open_limit_pin, int close_limit_pin) {
    this->speed_pin = speed_pin;
    this->dir_pin_1 = dir_pin_1;
    this->dir_pin_2 = dir_pin_2;
    this->open_limit_pin = open_limit_pin;
    this->close_limit_pin = close_limit_pin;

    pinMode(speed_pin, OUTPUT);
    pinMode(dir_pin_1, OUTPUT);
    pinMode(dir_pin_2, OUTPUT);
    pinMode(open_limit_pin, INPUT_PULLUP);
    pinMode(close_limit_pin, INPUT_PULLUP);

    this->speed = 0;
    this->forward = true;
    target = 0;
}

void MBV::setSpeed(int speed) {
    this->speed = constrain(speed, 0, 255);
}

void MBV::setDirection(bool forward) {
    this->forward = forward;
}

void MBV::brake() {
    digitalWrite(dir_pin_1, LOW);
    digitalWrite(dir_pin_2, LOW);
    analogWrite(speed_pin, 255);
    target = 0;
}

void MBV::coast() {
    digitalWrite(dir_pin_1, LOW);
    digitalWrite(dir_pin_2, LOW);
    analogWrite(speed_pin, 0);
    target = 0;
}

void MBV::run() {
    if (forward) {
        digitalWrite(dir_pin_1, HIGH);
        digitalWrite(dir_pin_2, LOW);
    } else {
        digitalWrite(dir_pin_1, LOW);
        digitalWrite(dir_pin_2, HIGH);
    }
    analogWrite(speed_pin, speed);

    if (target == 1) {
        if (digitalRead(open_limit_pin) == 1) {
            Serial.println("MBV: Open limit reached");
            brake();
            target = 0;
        }
    } else if (target == 2) {
        if (digitalRead(close_limit_pin) == 1) {
            Serial.println("MBV: Close limit reached");
            brake();
            target = 0;
        }
    }
}

void MBV::open() {
    setDirection(true);
    speed = 255;
    target = 1;
    run();
}

void MBV::close() {
    setDirection(false);
    speed = 255;
    target = 2;
    run();
}

void MBV::jog(int speed) {
    if (speed > 0) {
    digitalWrite(dir_pin_1, HIGH);
    digitalWrite(dir_pin_2, LOW);
    analogWrite(speed_pin, speed);
    }
    else {
    digitalWrite(dir_pin_1, LOW);
    digitalWrite(dir_pin_2, HIGH);
    analogWrite(speed_pin, abs(speed));
    }
}

void MBV::check_limits() {
    Serial.print("Open switch: ");
    Serial.println(digitalRead(open_limit_pin));
    Serial.print("Close switch: ");
    Serial.println(digitalRead(close_limit_pin));
}