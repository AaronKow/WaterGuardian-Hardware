/*
	The MIT License (MIT)

	Copyright (c) 2015 AaronKow

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/time.h>
#include <pthread.h>
#include <curl/curl.h>

 /****************************************************************
 * Constants
 ****************************************************************/
 
#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

/****************************************************************
 * My declarations
 ****************************************************************/

#define OUTPUT 1
#define INPUT 0
#define HIGH 1
#define LOW 0


volatile int flow_frequency1, flow_frequency2, flow_frequency3;
float l_hour_1, l_hour_2, l_hour_3; // litres/hour
float l_min_1, l_min_2, l_min_3;
float litres1, litres2, litres3;
float litres_total_1, litres_total_2, litres_total_3;
bool checker1, checker2, checker3;

char* url = "<YOUR URL>";						// http://192.168.0.106:3000/sensordata
char* remoteUrl = "<YOUR REMOTE URL>";			// http://192.168.0.106:3000/remotestate
char* device_id = "<YOUR DEVICE ID>";			// uWqgZon962M3XoFZk
char* device_token = "<YOUR DEVICE TOKEN>";		// 48f6ad75-b046-4858-9ea5-a7fefe1a2470


int valve2Pin = 8;		// pin 2
int valve3Pin = 9;		// pin 3
int valve4Pin = 10;		// pin 4
int pump2Pin = 11;		// pin 7
int waterLVL4 = 12;		// pin 8

char signal2, signal3, signal4;


/****************************************************************
 * gpio_export
 ****************************************************************/
int gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);		// http://ftp.tuwien.ac.at/languages/c/programming-bbrown/c_075.htm
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
 
	return 0;
}

/****************************************************************
 * gpio_unexport
 ****************************************************************/
int gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
 
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}
 
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_dir
 ****************************************************************/
int gpio_set_dir(unsigned int gpio, unsigned int out_flag)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}
 
	if (out_flag)
		write(fd, "out", 4);
	else
		write(fd, "in", 3);
 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_set_value
 ****************************************************************/
int gpio_set_value(unsigned int gpio, unsigned int value)
{
	int fd, len;
	char buf[MAX_BUF];
 
	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-value");
		return fd;
	}
 
	if (value)
		write(fd, "1", 2);
	else
		write(fd, "0", 2);
 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_get_value
 ****************************************************************/
int gpio_get_value(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
	char ch[2];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}
 
	read(fd, ch, 1);
	close(fd);

	return atoi(ch);
}


/****************************************************************
 * gpio_set_edge
 ****************************************************************/

int gpio_set_edge(unsigned int gpio, char *edge)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
 
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-edge");
		return fd;
	}
 
	write(fd, edge, strlen(edge) + 1); 
	close(fd);
	return 0;
}

/****************************************************************
 * gpio_fd_open
 ****************************************************************/

int gpio_fd_open(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
	fd = open(buf, O_RDONLY | O_NONBLOCK );
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}

/****************************************************************
 * gpio_fd_close
 ****************************************************************/

int gpio_fd_close(int fd)
{
	return close(fd);
}

/****************************************************************
 * Interrupt Function
 ****************************************************************/
void flow1(void){
	flow_frequency1++;
}

void flow2(void){
	flow_frequency2++;
}

void flow3(void){
	flow_frequency3++;
}

float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

struct interrupt_struct {
	unsigned int gpio1;
	unsigned int gpio2;
	unsigned int gpio3;
};

