#include "water_flow.h"
#include "curl_water.h"
#include "control_pump.h"

int main(int argc, char **argv, char **envp)
{
        if (argc != 4) {                                                                                // check for any arguments, if none, display messages
                printf("Error during execution:\n");
                printf("USAGE: <filename> <gpio-pin> <gpio-pin> <gpio-pin>\n\n");
                exit(-1);
        } else {                                                                                        // if received arguments, display message
                printf("Program Started ... \nPlease press <enter> to exit this program.\n");
        }

        struct gpio_struct args;
        args.gpio_1 = atoi(argv[1]);                                                                    // get the first argument
        args.gpio_2 = atoi(argv[2]);                                                                    // get the second argument
        args.gpio_3 = atoi(argv[3]);                                                                    // get the third argument

        pthread_t interrupt_service, water_sensor, curl_data, curl_remote, pump2_ctrl;

        /* Thread 1 responsible for interrupt service */
        if( pthread_create(&interrupt_service, NULL, &interrupt_func, (void *)&args) != 0){
                printf("Error: unable to create setting interrupt service thread\n");
                return -1;
        }

        /* Thread 2 to collect water data from water sensor */
        if( pthread_create(&water_sensor, NULL, &get_water_sensor, NULL) != 0){
                printf("Error: unable to create set water sensor thread\n");
                return -1;
        }

        /* Thread 3 to collect water data from water sensor */
        if( pthread_create(&curl_data, NULL, &cURL_data, NULL) != 0){
                                        printf("Error: unable to create curl water data thread\n");
                                        return -1;
        }

        /* Thread 4 responsible for interrupt service */
        if( pthread_create(&curl_remote, NULL, &get_remote_state, NULL) != 0){
                                        printf("Error: unable to create curl remote thread\n");
                                        return -1;
        }

        /* Thread 5 responsible for controlling pump 2 */
        if( pthread_create(&pump2_ctrl, NULL, &control_pump2, NULL) != 0){
                                        printf("Error: unable to create pump2 control thread\n");
                                        return -1;
        }

        pthread_join(water_sensor, NULL);
        pthread_join(interrupt_service, NULL);
        pthread_join(curl_data, NULL);
        pthread_join(curl_remote, NULL);
        pthread_join(pump2_ctrl, NULL);

        return 0;
}
