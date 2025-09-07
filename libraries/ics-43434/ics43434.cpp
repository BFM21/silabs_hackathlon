#include "ics43434.h"
#include "pins_arduino.h"

void ICS43434::init(){
    pinMode(PIN_MIC_ENABLE, OUTPUT);
    digitalWrite(PIN_MIC_ENABLE, HIGH);
    pinMode(MIC_SCK, OUTPUT);
    pinMode(MIC_SD, INPUT);
    pinMode(MIC_WS, OUTPUT);
    delay(100);

    while(1){

    }
}