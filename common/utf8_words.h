//
// Created by admin on 2022/6/25.
//

// 此处存放 utf-8 格式的文案，在gbk代码中引用，即可解决显示问题

#ifndef PROJECT_UTF8_WORDS_H
#define PROJECT_UTF8_WORDS_H

#define WORDS_Login_success "---- 用户登录 - 登陆成功 ----\n用户名：      %s\n姓  名：      %s\n角  色：      %s\n过期时间：    %s\n\n登陆成功，%s，欢迎您（按任意键进入主页）。\n\n"
#define WORDS_Login_status_saved "[自动登录] 登录状态已保存，1天内再次打开软件可自动登录。\n"
#define WORDS_Login_trying_autologin "[自动登录] 正在尝试自动登录...\n"
#define WORDS_Login_role_student "学生"
#define WORDS_Login_role_teacher "教师"
#define WORDS_Login_role_admin "管理员"
#define WORDS_Login_role_unknown "未知角色"

#define WORDS_Login_parse_role_error "[解析错误] 返回参数role解析错误，登录终止。\n"
#define WORDS_Login_save_session_error "[文件错误] 写入登录状态异常，自动登录功能可能不可用。\n"

#endif //PROJECT_UTF8_WORDS_H