void *get_water_sensor(void *arguments){
	struct interrupt_struct *args = arguments;
	unsigned int gpio1 = args -> gpio1;
	unsigned int gpio2 = args -> gpio2;
	unsigned int gpio3 = args -> gpio3;
	// printf("Digit 1: %d, Digit 2: %d, Digit 3: %d\n", , args -> gpio2, args -> gpio3);


	struct pollfd fdset[4];
	int nfds = 4;					// number of pollfd structures in the fds array
	int gpio_fd1, gpio_fd2, gpio_fd3, timeout, rc;
	char *buf[MAX_BUF];
	int len;

	/*=============================*/
	/* Setup for water flow sensor */
	/*=============================*/
	struct timeval currentTime;
	struct timeval cloopTime;
	float elapsed;
	gettimeofday(&currentTime, 0);
	
	litres1 = 0;
   	litres_total_1 = 0;
   	litres2 = 0;
   	litres_total_2 = 0;
   	litres3 = 0;
   	litres_total_3 = 0;


   	gpio_export(gpio1);
	gpio_set_dir(gpio1, 0);							// set pin to input
	gpio_set_edge(gpio1, "rising");					// set with rising interrupt

	gpio_export(gpio2);
	gpio_set_dir(gpio2, 0);							// set pin to input
	gpio_set_edge(gpio2, "rising");					// set with rising interrupt

	gpio_export(gpio3);
	gpio_set_dir(gpio3, 0);							// set pin to input
	gpio_set_edge(gpio3, "rising");					// set with rising interrupt


	gpio_fd1 = gpio_fd_open(gpio1);					// the open file descriptor for GPIO pin
	gpio_fd2 = gpio_fd_open(gpio2);					// the open file descriptor for GPIO pin
	gpio_fd3 = gpio_fd_open(gpio3);					// the open file descriptor for GPIO pin

	timeout = POLL_TIMEOUT;

	while (1) {

		/*========================*/
		/* Calculating Water Flow */
		/*========================*/
		gettimeofday(&cloopTime, 0);
		elapsed = timedifference_msec(currentTime, cloopTime);
   		// printf("\n Time difference in %f milliseconds.\n", elapsed);	// for debug
		if (elapsed > 1000){	// check every 1 second
			gettimeofday(&currentTime, 0);	// update current time


			/*=====================*/
			/* Water Flow Sensor 1 */
			/*=====================*/
			// the higher the pulse the lower the output, vice versa
			l_hour_1 = (flow_frequency1 * 60 / 4.6); // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
      		l_min_1 = (flow_frequency1 * 60 / 4.5 / 60);
			litres1 = (flow_frequency1 / (4.5 * 60) );
      		litres_total_1 += litres1;
      		checker1 = true;

      		printf("Water Flow Sensor 1: (pin %d)\n", gpio1);
      		printf("%f L/hour; ", l_hour_1);
      		printf("%f L/min; ", l_min_1);
      		printf(" Total Water Level: %f litres\n\n", litres_total_1);

      		flow_frequency1 = 0; // Reset Counter


      		/*=====================*/
      		/* Water Flow Sensor 2 */
      		/*=====================*/
			l_hour_2 = (flow_frequency2 * 60 / 4.4); // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
      		l_min_2 = (flow_frequency2 * 60 / 4.4 / 60);
			litres2 = (flow_frequency2 / (4.4 * 60) );
      		litres_total_2 += litres2;
      		checker2 = true;

      		printf("Water Flow Sensor 2: (pin %d)\n", gpio2);
      		printf("%f L/hour; ", l_hour_2);
      		printf("%f L/min; ", l_min_2);
      		printf(" Total Water Level: %f litres\n\n", litres_total_2);

      		flow_frequency2 = 0; // Reset Counter


      		/*=====================*/
      		/* Water Flow Sensor 3 */
      		/*=====================*/
			l_hour_3 = (flow_frequency3 * 60 / 5.2); // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
      		l_min_3 = (flow_frequency3 * 60 / 5.2 / 60);
			litres3 = (flow_frequency3 / (5.2 * 60) );
      		litres_total_3 += litres3;
      		checker3 = true;

      		printf("Water Flow Sensor 3: (pin %d)\n", gpio3);
      		printf("%f L/hour; ", l_hour_3);
      		printf("%f L/min; ", l_min_3);
      		printf(" Total Water Level: %f litres\n\n", litres_total_3);

      		flow_frequency3 = 0; // Reset Counter

      		checker1 = false;
      		checker2 = false;
      		checker3 = false;
		}


		memset((void*)fdset, 0, sizeof(fdset));		// clears the fdset block of memory

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;					// Data other than high-priority data may be read without blocking

		fdset[1].fd = gpio_fd1;
		fdset[1].events = POLLPRI;					// Data other than high-priority data may be read without blocking
      
		fdset[2].fd = gpio_fd2;
		fdset[2].events = POLLPRI;					// High-priority data may be read witout blocking

		fdset[3].fd = gpio_fd3;
		fdset[3].events = POLLPRI;					// High-priority data may be read witout blocking

		rc = poll(fdset, nfds, timeout);      

		if (rc < 0) {								// if poll is unsuccessful returns a negative value
			printf("\npoll() failed!\n");

			pthread_exit(NULL);
			return NULL;
		}
      
		if (rc == 0) {								// if poll returns 0 then call timed out, loop again in this case
			// printf("responding ...\n");
		}
            
		if (fdset[1].revents & POLLPRI) {			// the button was pressed - high-priority value
			len = read(fdset[1].fd, buf, MAX_BUF);	// need to read this buffer
			// printf("\npoll() GPIO %d interrupt occurred\n", gpio);
			flow1();
			// printf("Flow Frequency: %d\n", flow_frequency);
		}

		if (fdset[2].revents & POLLPRI) {			// the button was pressed - high-priority value
			len = read(fdset[2].fd, buf, MAX_BUF);	// need to read this buffer
			// printf("\npoll() GPIO %d interrupt occurred\n", gpio);
			flow2();
			// printf("Flow Frequency: %d\n", flow_frequency);
		}

		if (fdset[3].revents & POLLPRI) {			// the button was pressed - high-priority value
			len = read(fdset[3].fd, buf, MAX_BUF);	// need to read this buffer
			// printf("\npoll() GPIO %d interrupt occurred\n", gpio);
			flow3();
			// printf("Flow Frequency: %d\n", flow_frequency);
		}

		if (fdset[0].revents & POLLIN) {			// keyboard input from the terminal
			(void)read(fdset[0].fd, buf, 1);
			gpio_fd_close(gpio1);
			gpio_fd_close(gpio2);
			gpio_fd_close(gpio3);
			
			gpio_unexport(gpio1);
			gpio_unexport(gpio2);
			gpio_unexport(gpio3);
			printf("Program successfully ended\n");
			
			pthread_exit(NULL);
			return NULL;
		}

		fflush(stdout);								// flash the standard output and keep looping until we freeze the program ^c
	}
}

