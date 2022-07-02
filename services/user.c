//
// Created by admin on 2022/6/23.
//
// 特别说明：因为该页面涉及到的部分操作需要实现过滤中文字符等操作，故使用GBK编码

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include "socket.h"
#include "global.h"
#include <time.h>
#include "string_ext.h"
#include "utf8_words.h"
#include "ui_utf8.h"
#include "AES.h"
#include "hmacsha256.h"
#include "Windows.h"
#include "link_list_object.h"
#include "ui_gbk.h"
#include "user.h"

const char *USER_ROLE_MAP[] = {"学生", "教师", "管理员"};
const char *USER_STATUS_MAP[] = {"启用", "停用"};
const char *SORT_METHOD_USER[] = {"UID升序", "UID降序", "角色升序", "角色降序"};

/**
 * 判断该字符是否为可用字符
 * @param c
 * @return 可用则返回1，不可用则返回0
 */
char inAvailableCharset(char c) {
    const char *available_char = "`~!@#$%^&*()_+-= []{}\\|;:'\",.<>?/";
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return 1;
    for (int i = 0; i < 33; i++) if (c == available_char[i]) return 1;
    return 0;
}


/**
 * 获取密码，密码密文显示
 * @param do_exit 若esc取消，则退出
 * @return
 */
char *getPassword(char do_exit) {
    char *password = malloc(33); //存储密码
    memset(password, 0, 33);
    int i = 0, t; //记录密码长度
    char c; //用于实现密码隐式输入
    while (1) {
        t = _getch();
        if (t == 27) {
            if (do_exit) {
                exit(0);
            } else {
                return "";
            }
        }
        if (t > 127) continue; // 非ASCII字符，不输入
        c = (char) t; //用 _getch() 函数输入，字符不会显示在屏幕上
//        printf("%c, %d\n", c);
        if (c == '\r' || c == '\n') { //遇到回车，表明密码输入结束
            printf("\n");
            break; //while 循环的出口
        } else if (c == '\b') { //遇到退格，需要删除前一个星号
            if (i <= 0) continue; // 如果长度不足，则不执行退格操作
            printf("\b \b");  //退格，打一个空格，再退格，实质上是用空格覆盖掉星号
            --i;
        } else {
            if (inAvailableCharset(c)) {
                if (i >= 30) continue; // 限制密码长度
                password[i++] = c;//将字符放入数组
                printf("*");
            }
        }
    }
    return password;
}


/**
 * 获取用户名，按回车后检查
 * @return
 */
char *getUsername() {
    char *username = malloc(21); // 用户名最长20位
    memset(username, 0, 21);
    int i = 0, t;
    char c;
    while (1) {
        t = _getch();
        if (t == 27) exit(0);
        if (t > 127) continue; // 非ASCII字符，不输入
        c = (char) t; //用 _getch() 函数输入，字符不会显示在屏幕上
//        printf("%c, %d\n", c);
        if (c == '\r' || c == '\n') { //遇到回车，表明密码输入结束
            printf("\n");
            break; //while 循环的出口
        } else if (c == '\b') { //遇到退格，需要删除前一个星号
            if (i <= 0) continue; // 如果长度不足，则不执行退格操作
            printf("\b \b");  //退格，打一个空格，再退格，实质上是用空格覆盖掉星号
            --i;
        } else {
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) { // 用户名只允许字母数字组合形式，其他内容不键入
                if (i >= 20) continue; // 限制用户名长度
                username[i++] = c;//将字符放入数组
                printf("%c", c);
            }
        }
    }
    return username;
}


/**
 * 获取用户角色
 * @return 用户角色
 */
char *getUserRole(int role) {
    switch (role) {
        case 0:
            return WORDS_Login_role_student;
        case 1 :
            return WORDS_Login_role_teacher;
        case 2:
            return WORDS_Login_role_admin;
        default:
            return WORDS_Login_role_unknown;
    }
}


/**
 * 保存当前登录session
 * @return 成功返回1，失败返回0
 */
