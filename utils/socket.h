//
// Created by admin on 2022/6/23.
//
#include "cJSON.h"

#ifndef PROJECT_SOCKET_H
#define PROJECT_SOCKET_H

extern char *sendRequest(char *ipAddr, int port, char *sendData);

typedef struct responseData {
    // 响应数据
    int start_resp_ts, finish_resp_ts, status_code;
    char *raw_string;
    cJSON *json;
} ResponseData;

extern ResponseData parseResponse(char *raw_resp);

extern ResponseData callBackend(const char *action, cJSON *json);

extern void jsonParseString(cJSON *json, char *key, char *dest);

extern int jsonParseInteger(cJSON *json, char *key, int *dest);

extern int jsonParseFloat(cJSON *json, char *key, double *dest);

extern int jsonArrayParseInteger(cJSON *json, int index, int *dest);

extern int jsonParseLong(cJSON *json, char *key, long long *dest);

#endif //PROJECT_SOCKET_H
