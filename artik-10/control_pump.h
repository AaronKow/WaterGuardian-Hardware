#ifndef CONTROL_PUMP_H_
#define CONTROL_PUMP_H_

#include "interrupt.h"
#include "curl_water.h"   // required for signal variables

#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0

const unsigned int valve2Pin, valve3Pin, valve4Pin, pump2Pin, waterLVL4;

int pump_setup(void);
void pump2_logic(void);
void pump_debug(void);
void *control_pump2(void *arguments);

#endif