int Serv_saveLoginSession() {
    FILE *fp = fopen(USER_SESSION, "wb+");
    if (!fp) {
        return 0;
    }
    char key[65];
    strcpy(key, GLOBAL_device_uuid);
    unsigned long long size = sizeof(GLOBAL_user_info);
    unsigned char *source = malloc(size + 16); // 预留16位用于补位
    memcpy(source, &GLOBAL_user_info, size);
    unsigned long long padding_length; // 补齐后长度
    AES_Init(key);
    padding_length = AES_add_pkcs7Padding(source, size);
    unsigned char cipherText[padding_length];
    AES_Encrypt(source, cipherText, padding_length, NULL); // AES加密，秘钥为UUID
    fwrite(cipherText, sizeof(unsigned char), padding_length, fp);
    fclose(fp);
    free(source);
    return 1;
}


/**
 * 从文件获取登录Session
 * @return 获取session成功返回0，获取失败返回1 - 文件不存在 2 - 登录失效
 */
int Serv_getLoginSession() {
    FILE *fp = fopen(USER_SESSION, "rb+");
    if (!fp) {
        return 1; // 文件不存在
    }
    printf(WORDS_Login_trying_autologin);
    // 读取Session文件

    char key[65];
    strcpy(key, GLOBAL_device_uuid);
    AES_Init(key);
    char *raw_data = NULL, char_tmp;
    int raw_data_length = 0;
    while (fread(&char_tmp, sizeof(char), 1, fp) != 0) {
        raw_data_length++;
        raw_data = realloc(raw_data, raw_data_length * sizeof(char));
        raw_data[raw_data_length - 1] = char_tmp;
    }
    fclose(fp);
    unsigned char *plainData = calloc(raw_data_length, sizeof(char));
    unsigned char *raw = calloc(raw_data_length, sizeof(unsigned char));
    memcpy(raw, raw_data, raw_data_length);
    AES_Decrypt(plainData, raw, raw_data_length, NULL); // AES解密
    AES_delete_pkcs7Padding(plainData, raw_data_length);
    memcpy(&GLOBAL_user_info, plainData, sizeof(GLOBAL_user_info));
    free(plainData);
    free(raw);

    if (time(NULL) > GLOBAL_user_info.expired) {
        // 登录过期
        memset(&GLOBAL_user_info, 0, sizeof(GLOBAL_user_info));
        return 2;
    }

    // 发送请求
    cJSON *req_json = cJSON_CreateObject();
    ResponseData ret_data = callBackend("user.getUserInfo", req_json);
    cJSON_Delete(req_json);
    if (ret_data.status_code != 0) {  // 登录失败
        memset(&GLOBAL_user_info, 0, sizeof(GLOBAL_user_info));
        GLOBAL_user_info.role = 3;
        return 2;
    }

    ui_printHeader();
    printf(WORDS_Login_success, GLOBAL_user_info.userid, GLOBAL_user_info.name,
           getUserRole(GLOBAL_user_info.role), getFormatTimeString(GLOBAL_user_info.expired), GLOBAL_user_info.name);
    _getch();
    return 0;
}


/**
 * 执行登录操作
 * @param status 表示本次登录是否额外携带状态
 */
