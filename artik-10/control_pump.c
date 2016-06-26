#include "control_pump.h"

const unsigned int valve2Pin = 8;               // pin 2
const unsigned int valve3Pin = 9;               // pin 3
const unsigned int valve4Pin = 10;              // pin 4
const unsigned int pump2Pin = 11;               // pin 7
const unsigned int waterLVL4 = 12;              // pin 8

int pump_setup(void){
        gpio_export(valve2Pin);
        gpio_set_dir(valve2Pin, OUTPUT);
        gpio_export(valve3Pin);
        gpio_set_dir(valve3Pin, OUTPUT);
        gpio_export(valve4Pin);
        gpio_set_dir(valve4Pin, OUTPUT);
        gpio_export(pump2Pin);
        gpio_set_dir(pump2Pin, OUTPUT);
        gpio_export(waterLVL4);
        gpio_set_dir(waterLVL4, INPUT);

        /* HIGH is off, LOW is on in relay module */
        gpio_set_value(valve2Pin, HIGH);
        gpio_set_value(valve3Pin, HIGH);
        gpio_set_value(valve4Pin, HIGH);
        gpio_set_value(pump2Pin, HIGH);

        return 0;
}

void pump2_logic(void){
  // set the checker to prevent redundant gpio_set_value
  bool valve2AllowFlow = true;
  bool valve3AllowFlow = true;
  bool valve4AllowFlow = true;
  bool stopAllFlow = false;

  while(1){
        if((signal2 == '0') && (signal3 == '0') && (signal4 == '0') && stopAllFlow){
                // turn OFF all the valves and pump 2
                gpio_set_value(valve2Pin, HIGH);
                gpio_set_value(valve3Pin, HIGH);
                gpio_set_value(valve4Pin, HIGH);
                gpio_set_value(pump2Pin, HIGH);
                stopAllFlow = false;
        }
        if(signal2 == '0' && !valve2AllowFlow){
                // turn OFF valve2 only
                gpio_set_value(valve2Pin, HIGH);
                valve2AllowFlow = true;
        }
        if(signal3 == '0' && !valve3AllowFlow){
                // turn OFF valve3 only
                gpio_set_value(valve3Pin, HIGH);
                valve3AllowFlow = true;
        }
        if(signal4 == '0' && !valve4AllowFlow){
                // turn OFF valve4 only
                gpio_set_value(valve4Pin, HIGH);
                valve4AllowFlow = true;
        }
        if(signal2 == '1' && valve2AllowFlow){
                gpio_set_value(valve2Pin, LOW);      // turn on valve 2
                gpio_set_value(pump2Pin, LOW);       // turn on pump 2
                valve2AllowFlow = false;
                stopAllFlow = true;
        }
        if(signal3 == '1' && valve3AllowFlow){
                gpio_set_value(valve3Pin, LOW);      // turn on valve 3
                gpio_set_value(pump2Pin, LOW);       // turn on pump 3
                valve3AllowFlow = false;
                stopAllFlow = true;
        }
        if(signal4 == '1' && valve4AllowFlow){
                gpio_set_value(valve4Pin, LOW);      // turn on valve 3
                gpio_set_value(pump2Pin, LOW);       // turn on pump 3
                valve4AllowFlow = false;
                stopAllFlow = true;
        }
        sleep(1);
  }
}

/*void pump_debug(void){
        int d1 = gpio_get_value(waterLVL4);
        char c2 = signal2;
        char c3 = signal3;
        char c4 = signal4;

        printf("Signal 1: %d Signal 2: %c Signal 3: %c Signal 4: %c\n", d1, c2, c3, c4);
}*/

void *control_pump2(void *arguments){
        pump_setup();

        while(1){
                // pump_debug();
                pump2_logic();
                usleep(50);
        }
        pthread_exit(NULL);
        return NULL;
}
