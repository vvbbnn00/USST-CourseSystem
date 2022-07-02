//
// Created by admin on 2022/6/23.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "sysinfo.h"
#include "socket.h"
#include "ui_utf8.h"
#include <setjmp.h>
#include <stringapiset.h>
#include <conio.h>

jmp_buf GLOBAL_goto_login;
char GLOBAL_school[255] = "大学";
char GLOBAL_semester[50] = "学期";
char GLOBAL_device_uuid[66] = {0};
char GLOBAL_windows_version[64] = {0};
char GLOBAL_server_ip[128] = {0};
int GLOBAL_server_port;
UserInfo GLOBAL_user_info;

/**
 * 暂停
 */
void pause() {
    system("chcp 65001 & cls");
    ui_printHeader();
    printf("[系统提示] 程序退出...\n");
//    _getch();
//    system("pause");
}

/**
 * 加载基本信息，主要包括系统信息
 */
void loadBasicInformation() {
    system("chcp 65001>nul & cls"); // 切换页面文件为UTF-8编码
    atexit(pause);// 注册程序终止时暂停

    printf("程序开始初始化...\n");

    // 获取设备码和系统版本
    printf("正在获取设备信息...\n");
    char *t = getDeviceUUID();
    strcpy(GLOBAL_device_uuid, t);
    free(t);
    printf("Device-UUID: %s\n", GLOBAL_device_uuid);
    t = getWindowsVersion();
    strcpy(GLOBAL_windows_version, t);
    free(t);

    // 获取服务器地址
    printf("加载服务器地址配置文件中...\n");
    FILE *fp = fopen(SERVER_CONFIG_PATH, "r");
    if (!fp) {
        printf("服务器地址配置文件不存在，默认写入127.0.0.1:2333\n");
        fp = fopen(SERVER_CONFIG_PATH, "w");
        if (!fp) {
            printf("[ERROR] 文件写入失败(%s)\n", SERVER_CONFIG_PATH);
            exit(-1);
        }
        fprintf(fp, "# 服务器连接地址配置，格式为[IP地址] [端口]，IP地址和端口间由一个空格分隔（本行不会被程序读入，但请勿删除本行）\n");
        fprintf(fp, "127.0.0.1 2333\n");
        strcpy(GLOBAL_server_ip, "127.0.0.1");
        GLOBAL_server_port = 2333;
        fclose(fp);
    } else {
        // 后移一行
        char *tmp = malloc(1000);
        fgets(tmp, 1000, fp);
        free(tmp);
        char port[10];
        fscanf(fp, "%s %s", GLOBAL_server_ip, port);
        port[9] = 0;
        GLOBAL_server_port = (int) strtol(port, NULL, 10);
        fclose(fp);
    }

    // 尝试连接远程服务器，获取远程服务器状态
    printf("服务器地址加载成功，正在尝试连接到%s:%d...\n", GLOBAL_server_ip, GLOBAL_server_port);
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "message", cJSON_CreateString("Hello SocketServer"));
    ResponseData resp = callBackend("status.get_server_status", json);
    cJSON_Delete(json);
    if (resp.status_code != 0) {
        if (resp.status_code == -500) {
            printf("服务器连接失败，错误码：%d, 网络错误（按任意键退出）\n", resp.status_code);
        } else {
            printf("服务器连接失败，错误码: %d, %s（按任意键退出）\n", resp.status_code,
                   cJSON_Print(cJSON_GetObjectItem(resp.json, "message")));
        }
        _getch();
        exit(-1);
    } else {
        jsonParseString(resp.json, "school", GLOBAL_school);
        jsonParseString(resp.json, "semester", GLOBAL_semester);
        printf("服务器连接成功。\n");
    }
    if (resp.json != NULL) cJSON_Delete(resp.json);
    if (resp.raw_string != NULL)free(resp.raw_string);
    printf("程序初始化完毕\n");
}

/**
 * 查找可能的错误原因
 * @param resp
 * @return
 */
char *findExceptionReason(ResponseData *resp) {
    if (resp->status_code) {
        if (resp->status_code == -500) {
            return "网络连接失败";
        } else {
            cJSON *node = cJSON_GetObjectItem(resp->json, "message");
            if (node == NULL) return "未知错误";
            return (char *) cJSON_Print(node);
        }
    } else {
        return "";
    }
}


/**
 * UTF8转GBK编码
 *
 * @param strUTF8
 * @return
 */
char *UTF8ToGBK(const char *strUTF8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8, -1, NULL, 0);
    wchar_t wszGBK[len + 1];
    memset(wszGBK, 0, len + 1);
    MultiByteToWideChar(CP_UTF8, 0, (LPCTSTR) strUTF8, -1, wszGBK, len);
    len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
    char *szGBK = malloc(len + 1);
    memset(szGBK, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
    return szGBK;
}

/**
 * UTF8转GBK编码
 *
 * @param strUTF8
 * @return
 */
char *GBKToUTF8(const char *strGBK) {
    int len = MultiByteToWideChar(CP_ACP, 0, strGBK, -1, NULL, 0);
    wchar_t wstr[len + 1];
    memset(wstr, 0, (len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, strGBK, -1, wstr, len);
    len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char str[len + 1];
    memset(str, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    char *strTemp = malloc(len + 1);
    strcpy(strTemp, str);
    return strTemp;
}