int Serv_login(char status) {
    char *username, *password;
    Login: // 设置标签，若密码错误，则返回重新登录
    system("cls");
    system("@echo off");
    system("chcp 936 > nul & MODE CON COLS=55 LINES=30"); // _getch有bug，需要转换为GBK编码

    printf("\n ----------------------------\n");
    printf("|        学生选课系统        |\n");
    printf("|   ");
    printf(VERSION);
    printf("   | \n");
    printf(" ----------------------------\n");

    switch (status) {
        case 1:
            printf("[系统提示] 您已成功退出登录。\n");
            break;
        case 2:
            printf("[系统提示] 您的登录状态已过期，请重新登录。\n");
            break;
        default:
            break;
    }
    status = 0; // 状态是一次性的

    printf("\n---- 用户登录 - （按\"esc\"键取消） ----\n\n");
    printf("请输入用户名（按回车键确认）：\n");
    username = getUsername();
    if (strlen(username) == 0) {
        goto Login;
    }
    printf("请输入密码（按回车键确认）：\n");
    password = getPassword(1);
    if (strlen(password) == 0) {
        goto Login;
    }
    if (!regexMatch(PASSWD_PATTERN, password)) {
        printf("登录失败：密码不正确，请重新输入（按任意键重新登录）。\n");
        _getch();
        goto Login;
    }
    cJSON *req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "username", cJSON_CreateString(username));
    // 密码通过HMAC_SHA256方式先在客户端加密
    cJSON_AddItemToObject(req_json, "password", cJSON_CreateString(calcHexHMACSHA256(password, "course_system")));
    cJSON_AddItemToObject(req_json, "device_uuid", cJSON_CreateString(GLOBAL_device_uuid));
    ResponseData ret_data = callBackend("user.login", req_json);
    cJSON_Delete(req_json);
    if (ret_data.status_code != 0) {
        printf("登录失败（错误码：%d），可能原因是:%s （按任意键重新登录）\n",
               ret_data.status_code,
               UTF8ToGBK(findExceptionReason(&ret_data)));
        _getch();
        cJSON_Delete(ret_data.json);
        free(ret_data.raw_string);
        goto Login;
    }

    free(username);
    free(password);
    system("chcp 65001  & MODE CON COLS=55 LINES=30 & cls"); // 切换回UTF-8编码

    // 解析过期时间
    int ret = jsonParseLong(ret_data.json, "expire", &GLOBAL_user_info.expired);
    if (ret != 0) { // 过期时间解析失败，则默认1小时后登录过期
        GLOBAL_user_info.expired = time(NULL) + 3600;
    }
    // 解析用户名
    jsonParseString(ret_data.json, "username", GLOBAL_user_info.userid);
    // 解析jwt
    jsonParseString(ret_data.json, "jwt", GLOBAL_user_info.jwt);
    // 解析姓名
    jsonParseString(ret_data.json, "name", GLOBAL_user_info.name);
    // 解析角色
    ret = jsonParseInteger(ret_data.json, "role", &GLOBAL_user_info.role);
    if (ret != 0) { // 角色解析失败，则登录终止
        printf(WORDS_Login_parse_role_error);
        return -1;
    }

    ui_printHeader();
    printf(WORDS_Login_success, GLOBAL_user_info.userid, GLOBAL_user_info.name,
           getUserRole(GLOBAL_user_info.role), getFormatTimeString(GLOBAL_user_info.expired), GLOBAL_user_info.name);

    // 保存登录状态
    if (!Serv_saveLoginSession()) {
        printf(WORDS_Login_save_session_error);
    } else {
        printf(WORDS_Login_status_saved);
    }

    cJSON_Delete(ret_data.json);
    free(ret_data.raw_string);

    _getch();
    return 0;
}

/**
 * 退出登录，并删除Session文件，该操作本质上就是删除Session文件，并将全局变量重置，不会发起服务器请求
 */
void Serv_logout() {
    memset(&GLOBAL_user_info, 0, sizeof(GLOBAL_user_info));
    GLOBAL_user_info.role = 3;
    remove(USER_SESSION);
}


/**
 * 执行修改密码操作
 */
