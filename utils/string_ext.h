//
// Created by admin on 2022/6/27.
//

#ifndef PROJECT_STRING_EXT_H
#define PROJECT_STRING_EXT_H

#include <time.h>

extern char *getFormatTimeString(time_t timestamp);

extern char *getFormatTimeString_(char *format, time_t timestamp);

extern void gets_safe(char *str, int num);

extern char regexMatch(char *pattern, char *string);

#endif //PROJECT_STRING_EXT_H
