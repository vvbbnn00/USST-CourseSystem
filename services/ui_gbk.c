//
// Created by admin on 2022/7/2.
//

#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <stdarg.h>
#include <string.h>
#include <Windows.h>
#include "pcre2.h"
#include "ui_gbk.h"
#include "global.h"
#include "string_ext.h"

#define TITLE() system("title 学生选课系统 - " VERSION)

/**
 * 设置程序标题（GBK）
 */
void setTitle() {
    TITLE();
}

/**
 * 输出错误信息
 * @param msg
 */
void printErrorData(char *msg) {
    printf("[系统错误] %s（按任意键继续）\n", msg);
    getch();
}


/**
 * 居中输出消息
 * @param msg 消息
 * @param width 宽度
 */
void printInMiddle_GBK(char *msg, int width, ...) {
    unsigned int str_length = (unsigned int) strlen(msg);
    char *final_str = calloc(str_length + 1000, sizeof(char));
    // 先将格式化的结果输出到字符串中
    va_list args;
    va_start(args, width);
    vsnprintf(final_str, str_length + 1000, msg, args);
    va_end(args);
    // 计算真正需要占用的长度
    str_length = strlen(final_str);
    if (width - (int) str_length > 0) {
        for (int i = 0; (i < (width - str_length) / 2); i++) printf(" ");
    }
    printf("%s\n", final_str);
}


/**
 * 正则表达式读入数据并验证
 * @param message 提示信息
 * @param pattern 正则表达式
 * @param _dest 目标字符串
 * @param max_length 最大读入长度
 *
 * @return 0 正常 -1 用户取消
 */
int inputStringWithRegexCheck(char *message, char *pattern, char *_dest, int max_length) {
    char *input_string;
    printf(message);

    InputString:
    printf("[请输入数据(Esc取消)] ");
    input_string = calloc(max_length + 1, sizeof(char));
    int str_len = 0, t; //记录长度
    while (1) {
        t = _getch();
        if (t == 27) {
            return -1; // 用户退出
        }
        if (t == '\r' || t == '\n') { //遇到回车，表明输入结束
            printf("\n");
            break; //while 循环的出口
        } else if (t == '\b') { //遇到退格，需要删除前一个星号
            if (str_len <= 0) continue; // 如果长度不足，则不执行退格操作
            printf("\b \b");  //退格，打一个空格，再退格，实质上是用空格覆盖掉星号
            --str_len;
        } else {
            if (str_len >= max_length) continue; // 限制输入长度
            input_string[str_len++] = (char) t;//将字符放入数组
            printf("%c", t);
        }
    }

    char *final_str = GBKToUTF8(input_string); // 需转换成UTF8格式
    if (regexMatch(pattern, final_str) == 0) {
        free(input_string);
        printf("输入的数据不符合规则，请重新输入\n");
        goto InputString;
    }

    strcpy(_dest, final_str);
    free(input_string);
    return 0;
}


/**
 * 正则表达式读入数据并验证
 * @param message 提示信息
 * @param pattern 正则表达式
 * @param _dest 目标字符串
 * @param max_length 最大读入长度
 *
 * @return 0 正常 -1 用户取消
 */
int inputIntWithRegexCheck(char *message, char *pattern, int *_dest) {
    char *input_string;
    InputInteger:
    input_string = calloc(12, sizeof(char));
    if (inputStringWithRegexCheck(message, pattern, input_string, 11) == -1) {
        return -1;
    }
    char *end_ptr;
    int result = strtol(input_string, &end_ptr, 10);
    if (*end_ptr != 0) {
        printf("请输入正确的数字\n");
        free(input_string);
        goto InputInteger;
    }
    *_dest = result;
    return 0;
}

/**
 * 正则表达式读入数据并验证（小数）
 * @param message 提示信息
 * @param pattern 正则表达式
 * @param _dest 目标字符串
 * @param max_length 最大读入长度
 *
 * @return 0 正常 -1 用户取消
 */
int inputFloatWithRegexCheck(char *message, char *pattern, double *_dest) {
    char *input_string;
    InputFloat:
    input_string = calloc(12, sizeof(char));
    if (inputStringWithRegexCheck(message, pattern, input_string, 11) == -1) {
        return -1;
    }
    char *end_ptr;
    double result = strtof(input_string, &end_ptr);
    if (*end_ptr != 0) {
        printf("请输入正确的数字\n");
        free(input_string);
        goto InputFloat;
    }
    *_dest = result;
    return 0;
}

/**
 * 自增计数输出
 *
 * @param format
 * @param counter
 * @param selected 需要高亮的行
 * @param args
 */
void selfPlusPrint(char *format, int *counter, int selected, ...) {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (*counter == selected) {
        SetConsoleTextAttribute(windowHandle, 0x70);
    }

    va_list args;
    va_start(args, selected);
    // 不固定参数通过va_list保存，使用vprintf输出，可更好地封装print函数
    vprintf(format, args);
    va_end(args);

    SetConsoleTextAttribute(windowHandle, 0x07);
    (*counter)++;
}


/**
 * UI模块，打印程序名称、版本号等信息
 */
void ui_printHeader_GBK(int width) {
    printInMiddle_GBK("----------------------------", width);
    printInMiddle_GBK("|        学生选课系统        |", width);
    printInMiddle_GBK("|   %s   |", width, VERSION);
    printInMiddle_GBK("----------------------------", width);
}

