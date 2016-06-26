#include "curl_water.h"

char* url = "http://192.168.43.85:3000/sensordata";           // 192.168.0.106:3000/sensordata
char* remoteUrl = "http://192.168.43.85:3000/remotestate";    // 192.168.0.106:3000/remotestate
char* device_id = "cJufJNZ5vBJksYbfj";
char* device_token = "6fd7b32d-9b26-4956-b973-f0fcf4e9d27e";

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

void curlWaterGuardian(char* sensor_locate, float water_data, float l_min_data, float l_hour_data){
        CURL *curl;
        CURLcode res;

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
            if (res != CURLE_OK){
              fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res)); // curl_easy_strerror = return string describing error code
            }
            curl_easy_cleanup(curl);              // End a libcurl easy handle
        }
}

void *cURL_data(void *arguments){
        // to prevent sending redundant water data during the water flow is stopped
        signal2_checker = false;    // for water closet
        signal3_checker = false;    // for bathroom
        signal4_checker = false;    // for kitchen

        while(1){
            if(signal2 == '1'){
                curlWaterGuardian("water_closet", (ml_1 / 1000), flow_rate_1, (flow_rate_1 * 60));
                printf(">> Water Level 1: %f, Flow rate 1a: %f L/min, Flow Rate 1b: %f L/hour\n\n", (ml_1 / 1000), flow_rate_1, (flow_rate_1 * 60));
                signal2_checker = true;
            }
            else if(signal2 == '0' && signal2_checker){
                curlWaterGuardian("water_closet", 0, 0, 0);
                signal2_checker = false;
            }

            if(signal3 == '1'){
                curlWaterGuardian("bathroom", (ml_2 / 1000), flow_rate_2, (flow_rate_2 * 60));
                printf(">> Water Level 2: %f, Flow rate 2a: %f L/min, Flow Rate 2b: %f L/hour\n\n", (ml_2 / 1000), flow_rate_2, (flow_rate_2 * 60));
                signal3_checker = true;
            }
            else if(signal3 == '0' && signal3_checker){
                curlWaterGuardian("bathroom", 0, 0, 0);
                signal3_checker = false;
            }

            if(signal4 == '1'){
                curlWaterGuardian("kitchen", (ml_3 / 1000), flow_rate_3, (flow_rate_3 * 60));
                printf(">> Water Level 3: %f, Flow rate 3a: %f L/min, Flow Rate 3b: %f L/hour\n\n", (ml_3 / 1000), flow_rate_3, (flow_rate_3 * 60));
                signal4_checker = true;
            }
            else if(signal4 == '0' && signal4_checker){
                curlWaterGuardian("kitchen", 0, 0, 0);
                signal4_checker = false;
            }
            sleep(1);
        }

        pthread_exit(NULL);
        return NULL;
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
      signal2 = chunk.memory[0];
      signal3 = chunk.memory[2];
      signal4 = chunk.memory[4];
    }
    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
    curl_global_cleanup();

    sleep(1);
  }

  pthread_exit(NULL);
  return NULL;
}
