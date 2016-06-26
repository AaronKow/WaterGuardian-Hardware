#include "water_flow.h"

bool allowCURL = false;

void flow_1(void){
        pulse_count_1++;
}

void flow_2(void){
        pulse_count_2++;
}

void flow_3(void){
        pulse_count_3++;
}

float timedifference_msec(struct timeval t0, struct timeval t1){
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

// this function needs to work with interrupt_func in order to complete the readings for water flow sensor
void *get_water_sensor(void *arguments){
        sleep(1);             // sleep for 1 second after the interrupt start to prevent error of this function

        pulse_count_1 = 0;
        pulse_count_2 = 0;
        pulse_count_3 = 0;

        flow_rate_1 = 0.0;
        flow_rate_2 = 0.0;
        flow_rate_3 = 0.0;

        previous_rate_1 = 0.0;
        previous_rate_2 = 0.0;
        previous_rate_3 = 0.0;

        ml_1 = 0;
        ml_2 = 0;
        ml_3 = 0;

        ml_total_1 = 0;
        ml_total_2 = 0;
        ml_total_3 = 0;

        calibration_factor = 5.0;         // set the pulse frequency here, reference is 4.5

        gettimeofday(&oldTime, 0);        // set the base time

        /*========================*/
        /* Calculating Water Flow */
        /*========================*/
        while(1){
                gettimeofday(&currentTime, 0);    // set the current time
                elapsed = timedifference_msec(oldTime, currentTime);
                if (elapsed > 1000){              // check every 1 second
                    gettimeofday(&oldTime, 0);    // update base time
                    flow_rate_1 = ((1000.0 / (elapsed)) * pulse_count_1) / calibration_factor;
                    flow_rate_2 = ((1000.0 / (elapsed)) * pulse_count_2) / calibration_factor;
                    flow_rate_3 = ((1000.0 / (elapsed)) * pulse_count_3) / calibration_factor;

                    // calibration for all water_sensors if suddenly exceed normal value
                    // to prevent unstable of water data result
                    if(flow_rate_1 >= 11.0) flow_rate_1 = previous_rate_1;
                    if(flow_rate_2 >= 11.0) flow_rate_2 = previous_rate_2;
                    if(flow_rate_3 >= 11.0) flow_rate_3 = previous_rate_3;

                    // to get millilitre reading
                    ml_1 = (flow_rate_1 / 60) * 1000;
                    ml_total_1 += ml_1;
                    ml_2 = (flow_rate_2 / 60) * 1000;
                    ml_total_2 += ml_2;
                    ml_3 = (flow_rate_3 / 60) * 1000;
                    ml_total_3 += ml_3;

                    // only allow data to be cURL when data is correct
                    allowCURL = true;

                    // for debug
                    // printf("Flow rate 1: %f L/min; Current Liquid Flowing: %f ml/sec; Output Liquid Quantity: %f ml\n", flow_rate_1, ml_1, ml_total_1);
                    // printf("Flow rate 2: %f L/min; Current Liquid Flowing: %f ml/sec; Output Liquid Quantity: %f ml\n", flow_rate_2, ml_2, ml_total_2);
                    // printf("Flow rate 3: %f L/min; Current Liquid Flowing: %f ml/sec; Output Liquid Quantity: %f ml\n\n", flow_rate_3, ml_3, ml_total_3);

                    // set all previous rate to current rate
                    previous_rate_1 = flow_rate_1;
                    previous_rate_2 = flow_rate_2;
                    previous_rate_3 = flow_rate_3;

                    // Reset all Pulse Counters
                    pulse_count_1 = 0;
                    pulse_count_2 = 0;
                    pulse_count_3 = 0;

                    allowCURL = false;                // don't allow data to be cURL after pulse count is reset
                }
                usleep(100);
        }
        pthread_exit(NULL);
        return NULL;
}

void *interrupt_func(void *arguments){
        // get the pins from arguments
        struct gpio_struct *args = arguments;
        unsigned int gpio_1 = args -> gpio_1;
        unsigned int gpio_2 = args -> gpio_2;
        unsigned int gpio_3 = args -> gpio_3;

        // set the polling required for the interrupt
        struct pollfd fdset[4];
        int nfds = 4;                                     // number of pollfd structures in the fds array
        int gpio_fd1, gpio_fd2, gpio_fd3, timeout, rc;
        char *buf[MAX_BUF];
        int len;
        timeout = POLL_TIMEOUT;

        // setup for Water Flow Sensor 1
        gpio_export(gpio_1);
        gpio_set_dir(gpio_1, 0);                          // set pin to input
        gpio_set_value(gpio_1, 1);
        gpio_set_edge(gpio_1, "falling");                 // set with falling edge interrupt

        // setup for Water Flow Sensor 2
        gpio_export(gpio_2);
        gpio_set_dir(gpio_2, 0);                          // set pin to input
        gpio_set_value(gpio_2, 1);
        gpio_set_edge(gpio_2, "falling");                 // set with falling edge interrupt

        // setup for Water Flow Sensor 3
        gpio_export(gpio_3);
        gpio_set_dir(gpio_3, 0);                          // set pin to input
        gpio_set_value(gpio_3, 1);
        gpio_set_edge(gpio_3, "falling");                 // set with falling edge interrupt


        // open file descriptor for GPIO pins
        gpio_fd1 = gpio_fd_open(gpio_1);
        gpio_fd2 = gpio_fd_open(gpio_2);
        gpio_fd3 = gpio_fd_open(gpio_3);

        while (1) {
                memset((void*)fdset, 0, sizeof(fdset));   // clears the fdset block of memory

                // set fdset pointing to the respective file and event
                fdset[0].fd = STDIN_FILENO;
                fdset[0].events = POLLIN;
                fdset[1].fd = gpio_fd1;
                fdset[1].events = POLLPRI;
                fdset[2].fd = gpio_fd2;
                fdset[2].events = POLLPRI;
                fdset[3].fd = gpio_fd3;
                fdset[3].events = POLLPRI;

                // start the polling
                rc = poll(fdset, nfds, timeout);

                // if poll is unsuccessful, stop thread
                if (rc < 0) {
                        printf("\npoll() failed!\n");

                        pthread_exit(NULL);
                        return NULL;
                }

                // if poll returns 0 then call timed out, loop again in this case
                if (rc == 0) {
                        printf("Responding ...\n");   // let users know that the interrupt actually responding
                }

                // interrupt for Water Flow Sensor 1
                if (fdset[1].revents & POLLPRI) {
                        len = read(fdset[1].fd, buf, MAX_BUF);
                        flow_1();
                }

                // interrupt for Water Flow Sensor 2
                if (fdset[2].revents & POLLPRI) {
                        len = read(fdset[2].fd, buf, MAX_BUF);
                        flow_2();
                }

                // interrupt for Water Flow Sensor 3
                if (fdset[3].revents & POLLPRI) {
                        len = read(fdset[3].fd, buf, MAX_BUF);
                        flow_3();
                }

                // Press "ENTER" in the terminal to stop this program
                if (fdset[0].revents & POLLIN) {
                        (void)read(fdset[0].fd, buf, 1);

                        gpio_fd_close(gpio_fd1);
                        gpio_unexport(gpio_1);

                        gpio_fd_close(gpio_fd2);
                        gpio_unexport(gpio_2);

                        gpio_fd_close(gpio_fd3);
                        gpio_unexport(gpio_3);

                        /* Unexport all pins defined in control_pump.c */
                        gpio_unexport(8);
                        gpio_unexport(9);
                        gpio_unexport(10);
                        gpio_unexport(11);
                        gpio_unexport(12);

                        printf("Program successfully ended\n");

                        pthread_exit(NULL);
                        return NULL;
                }

                fflush(stdout);    // flash the standard output and keep looping until we freeze the program ^c
        }
}
