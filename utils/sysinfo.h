//
// Created by admin on 2022/6/23.
//

#ifndef PROJECT_SYSINFO_H
#define PROJECT_SYSINFO_H

extern char *getDeviceUUID();

extern char *getWindowsVersion();

extern int openFileDialog(char *path, const char *fileType, char *title);

#endif //PROJECT_SYSINFO_H
