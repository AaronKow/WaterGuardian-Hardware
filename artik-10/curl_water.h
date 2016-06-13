#ifndef CURL_WATER_H_
#define CURL_WATER_H_

#include <curl/curl.h>
#include "water_flow.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

char signal2, signal3, signal4;
bool signal2_checker, signal3_checker, signal4_checker;   // this checker is to prevent oversending 0 value to server

void remote_state_debug(void);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
void curlWaterGuardian(char* sensor_locate, float water_data, float l_min_data, float l_hour_data);
void *cURL_data(void *arguments);
void *get_remote_state(void *arguments);

#endif