void Serv_changePassword() {
    char *ori_password, *new_password, *new_password_repeat;

    Init:

    system("@echo off");
    system("chcp 936 > nul & MODE CON COLS=55 LINES=30 & cls"); // _getch有bug，需要转换为GBK编码

    printf("\n ----------------------------\n");
    printf("|        学生选课系统        |\n");
    printf("|   ");
    printf(VERSION);
    printf("   | \n");
    printf(" ----------------------------\n\n");

    printf("\n---- 修改密码 - （按\"esc\"键或不输入内容以返回） ----\n\n");
    printf("用户名：%s\n", GLOBAL_user_info.userid);
    printf("请输入原密码（按\"esc\"键或不输入内容以返回）：\n");
    ori_password = getPassword(0);
    if (strlen(ori_password) == 0) {
        return;
    }
    // 设置标记点，若密码不符合规则，则跳回此处
    EnterNewPassword:
    printf("请输入新密码：\n");
    printf("[密码强度提示] 密码中至少同时出现字母、数字、字符中的两种符号，长度在8-20位。\n");
    new_password = getPassword(0);
    if (strlen(new_password) == 0) {
        return;
    }
    if (!regexMatch(PASSWD_PATTERN, new_password)) {
        printf("密码强度不符合规则，请重新输入。\n\n");
        goto EnterNewPassword;
    }
    printf("请再次输入新密码：\n");
    new_password_repeat = getPassword(0);
    if (strcmp(new_password_repeat, new_password) != 0) {
        printf("两次密码输入不相同，请重新输入。\n\n");
        goto EnterNewPassword;
    }

    if (!regexMatch(PASSWD_PATTERN, ori_password)) {
        printf("[系统提示] 修改密码失败：原密码不正确（按任意键重新输入）\n");
        _getch();
        goto Init;
    }

    cJSON *req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "ori_password",
                          cJSON_CreateString(calcHexHMACSHA256(ori_password, "course_system")));
    cJSON_AddItemToObject(req_json, "new_password",
                          cJSON_CreateString(calcHexHMACSHA256(new_password, "course_system")));
    ResponseData ret_data = callBackend("user.changePassword", req_json);
    cJSON_Delete(req_json);

    if (ret_data.status_code != 0) {
        printf("[系统提示] 修改密码失败（错误码：%d），可能原因是:%s （按任意键重新输入）\n",
               ret_data.status_code,
               UTF8ToGBK(findExceptionReason(&ret_data)));
        _getch();
        cJSON_Delete(ret_data.json);
        free(ret_data.raw_string);
        goto Init;
    }

    printf("[系统提示] 密码修改成功（按任意键返回主菜单）。\n");
    _getch();
}


/**
 * 解析用户数据，存入指针对应结构体
 *
 * @param json
 * @param _dest
 * @return 0 成功 非零 失败
 */
int parseUserData(cJSON *json, User *_dest) {
    if (jsonParseInteger(json, "role", &_dest->role) ||
        jsonParseLong(json, "last_login_time", &_dest->last_login_time) ||
        jsonParseInteger(json, "status", &_dest->status)) {
        printErrorData("用户信息数据解析失败，请返回重试。");
        return -1;
    }
    jsonParseString(json, "uid", _dest->uid);
    jsonParseString(json, "name", _dest->name);
    jsonParseString(json, "last_login_ip", _dest->last_login_ip);
    return 0;
}


/**
 * 新增用户
 * @return 取消则返回NULL，否则返回用户指针
 */
User *addUser() {
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= 用户·新增用户 =======\n", 71);

    User *user = calloc(1, sizeof(User));
    if (inputStringWithRegexCheck("[新增用户] 请输入用户UID（由3-15位的字母、数字组成）\n",
                                  USER_PATTERN,
                                  user->uid,
                                  15) == -1)
        goto CancelAdd;
    if (inputStringWithRegexCheck("[新增用户] 请设置用户初始密码（由8-20为字母、数字、字符组成，三种类型至少同时出现两种）\n",
                                  PASSWD_PATTERN,
                                  user->passwd,
                                  20) == -1)
        goto CancelAdd;
    if (inputStringWithRegexCheck("[新增用户] 请设置用户姓名（由2-20位的中文、英文等组成）\n",
                                  USER_NAME_PATTERN,
                                  user->name,
                                  20) == -1)
        goto CancelAdd;
    strcpy(user->passwd, calcHexHMACSHA256(GBKToUTF8(user->passwd), "course_system"));
    if (inputIntWithRegexCheck("[新增用户] 请设置用户角色：0-学生、1-教师、2-管理员\n",
                               USER_ROLE_PATTERN,
                               &user->role) == -1)
        goto CancelAdd;
    goto Return;

    CancelAdd:  // 取消添加
    free(user);
    user = NULL;

    Return:  // 返回数据
    return user;
}


