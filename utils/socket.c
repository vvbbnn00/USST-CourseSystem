#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include "math.h"
#include "socket.h"
#include "cJSON.h"
#include "global.h"
#include "hmacsha256.h"

#pragma comment(lib, "ws2_32.lib")

/**
 * 打印错误信息
 * @param msg
 */
void printfSocketError(char *msg) {
    printf("[ERROR] Socket Error: %s\n", msg);
}

/**
 * 解析整型，遇到,符号停止
 * @param ptr 字符指针
 * @return 若异常，则返回-2147483648，否则返回解析出的数值
 */
int parseInteger(char **ptr) {
    char flag = 1;
    int ans = 0;
    if (**ptr == '-') {
        flag = -1;
        (*ptr)++;
    }
    while (**ptr != ',') {
        // 遇到字符串结尾，则判断为异常数据
        if (**ptr == '\0') {
            return -2147483648;
        }
        if (**ptr >= '0' && **ptr <= '9') {
            ans = ans * 10 + (**ptr - '0');
        } else {
            return -2147483648;
        }
        (*ptr)++;
    }
    return ans * flag;
}

/**
 * json解析字符串
 * @param json
 * @param key
 * @param dest
 */
void jsonParseString(cJSON *json, char *key, char *dest) {
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == NULL) {
        printf("[JSON Parse Error] Cannot Parse key:%s\n", key);
    }
    char *value = cJSON_Print(item);
    value[strlen(value) - 1] = 0; // 去除双引号
    strcpy(dest, value + 1);
    free(value);
}


/**
 * json解析整形数字
 *
 * @param json
 * @param key
 * @param dest
 * @return 成功返回0，不成功返回1
 */
int jsonParseInteger(cJSON *json, char *key, int *dest) {
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == NULL) {
        printf("[JSON Parse Error] Cannot Parse key:%s\n", key);
    }
    char *end_ptr, *data = cJSON_Print(item);
    int result = strtol(data, &end_ptr, 10);
    if (end_ptr == NULL) {
        return 1;
    }
    *dest = result;
    free(data);
    return 0;
}

/**
 * json解析长整形数字
 *
 * @param json
 * @param key
 * @param dest
 * @return 成功返回0，不成功返回1
 */
int jsonParseLong(cJSON *json, char *key, long long *dest) {
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == NULL) {
        printf("[JSON Parse Error] Cannot Parse key:%s\n", key);
    }
    char *end_ptr, *data = cJSON_Print(item);
    int result = strtol(data, &end_ptr, 10);
    if (end_ptr == NULL) {
        return 1;
    }
    *dest = result;
    free(data);
    return 0;
}


/**
 * jsonArray解析整形数字
 *
 * @param json
 * @param index
 * @param dest
 * @return 成功返回0，不成功返回1
 */
int jsonArrayParseInteger(cJSON *json, int index, int *dest) {
    cJSON *item = cJSON_GetArrayItem(json, index);
    if (item == NULL) {
        printf("[JSON Parse Error] Cannot Parse Array[%d]\n", index);
    }
    char *end_ptr, *data = cJSON_Print(item);
    int result = strtol(data, &end_ptr, 10);
    if (end_ptr == NULL) {
        return 1;
    }
    *dest = result;
    free(data);
    return 0;
}


/**
 * json解析浮点数字
 *
 * @param json
 * @param key
 * @param dest
 * @return 成功返回0，不成功返回1
 */
int jsonParseFloat(cJSON *json, char *key, double *dest) {
    cJSON *item = cJSON_GetObjectItem(json, key);
    if (item == NULL) {
        printf("[JSON Parse Error] Cannot Parse key:%s\n", key);
    }
    char *end_ptr, *data = cJSON_Print(item);
    double result = strtod(data, &end_ptr);
    if (end_ptr == NULL) {
        return 1;
    }
    *dest = result;
    free(data);
    return 0;
}


/**
 * 向指定服务器发送请求
 * @param ipAddr IP地址
 * @param port 端口号
 * @param sendData 发送的数据，需要通过UTF-8编码
 * @return 返回接收数据的Data部分，若接收失败或数据无效则返回空字符串
 */
char *sendRequest(char *ipAddr, int port, char *sendData) {
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA data;

    // 初始化Socket
    if (WSAStartup(sockVersion, &data) != 0) {
        printfSocketError("Failed to initialize socket.");
        return NULL;
    }
    // TCP协议
    SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sclient == INVALID_SOCKET) {
        printfSocketError("Invalid Socket.");
        return NULL;
    }

    // 配置服务器IP地址，并尝试链接
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(port);
    serAddr.sin_addr.S_un.S_addr = inet_addr(ipAddr);
    if (connect(sclient, (struct sockaddr *) &serAddr, sizeof(serAddr)) == SOCKET_ERROR) {
        // 链接失败，退出
        printfSocketError("Failed to get response.");
        closesocket(sclient);
        return NULL;
    }

    // 链接成功，发送信息
    send(sclient, sendData, (int) strlen(sendData), 0);

    // 接收数据

    // 签名数据，代表该数据是有效的
    char signData[11];
    int ret = recv(sclient, signData, 10, 0);
    if (ret > 0) {
        signData[ret] = 0x00;
        if (strcmp(signData, "socket 0.1") != 0) {
            printfSocketError("Invalid Response (Invalid Signature).");
            return NULL;
        }
    }

    // 获取数据长度，该数据长度为10，代表后面数据的大小，用于开辟变长数组
    char lengthData[11];
    int dataLength = 0;
    ret = recv(sclient, lengthData, 10, 0);
    if (ret == 10) {
        for (int i = 9; i >= 0; i--) dataLength += (int) pow(10, 9 - i) * (lengthData[i] - '0');
    } else {
        printfSocketError("Invalid Response (Invalid Data Length).");
        return NULL;
    }

    // 开辟空间用于接收数据（JSON格式）
    char *respData = calloc(dataLength + 100, sizeof(char));
    unsigned long long total_received = 0;
    // 因为TCP在数据过大时分包发送，一次可能发不完，所以需要多次获取
    do {
        char *tmp = calloc(dataLength + 100, sizeof(char));
        ret = recv(sclient, tmp, dataLength, 0);
        total_received += ret;
        strcat(respData, tmp);
        free(tmp);
    } while (total_received < dataLength && ret > 0);
    if (total_received != dataLength) {
        printfSocketError("Invalid Response (Wrong Data Length).");
        return NULL;
    }
    closesocket(sclient);
    WSACleanup();
    return respData;
}