void curlWaterGuardian(char* sensor_locate, float water_data, float l_min_data, float l_hour_data){
	CURL *curl;
	CURLcode res;
	printf("cURL in progress ... \n");

  	curl = curl_easy_init();  // get a curl handle

	if (curl) {
		struct curl_slist *requestHeader = NULL;
	    char bearer[60]="";
	    requestHeader = curl_slist_append(requestHeader, "Content-Type: application/json");

		char requestBody[256]="";
    	sprintf(requestBody, "{\n\"id\":\"%s\",\n\"token\":\"%s\",\n\"sensor_locate\":\"%s\",\n\"water_data\":\"%f\",\n\"l_min_data\":\"%f\",\n\"l_hour_data\":\"%f\"\n}" , device_id, device_token, sensor_locate, water_data, l_min_data, l_hour_data);

	    curl_easy_setopt(curl, CURLOPT_URL, url);                 // CURLOPT_URL = URL to work on
   		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, requestHeader);    // CURLOPT_HTTPHEADER = Custom HTTP headers
	    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody);      // CURLOPT_POSTFIELDS = Send a POST with this data
	    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);                  // CURLOPT_VERBOSE = Display verbose information

	    res = curl_easy_perform(curl);    // perform the request (performs the entire request in a blocking manner and returns when done)
	    if (res != CURLE_OK)
	      fprintf(stderr, "curl_easy_perform() failed: %s\n",
	              curl_easy_strerror(res)); // curl_easy_strerror = return string describing error code

	    curl_easy_cleanup(curl);              // End a libcurl easy handle
	}
}

void *cURL_data(void *arguments){
	while(1){
		if(checker1 && (signal2 == '1')){
			curlWaterGuardian("water_closet", litres1, l_min_1, l_hour_1);
		}
		else if(signal2 == '0'){
			curlWaterGuardian("water_closet", 0, 0, 0);	
		}

		if(checker2 && (signal3 == '1')){
			curlWaterGuardian("bathroom", litres2, l_min_2, l_hour_2);
		}
		else if(signal3 == '0'){
			curlWaterGuardian("bathroom", 0, 0, 0);
		}

		if(checker3 && (signal4 == '1')){
			curlWaterGuardian("kitchen", litres3, l_min_3, l_hour_3);
		}
		else if(signal4 == '0'){
			curlWaterGuardian("kitchen", 0, 0, 0);
		}

  		usleep(20);
	}

	pthread_exit(NULL);
	return NULL;
}


/****************************************************************
 * Control Pump 2
 ****************************************************************/


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


	/* HIGH is off, LOW is ON in current relay module */
	gpio_set_value(valve2Pin, HIGH);
	gpio_set_value(valve3Pin, HIGH);
	gpio_set_value(valve4Pin, HIGH);
	gpio_set_value(pump2Pin, HIGH);

	return 0;
}

