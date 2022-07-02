//
// Created by admin on 2022/6/25.
//

#include "ui_utf8.h"
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "user.h"
#include "course.h"
#include <stdlib.h>
#include "system_config.h"


/**
 * UI模块，打印程序名称、版本号等信息
 */
void ui_printHeader() {
    printf("\n ----------------------------\n");
    printf("|        学生选课系统        |\n");
    printf("|   ");
    printf(VERSION);
    printf("   | \n");
    printf(" ----------------------------\n");
    if (strcmp(GLOBAL_school, "大学") != 0) {
        printf("\n%20s %9s学期\n\n", GLOBAL_school, GLOBAL_semester);
    }
}

/**
 * UI模块，打印用户信息
 */
void ui_printUserInfo() {
    printf("\n-------- 用户信息 --------\n\n - 用户名： %s\n - 姓  名： %s\n - 角  色： %s\n\n",
           GLOBAL_user_info.userid,
           GLOBAL_user_info.name,
           getUserRole(GLOBAL_user_info.role));
}

/**
 * 居中输出消息
 * @param msg 消息
 * @param width 宽度
 */
void printInMiddle(char *msg, int width, int ascii_number) {
    unsigned int str_length = (unsigned int) strlen(msg);
    str_length = (str_length - ascii_number) / 3 * 2 + ascii_number; // utf8占3字节，在代码中需要适配
    for (int i = 0; i < (width - str_length) / 2; i++) printf(" ");
    printf("%s\n", msg);
}


/**
 * 按照左右两栏分栏形式输出菜单选项
 * @param msg
 * @param current
 * @param utf8_count utf8字符数，需要转换
 */
void printChoices(char *msg, int *current, int utf8_count) {
    char fmt[50] = "%-30s";
    sprintf(fmt, "%%-%ds", 30 + utf8_count);
    printf(fmt, msg);
    if (*current % 2 == 0) {
        printf("\n");
    }
    (*current)++;
}

/**
 * 对传入的指令进行操作
 * @param command
 */
void ui_doMainMenuActions(int command) {
    switch (command) {
        case 1: // 学生选课
            printStudentCourseSelection();
            break;
        case 2: // 学生课表查看
            printStudentLectureTable();
            break;
        case 11: // 教师管理课程
            printAllCourses(1);
            break;
        case 3: // 查看全校课表
        case 12:
        case 21:
            printAllCourses(0);
            break;
        case 22: // 用户管理
            printAllUsers();
            break;
        case 23: // 系统信息管理
            printSemesterData();
            break;
        case 4:// 修改密码
        case 13:
        case 24:
            Serv_changePassword();
            break;
        case 5:// 退出登录
        case 14:
        case 25:
            Serv_logout();
            longjmp(GLOBAL_goto_login, 1); // 跳转，无需break
        case 6:// 退出程序
        case 15:
        case 26:
            exit(0); // exit 无需break
        default:
            return;
    }
}


/**
 * 按照权限输出主菜单模块
 * @param wrong_command 是否在先前输入过错误指令
 */
int ui_printMainMenu(char wrong_command) {
    init:
    system("chcp 65001>nul & MODE CON COLS=55 LINES=30 & cls");
    ui_printHeader();
    ui_printUserInfo();
    printf("============= 主菜单 - 请键入指令 =============\n\n");
    int INDEX = 1;
    switch (GLOBAL_user_info.role) {
        case 0: // 学生
            printChoices("(1) 学生选课", &INDEX, 4);
            printChoices("(2) 查看当前课表", &INDEX, 6);
            printChoices("(3) 全校课程信息一览", &INDEX, 8);
            printChoices("(4) 修改密码", &INDEX, 4);
            printChoices("(5) 退出登录", &INDEX, 4);
            printChoices("(6) 退出程序", &INDEX, 4);
            break;
        case 1: // 教师
            printChoices("(1) 授课课程信息管理", &INDEX, 8);
            printChoices("(2) 全校课程信息一览", &INDEX, 8);
            printChoices("(3) 修改密码", &INDEX, 4);
            printChoices("(4) 退出登录", &INDEX, 4);
            printChoices("(5) 退出程序", &INDEX, 4);
            break;
        case 2: // 管理员
            printChoices("(1) 课程信息管理", &INDEX, 6);
            printChoices("(2) 用户管理", &INDEX, 4);
            printChoices("(3) 系统信息管理", &INDEX, 6);
            printChoices("(4) 修改密码", &INDEX, 4);
            printChoices("(5) 退出登录", &INDEX, 4);
            printChoices("(6) 退出程序", &INDEX, 4);
            break;
        default:
            longjmp(GLOBAL_goto_login, 2);
    }
    if (INDEX % 2 == 0) printf("\n");
    printf("\n===============================================\n");
    if (wrong_command) {
        printf("您输入的指令有误，请重新输入：");
        wrong_command = 0;
    } else { printf("请输入命令编号："); }
    char command_raw[11] = {0};
    gets(command_raw);
    fflush(stdin); // 清除多余的无效指令，防止影响后续操作
    if (strlen(command_raw) != 1) {
        wrong_command = 1;
        goto init;
    }
    int command = (int) command_raw[0];
    if (command >= '0' && command <= '9') {
        command += GLOBAL_user_info.role * 10 - '0';
        ui_doMainMenuActions(command);
        goto init;
    } else {
        wrong_command = 1;
        goto init;
    }
}


