#ifndef WATER_FLOW_H_
#define WATER_FLOW_H_

#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include "interrupt.h"
// #include "control_pump.h"            // required to unexport all exported pins

struct gpio_struct {
        unsigned int gpio_1;
        unsigned int gpio_2;
        unsigned int gpio_3;
};
struct timeval currentTime, oldTime;

volatile int pulse_count_1;
volatile int pulse_count_2;
volatile int pulse_count_3;
float ml_1, ml_2, ml_3;                         // millilitre
float ml_total_1, ml_total_2, ml_total_3;       // total millilitre
float elapsed, calibration_factor, flow_rate_1, flow_rate_2, flow_rate_3, previous_rate_1, previous_rate_2, previous_rate_3;
float timedifference_msec(struct timeval t0, struct timeval t1);
extern bool allowCURL;

void flow_1(void);
void flow_2(void);
void flow_3(void);
void *get_water_sensor(void *arguments);
void *interrupt_func(void *arguments);

#endif