void pump2_logic(void){
	if(!gpio_get_value(waterLVL4)){				// when barrel2 has no water
		gpio_set_value(valve2Pin, HIGH);		// turn off valve 2
		gpio_set_value(valve3Pin, HIGH);		// turn off valve 3
		gpio_set_value(valve4Pin, HIGH);		// turn off valve 4
		gpio_set_value(pump2Pin, HIGH);			// turn off pump 2
	}
	if(signal2 == '0'){
		gpio_set_value(valve2Pin, HIGH);
	}
	if(signal3 == '0'){
		gpio_set_value(valve3Pin, HIGH);
	}
	if(signal4 == '0'){
		gpio_set_value(valve4Pin, HIGH);
	}
	if((signal2 == '0') && (signal3 == '0') && (signal4 == '0')){
		gpio_set_value(pump2Pin, HIGH);
	}
	if(gpio_get_value(waterLVL4) && (signal2 == '1')){
		gpio_set_value(valve2Pin, LOW);			// turn on valve 2
		gpio_set_value(pump2Pin, LOW);			// turn on pump 2
	}
	if(gpio_get_value(waterLVL4) && (signal3 == '1')){
		gpio_set_value(valve3Pin, LOW);			// turn on valve 3
		gpio_set_value(pump2Pin, LOW);			// turn on pump 3
	}
	if(gpio_get_value(waterLVL4) && (signal4 == '1')){
		gpio_set_value(valve4Pin, LOW);			// turn on valve 4
		gpio_set_value(pump2Pin, LOW);			// turn on pump 4
	}
}

void pump_debug(void){
	int d1 = gpio_get_value(waterLVL4);
	char c2 = signal2;
	char c3 = signal3;
	char c4 = signal4;

	printf("Signal 1: %d Signal 2: %c Signal 3: %c Signal 4: %c\n", d1, c2, c3, c4);
}

void *control_pump2(void *arguments){
	pump_setup();

	while(1){
		// pump_debug();
		pump2_logic();
		usleep(500);
	}
	pthread_exit(NULL);
	return NULL;
}

/****************************************************************
 * Get Remote Status
 ****************************************************************/

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp){
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void remote_state_debug(void){
  	if(signal2 == '0'){
      printf("Signal 2: OFF, ");
    } else {
      printf("Signal 2: ON, ");
    }

    if(signal3 == '0'){
      printf("Signal 3: OFF, ");
    } else {
      printf("Signal 3: ON, ");
    }

    if(signal4 == '0'){
      printf("Signal 4: OFF\n");
    } else {
      printf("Signal 4: ON\n");
    }
}

void *get_remote_state(void *arguments){
  while(1){
    CURL *curl_handle;
    CURLcode res;

    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();

    struct curl_slist *requestHeader = NULL;
    requestHeader = curl_slist_append(requestHeader, "Content-Type: application/json");

    char requestBody[256]="";
    sprintf(requestBody, "{\n\"id\":\"%s\",\n\"token\":\"%s\"\n}" , device_id, device_token);

    curl_easy_setopt(curl_handle, CURLOPT_URL, remoteUrl);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, requestHeader);    // CURLOPT_HTTPHEADER = Custom HTTP headers
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, requestBody);      // CURLOPT_POSTFIELDS = Send a POST with this data
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    else {
      // printf("Data Received: %s\n", chunk.memory);
      signal2 = chunk.memory[0];
      signal3 = chunk.memory[2];
      signal4 = chunk.memory[4];
      // printf("%lu bytes retrieved\n", (long)chunk.size);
    }
    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
    curl_global_cleanup();
    // usleep(20);

    remote_state_debug();
    usleep(500);
  }

  pthread_exit(NULL);
  return NULL;
}


/****************************************************************
 * Main
 ****************************************************************/
int main(int argc, char **argv, char **envp)
{
	if (argc < 3) {		// checker for received arguments, if false, display messages
		printf("Usage: gpio-int <gpio-pin>\n\n");
		printf("Waits for a change in the GPIO pin voltage level or input on stdin\n");
		exit(-1);
	} else {
		printf("Program Started ... \nPlease press <enter> to exit this program with unexporting the pins defined\n");
	}

	struct interrupt_struct args;
	args.gpio1 = atoi(argv[1]);							// get the first argument, convert it to integer
	args.gpio2 = atoi(argv[2]);							// get the first argument, convert it to integer
	args.gpio3 = atoi(argv[3]);							// get the first argument, convert it to integer

	
	pthread_t sensor_thread, submit_data, control_pump, remote_state;
	/* Thread 1 to collect water data from sensors */
	if( pthread_create(&sensor_thread, NULL, &get_water_sensor, (void *)&args) != 0){
		printf("Error: unable to set sensors interrupts thread\n");
		return -1;
	}

	/* Thread 2 to submit water data via cURL */
	if( pthread_create(&submit_data, NULL, &cURL_data, NULL) != 0){
		printf("Error: unable to set cURL thread\n");
		return -1;
	}

	/* Thread 3 to control pump 2 */
	if( pthread_create(&control_pump, NULL, &control_pump2, NULL) != 0){
		printf("Error: unable to set control pump2 thread\n");
		return -1;
	}

	/* Thread 4 to get remote state */
	if( pthread_create(&remote_state, NULL, &get_remote_state, NULL) != 0){
	    printf("Error: unable to set remote state thread\n");
	    return -1;
	}

	pthread_join(sensor_thread, NULL);
	return 0;
}