/**
 * 新增/修改用户
 *
 * @param _user 当为NULL时，新增用户，否则为修改用户
 * @return
 */
void editUser(User *_user) {
    // 如无用户信息，则新增用户
    int action = 0;
    User *user = _user;

    if (user == NULL) {
        user = addUser();
        action = 1;
        if (user == NULL) {
            return;
        }
    }

    int counter = 0, selected = 0, key;

    EditUser_Refresh:

    counter = 0;
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= 用户·新增/修改用户 =======\n\n", 71);

    selfPlusPrint("\t\t用户UID       %s\n", &counter, selected, UTF8ToGBK(user->uid)); // 0
    selfPlusPrint("\t\t用户姓名      %s\n", &counter, selected, UTF8ToGBK(user->name)); // 1
    selfPlusPrint("\t\t用户角色      %s\n", &counter, selected, USER_ROLE_MAP[user->role]); // 2
    selfPlusPrint("\t\t用户状态      %s\n", &counter, selected, USER_STATUS_MAP[user->status]); // 3
    selfPlusPrint("\t\t最后登录IP    %s\n", &counter, selected, UTF8ToGBK(user->last_login_ip)); // 4
    selfPlusPrint("\t\t最后登录时间  %s\n", &counter, selected, getFormatTimeString(user->last_login_time)); // 5
    selfPlusPrint("\t\t用户密码      [可重置]\n", &counter, selected); // 6

    printf("\n");
    printInMiddle_GBK("<Enter>修改选中行 <Y>提交修改 <Esc>取消修改", 71);

    EditCourse_GetKey:

    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 80: // 下
                    if (selected < 6) selected++;
                    else selected = 0;
                    goto EditUser_Refresh;
                case 72: // 上
                    if (selected > 0) selected--;
                    else selected = 6;
                    goto EditUser_Refresh;
                default:
                    break;
            }
            break;
        case 13:
            switch (selected) {
                case 1:
                    inputStringWithRegexCheck("[修改用户] 请设置用户姓名（由2-20位的中文、英文等组成）\n",
                                              USER_NAME_PATTERN,
                                              user->name,
                                              20);
                    break;
                case 2:
                    inputIntWithRegexCheck("[修改用户] 请设置用户角色：0-学生、1-教师、2-管理员\n",
                                           USER_ROLE_PATTERN,
                                           &user->role);
                    break;
                case 3:
                    user->status = user->status == 0 ? 1 : 0;
                    printf("[修改用户] 用户的账户状态已设置为 %s\n", USER_ROLE_MAP[user->status]);
                    break;
                case 6:
                    inputStringWithRegexCheck("[修改用户] 请设置用户密码（由8-20为字母、数字、字符组成，三种类型至少同时出现两种）\n",
                                              PASSWD_PATTERN,
                                              user->passwd,
                                              20);
                    break;
                default:
                    break;
            }
            goto EditUser_Refresh;
        case 'y':
        case 'Y': {
            printf("\n[提示] 正在请求服务器...\n");
            cJSON *req_json = cJSON_CreateObject();
            cJSON_AddItemToObject(req_json, "action", cJSON_CreateNumber(action));
            cJSON_AddItemToObject(req_json, "uid", cJSON_CreateString(user->uid));
            cJSON_AddItemToObject(req_json, "passwd", cJSON_CreateString(user->passwd));
            cJSON_AddItemToObject(req_json, "name", cJSON_CreateString(user->name));
            cJSON_AddItemToObject(req_json, "role", cJSON_CreateNumber(user->role));
            cJSON_AddItemToObject(req_json, "status", cJSON_CreateNumber(user->status));
            ResponseData resp = callBackend("user.submit", req_json);
            free(req_json);
            free(resp.raw_string);
            if (resp.status_code != 0) {
                printf("[提交失败] 错误码：%d, %s(按任意键继续)\n", resp.status_code, UTF8ToGBK(findExceptionReason(&resp)));
                getch();
                cJSON_Delete(resp.json);
                goto EditUser_Refresh;
            }
            cJSON_Delete(resp.json);
            printf("[提交成功] 用户修改/新增成功（按任意键返回用户列表）\n");
            getch();
            goto GC_COLLECT;
        }
            break;
        case 27:
            goto GC_COLLECT;
        default:
            break;
    }
    goto EditCourse_GetKey;

    GC_COLLECT:
    if (_user == NULL) free(user);
}


