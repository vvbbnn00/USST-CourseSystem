//
// Created by admin on 2022/6/23.
//

#include "global.h"
#include "sysinfo.h"
#include "hmacsha256.h"

#include <windows.h>
#include <Commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basetsd.h>
#include <minwindef.h>
#include <sysinfoapi.h>

#define GET_DISK_INFO "wmic diskdrive get serialnumber" // Windows系统中，获取硬盘序列号的指令


/**
 * 执行system命令行，然后将获取到的数据存在char内，返回
 * @param cmd 命令行
 * @return 命令行执行的结果
 */
char *getSystem(const char *cmd) {
    char result[10240] = {0};
    char buf[1024] = {0};
    FILE *fp = NULL;
    if ((fp = popen(cmd, "r")) == NULL) {
        printf("popen error!\n");
        return malloc(0);
    }
    while (fgets(buf, sizeof(buf), fp)) {
        strcat(result, buf);
    }
    pclose(fp);
    char *retData = malloc(strlen(result) + 1);
    strcpy(retData, result);
    return retData;
}

/**
 * 获取硬盘数据
 * @return
 */
char *getDiskInfo() {
    // 此处获取硬盘序列号还可以使用与汇编相关的API，但过于复杂，不如抓取命令行结果来的方便
    return getSystem(GET_DISK_INFO);
}

/**
 * 获取设备UUID
 * @return 设备uuid
 */
char *getDeviceUUID() {
    char *disk_info = getDiskInfo();
    char *uuid = calcHexHMACSHA256(GLOBAL_SOFTWARE_NAME, disk_info);
    free(disk_info);
    return uuid;
}


/**
 * 获取Windows版本号
 * @return
 */
char *getWindowsVersion() {
    DWORD dwVersion;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuild = 0;

    dwVersion = GetVersion();

    // Get the Windows version.

    dwMajorVersion = (DWORD) (LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD) (HIBYTE(LOWORD(dwVersion)));

    // Get the build number.

    if (dwVersion < 0x80000000)
        dwBuild = (DWORD) (HIWORD(dwVersion));

    char *version_info = malloc(50);

    sprintf(version_info, "%d.%d(%d)",
            dwMajorVersion,
            dwMinorVersion,
            dwBuild);
    return version_info;
}


/**
 * 打开文件对话框
 *
 * @param path 最终返回路径
 * @param fileType 文件类型，用\0分割，例如：All *.* Text *.TXT
 * @return 1 - 成功 0 - 失败
 */
int openFileDialog(char *path, const char *fileType, char *title) {
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = fileType;
    ofn.lpstrFileTitle = title;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    return GetOpenFileName(&ofn);
}
