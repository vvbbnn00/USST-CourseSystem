//
// Created by admin on 2022/6/23.
//

#ifndef PROJECT_USER_H
#define PROJECT_USER_H

extern int Serv_login(char status);

extern int Serv_getLoginSession();

extern void Serv_logout();

extern char *getUserRole(int role);

extern void Serv_changePassword();

extern void printAllUsers();

typedef struct user {
    char uid[21], // 用户ID
    name[21],// 用户姓名
    passwd[65], // 密码
    last_login_ip[201]; // 用户最后一次登录IP
    int role,// 用户角色
    status; // 用户状态 0-启用 1-停用
    long long last_login_time; // 最后一次登录时间戳
} User;


#endif //PROJECT_USER_H