/**
 * 解析响应数据
 * @param raw_resp
 * @return 返回响应数据结构体
 */
ResponseData parseResponse(char *raw_resp) {
    ResponseData resp = {0, 0, -500, NULL};
    resp.raw_string = malloc(0);
    if (raw_resp == NULL) {
        printfSocketError("Failed to Parse Response Data (Can't receive response).");
        return resp;
    }
    int str_len = (int) strlen(raw_resp);
    if (str_len == 0) {
        printfSocketError("Failed to Parse Response Data (Response is empty).");
        return resp;
    }
    char *pointer = raw_resp;
    // 获取起始时间戳
    int ts = parseInteger(&pointer);
    if (ts == -2147483648) {
        printfSocketError("Failed to Parse Response Data (Failed to get start ts).");
        return resp;
    }
    resp.start_resp_ts = ts;
    // 指针后移，获取终止时间戳
    pointer++;
    ts = parseInteger(&pointer);
    if (ts == -2147483648) {
        printfSocketError("Failed to Parse Response Data (Failed to get finish ts).");
        return resp;
    }
    resp.finish_resp_ts = ts;
    // 指针后移，获取状态码
    pointer++;
    ts = parseInteger(&pointer);
    if (ts == -2147483648) {
        printfSocketError("Failed to Parse Response Data (Failed to get status code).");
        return resp;
    }
    resp.status_code = ts;
    pointer++;
    // 将剩余数据存入raw_string中
    int str_size = (int) strlen(pointer); // 剩余部分的数据长度
    resp.raw_string = malloc(str_size + 10);
    memset(resp.raw_string, 0, str_size + 10);
    strcpy(resp.raw_string, pointer);
    // 将剩余数据解析为JSON
    resp.json = cJSON_Parse(resp.raw_string);
    return resp;
}


/**
 * 调用后端函数
 *
 * @param action 请求的路径
 * @param json 请求的json内容
 * @return
 */
ResponseData callBackend(const char *action, cJSON *json) {
    char *outputStr = cJSON_PrintUnformatted(json);
    int outputStrLen = (int) strlen(outputStr);
    char requestTime[20] = {0};
    sprintf(requestTime, "%d", (int) time(NULL));
    unsigned long long offset_length =
            strlen(GLOBAL_SOFTWARE_NAME) + strlen(GLOBAL_windows_version) + strlen(requestTime) +
            strlen(GLOBAL_SOFTWARE_VERSION) + 200 + strlen(action) + strlen(GLOBAL_user_info.jwt);
    char *requestStr = malloc(offset_length + outputStrLen);
    memset(requestStr, 0, offset_length + outputStrLen);
    char *signStr = malloc(offset_length + outputStrLen);
    memset(signStr, 0, offset_length + outputStrLen);
    // sign计算方法： HMAC_SHA256(json+timestamp, deviceID)
    memset(signStr, 0, offset_length + outputStrLen);
    strcpy(signStr, outputStr);
    strcat(signStr, requestTime);
    char *signature = calcHexHMACSHA256(signStr, GLOBAL_device_uuid);
    sprintf(requestStr, "socket 0.1\n%s Windows_%s %s %s %s\n%s\n%s\n%s", GLOBAL_SOFTWARE_NAME, GLOBAL_windows_version,
            requestTime, GLOBAL_SOFTWARE_VERSION, signature,
            strlen(GLOBAL_user_info.jwt) > 0 ? GLOBAL_user_info.jwt : "null", action,
            outputStr);
    char *resp_string = sendRequest(GLOBAL_server_ip, GLOBAL_server_port, requestStr);
    ResponseData resp = parseResponse(resp_string);
    free(signature);
    free(outputStr);
    free(requestStr);
    free(signStr);
    if ((resp.status_code == 101 || resp.status_code == 102) && strcmp(action, "user.getUserInfo") != 0) {
        // 登陆过期或Token无效时，需要重新登录
        char filePath[260];
        GetModuleFileName(NULL, filePath, 260); // 获取原先进程的运行位置
        printf("Login Session Expired, Press Any Key to Restart the Program.\n");
        getchar();
        char command[300];
        sprintf(command, "start %s /NOWAIT", filePath); // 使用NOWAIT参数，可以防止前一个进程还在等待
        system(command);
        exit(1);
    }
    if (resp_string != NULL) free(resp_string);
    return resp;
}