//
// Created by admin on 2022/6/23.
//
#include <stdlib.h>
#include "global.h"
#include "socket.h"
#include "user.h"
#include "ui_utf8.h"
#include "ui_gbk.h"
#include <setjmp.h>
#include <Windows.h>

int main() {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x07); // 设置标准颜色
    setTitle(); // 设置标题
    loadBasicInformation(); //程序初始化

    system("MODE CON COLS=55 LINES=30>nul & cls");

    char session_status = (char) Serv_getLoginSession(); // 自动登录
    char status = setjmp(GLOBAL_goto_login); // 全局标记点，用于退出登录或登录过期时跳转
    if (status != 0) {
        Serv_login(status);
    } else {
        switch (session_status) {
            case 1: // Session文件不存在
                Serv_login(0);
                break;
            case 2: // Session登录失败
                Serv_login(2);
                break;
            default:
                break;
        }
    }

    // 打印主菜单
    ui_printMainMenu(0);

    // 理论上该代码不会被访问到
    system("pause");
    return 0;
}
