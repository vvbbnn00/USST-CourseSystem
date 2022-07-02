//
// Created by admin on 2022/7/2.
//

#ifndef PROJECT_UI_GBK_H
#define PROJECT_UI_GBK_H

extern void setTitle();

extern void printErrorData(char *msg);

extern void printInMiddle_GBK(char *msg, int width, ...);

extern int inputStringWithRegexCheck(char *message, char *pattern, char *_dest, int max_length);

extern int inputIntWithRegexCheck(char *message, char *pattern, int *_dest);

extern int inputFloatWithRegexCheck(char *message, char *pattern, double *_dest);

extern void selfPlusPrint(char *format, int *counter, int selected, ...);

extern void ui_printHeader_GBK(int width);

#endif //PROJECT_UI_GBK_H