/**
 * 输出全体用户信息
 */
void printAllUsers() {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    char search_kw[36] = "";
    int sort_method = 0; // 排序方法 0 - UID升序 1 - UID降序 2 - 角色升序 3 - 角色降序
    int total, page = 1, max_page, page_size = 15, current_total;

    LinkList_Node *selectedRow = NULL; // 被选中的行
    cJSON *req_json = NULL;
    LinkList_Object *user_data_list = NULL;

    User_GetAndDisplay:

    system("chcp 936>nul & cls & MODE CON COLS=130 LINES=50");
    // ... 请求过程
    printf("[提示] 正在请求服务器...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "page", cJSON_CreateNumber(page));
    cJSON_AddItemToObject(req_json, "size", cJSON_CreateNumber(page_size));
    cJSON_AddItemToObject(req_json, "kw", cJSON_CreateString(GBKToUTF8(search_kw)));
    cJSON_AddItemToObject(req_json, "sort_method", cJSON_CreateNumber(sort_method));
    ResponseData resp = callBackend("user.getUserList", req_json);
    cJSON_Delete(req_json);
    cJSON *ret_json = resp.json;
    if (resp.status_code != 0) {
        printErrorData(UTF8ToGBK(findExceptionReason(&resp)));
        if (ret_json != NULL) cJSON_Delete(ret_json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }
    total = page = current_total = max_page = page_size = -1;

    // 解析页面基础数据：页码等
    if (jsonParseInteger(ret_json, "total", &total) ||
        jsonParseInteger(ret_json, "current_total", &current_total) ||
        jsonParseInteger(ret_json, "page", &page) ||
        jsonParseInteger(ret_json, "max_page", &max_page) ||
        jsonParseInteger(ret_json, "page_size", &page_size)) {
        printErrorData("页面基础数据解析失败，请返回重试。");
        if (resp.json != NULL) cJSON_Delete(resp.json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // 解析用户数据
    user_data_list = linkListObject_Init(); // 链表初始化
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < current_total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        User *user_data_pt = calloc(1, sizeof(User));
        if (parseUserData(row_data, user_data_pt)) return;
        LinkList_Node *node = linkListObject_Append(user_data_list, user_data_pt); // 链表尾部追加元素
        if (node == NULL) {
            printErrorData("内存错误");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    User_Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(87);
    printf("\n");
    printInMiddle_GBK("======= 用户·用户管理 =======\n", 89);
    printf("%-17s%-20s%-8s%-6s%-18s%-20s\n", "用户UID", "姓名", "角色", "状态", "最后登录IP", "最后登录时间");
    printf("-----------------------------------------------------------------------------------------\n");
    for (LinkList_Node *pt = user_data_list->head; pt != NULL; pt = pt->next) {
        User *tmp = pt->data;
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        printf("%-17s%-20s%-8s%-6s%-18s%-20s\n",
               UTF8ToGBK(tmp->uid),
               UTF8ToGBK(tmp->name),
               USER_ROLE_MAP[tmp->role],
               USER_STATUS_MAP[tmp->status],
               UTF8ToGBK(tmp->last_login_ip),
               getFormatTimeString(tmp->last_login_time));
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // 链表的最后一个节点
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total; i++) printf("\n"); // 补齐页面
    printf("\n");
    printInMiddle_GBK("=============================\n", 89);
    printf("[当前搜索条件] ");
    if (strlen(search_kw) > 0) printf("模糊搜索=%s AND ", search_kw);
    printf("排序方式=%s\n", SORT_METHOD_USER[sort_method]);
    printf("[提示] 共%4d条数据，当前第%3d页，共%3d页（左方向键：前一页；右方向键：后一页；上/下方向键：切换选中数据）\n",
           total, page, max_page);
    printf("\n  ");
    printf(" <A>新建用户 <Enter>编辑用户 <D>删除用户 <K>用户模糊查询 <S>排序切换至%s",
           SORT_METHOD_USER[sort_method + 1 > 3 ? 0 : sort_method + 1]);
    printf(" <Esc>返回主菜单\n");

    if (selectedRow == NULL) {
        if (strlen(search_kw)) {
            printErrorData("没有查询到符合条件的用户");
            strcpy(search_kw, "");
            goto User_GetAndDisplay;
        } else {
            printErrorData("暂无用户（这是不正常的，你是如何登录进来的？）");
            goto GC_Collect;
        }
    }

    int keyboard_press;

    GetKey:
    keyboard_press = _getch();
    switch (keyboard_press) {
        case 224:
            keyboard_press = _getch();
            switch (keyboard_press) {
                case 80: // 下
                    if (selectedRow->next == NULL) selectedRow = user_data_list->head;
                    else selectedRow = selectedRow->next;
                    goto User_Refresh;
                case 72: // 上
                    if (selectedRow->prev == NULL) selectedRow = user_data_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto User_Refresh;
                case 75: // 左
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto User_GetAndDisplay;
                case 77: // 右
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto User_GetAndDisplay;
                default:
                    break;
            }
            break;
        case 's': // 修改排序顺序
        case 'S':
            selectedRow = 0;
            sort_method++;
            if (sort_method > 3) sort_method = 0;
            goto User_GetAndDisplay;
        case 'k': // 搜索关键词
        case 'K':
            printf("\n[模糊搜索] 请输入关键词（以\"Enter\"键结束）：");
            gets_safe(search_kw, 35);
            fflush(stdin);
            selectedRow = 0;
            goto User_GetAndDisplay;
        case 'A':
        case 'a': // 新建用户
            editUser(NULL);
            goto User_GetAndDisplay;
        case 'D':
        case 'd': // 删除用户
        {
            User *pt = selectedRow->data;
            printf("\n[确认删除] 您确定要删除用户 %s(%s) 吗？（不建议删除用户，建议使用停用账户功能。若要继续，请输入该用户的UID:%s确认）\n",
                   UTF8ToGBK(pt->name),
                   UTF8ToGBK(pt->uid), UTF8ToGBK(pt->uid));
            char input_char[31];
            gets_safe(input_char, 30);
            if (strcmp(input_char, pt->uid) != 0) {
                printf("UID不一致，已取消操作（按任意键继续）。\n");
                getch();
                goto User_GetAndDisplay;
            }
            cJSON *cancel_req_json = cJSON_CreateObject();
            cJSON_AddItemToObject(cancel_req_json, "uid", cJSON_CreateString(pt->uid));
            ResponseData cancel_resp = callBackend("user.deleteUser", cancel_req_json);
            cJSON_Delete(cancel_req_json);
            free(cancel_resp.raw_string);
            if (cancel_resp.status_code != 0) {
                printf("[删除失败] 错误码：%d, %s（按任意键继续）\n", cancel_resp.status_code,
                       UTF8ToGBK(findExceptionReason(&cancel_resp)));
                cJSON_Delete(cancel_resp.json);
                getch();
                goto User_GetAndDisplay;
            }
            printf("[删除成功] 用户 %s(%s) 已被删除（按任意键继续）。\n", UTF8ToGBK(pt->name), UTF8ToGBK(pt->uid));
            getch();
            goto User_GetAndDisplay;
        }
        case 13: // 编辑用户
            editUser((User *) selectedRow->data);
            goto User_GetAndDisplay;
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    linkListObject_Delete(user_data_list, 1);
    free(user_data_list);
    user_data_list = NULL;
}
