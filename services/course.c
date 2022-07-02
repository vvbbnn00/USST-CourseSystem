//
// Created by admin on 2022/6/26.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "course.h"
#include "cJSON.h"
#include "socket.h"
#include "global.h"
#include <Windows.h>
#include <conio.h>
#include "sysinfo.h"
#include "string_ext.h"
#include "link_list_object.h"
#include <math.h>
#include "simple_string_hash_list_obj.h"
#include "ui_gbk.h"

const char *SORT_METHOD_COURSE[3] = {"默认排序", "学分降序", "学分升序"}; // 显示排序顺序
const char *NUMBER_CAPITAL[10] = {"零", "一", "二", "三", "四", "五", "六", "日", "八", "九"};
const char *LECTURE_TYPE[4] = {"必修", "选修", "公选", "辅修"};


/**
 * 获取课程状态
 *
 * @param status 课程状态
 * @return
 */
char *getCourseStatus(int status) {
    switch (status) {
        case 0:
            return "可选";
        case 1:
            return "已选";
        case 2:
            return "锁定";
        default:
            return "未知";
    }
}


/**
 * 打印课程安排
 *
 * @param schedule
 * @return
 */
char *printSchedule(int schedule[7][13]) {
    char *final_str = (char *) calloc(2000, sizeof(char));
    if (final_str == NULL) return NULL;
    for (int i = 0; i < 6; i++) {
        char has_course = 0;
        char schedule_str[100] = "";
        char t_str[30] = "";
        char tt[10] = "";
        for (int j = 1; j <= 12; j++) {
            if (schedule[i][j]) {
                if (has_course) strcat(schedule_str, ",");
                has_course = 1;
                sprintf(tt, "%d", j);
                strcat(schedule_str, tt);
            }
        }
        if (has_course) {
            sprintf(t_str, "周%s:", NUMBER_CAPITAL[i + 1]);
            strcat(final_str, t_str);
            strcat(final_str, schedule_str);
            strcat(final_str, "节;");
        }
    }
    return final_str;
}


/**
 * 获取每周总学时
 *
 * @param schedule
 * @return
 */
int getTotalWeekHour(int schedule[][13]) {
    int ans = 0;
    for (int i = 0; i < 7; i++) {
        for (int j = 1; j <= 12; j++) {
            if (schedule[i][j]) {
                ans++;
            }
        }
    }
    return ans;
}


/**
 * 打印课程信息
 * @param selected_course
 */
void printCourseData(Course *selected_course) {
    char *t = UTF8ToGBK(selected_course->title);
    printf("\n\t===== 课程信息·%s =====\n\n", t);
    free(t);
    t = UTF8ToGBK(selected_course->course_id);
    printf("\t课  程ID：%s\n", t);
    free(t);
    t = UTF8ToGBK(selected_course->title);
    printf("\t课程名称：%s\n", t);
    free(t);
    t = UTF8ToGBK(selected_course->description);
    printf("\t课程简介：%s\n", t);
    free(t);
    printf("\t课程性质：%s\n", LECTURE_TYPE[selected_course->type]);
    t = UTF8ToGBK(selected_course->semester);
    printf("\t开课学期：%s\n", t);
    free(t);
    printf("\t授课周数：第%d周~第%d周\n", selected_course->week_start, selected_course->week_end);
    char *courseArrangeStr = printSchedule(selected_course->schedule);
    if (courseArrangeStr != NULL) {
        printf("\t授课安排：%s\n", courseArrangeStr);
        free(courseArrangeStr);
    }
    char *t1 = UTF8ToGBK(selected_course->teacher.name), *t2 = UTF8ToGBK(selected_course->teacher.uid);
    printf("\t授课教师：%s(UID:%s)\n", t1, t2);
    free(t1);
    free(t2);
    printf("\t课程学时：%d学时\n",
           getTotalWeekHour(selected_course->schedule) * (selected_course->week_end - selected_course->week_start + 1));
    printf("\t课程学分：%.2f\n", selected_course->points);
}

/**
 * 解析课程数据，存入指针对应结构体
 *
 * @param json
 * @param _dest
 * @return 0 成功 非零 失败
 */
int parseCourseData(cJSON *json, Course *_dest) {
    if (jsonParseInteger(json, "type", &_dest->type) ||
        jsonParseInteger(json, "week_start", &_dest->week_start) ||
        jsonParseInteger(json, "week_end", &_dest->week_end) ||
        jsonParseInteger(json, "max_members", &_dest->max_members) ||
        jsonParseInteger(json, "current_members", &_dest->current_members) ||
        jsonParseFloat(json, "points", &_dest->points)) {
        printErrorData("课程信息数据解析失败，请返回重试。");
        return -1;
    }
    jsonParseString(json, "course_id", _dest->course_id);
    jsonParseString(json, "title", _dest->title);
    jsonParseString(json, "description", _dest->description);
    jsonParseString(json, "semester", _dest->semester);

    cJSON *teacher_data = cJSON_GetObjectItem(json, "teacher");
    jsonParseString(teacher_data, "uid", _dest->teacher.uid);
    jsonParseString(teacher_data, "name", _dest->teacher.name);

    cJSON *schedule_dict = cJSON_GetObjectItem(json, "schedule");
    memset(_dest->schedule, 0, sizeof(_dest->schedule));
    for (int schedule = 1; schedule <= 7; schedule++) {
        char str[2];
        sprintf(str, "%d", schedule);
        cJSON *array_item = cJSON_GetObjectItem(schedule_dict, str);
        int array_length = cJSON_GetArraySize(array_item);
        for (int j = 0; j < array_length; j++) {
            int lecture_key = -1;
            if (jsonArrayParseInteger(array_item, j, &lecture_key)) {
                printErrorData("页面详细数据(节次安排)解析失败，请返回重试。");
            }
            _dest->schedule[schedule - 1][lecture_key] = 1;
        }
    }
    return 0;
}

/**
 * 执行学生选课事宜
 */
void printStudentCourseSelection() {
    system("chcp 936>nul & cls & MODE CON COLS=110 LINES=50");
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    char search_kw[36] = "";
    double max_score = 0, current_score = 0; // 本学期最多可选学分, 当前已选学分
    int sort_method = 0; // 排序方法 0 - 默认排序 1 - 学分降序 2 - 学分升序
    int total, page = 1, max_page, page_size = 10, current_total;

    LinkList_Node *selectedRow = NULL; // 被选中的行
    cJSON *req_json = NULL;
    LinkList_Object *course_data_list = NULL;

    GetCourseAndDisplay:
    // ... 请求过程
    printf("[提示] 正在请求服务器...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "course_selection", cJSON_CreateNumber(1)); // 选课场景，需要返回相关数据
    cJSON_AddItemToObject(req_json, "page", cJSON_CreateNumber(page));
    cJSON_AddItemToObject(req_json, "size", cJSON_CreateNumber(page_size));
    cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(GLOBAL_semester));
    cJSON_AddItemToObject(req_json, "kw", cJSON_CreateString(GBKToUTF8(search_kw)));
    cJSON_AddItemToObject(req_json, "sort_method", cJSON_CreateNumber(sort_method));
    ResponseData resp = callBackend("course.getCourseList", req_json);
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
        jsonParseInteger(ret_json, "page_size", &page_size) ||
        jsonParseFloat(ret_json, "current_score", &current_score) ||
        jsonParseFloat(ret_json, "max_score", &max_score)) {
        printErrorData("页面基础数据解析失败，请返回重试。");
        if (resp.json != NULL) cJSON_Delete(resp.json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // 解析课程数据
    course_data_list = linkListObject_Init(); // 链表初始化
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < current_total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        struct studentCourseSelection *course_data_pt = calloc(1, sizeof(struct studentCourseSelection));
        if (parseCourseData(row_data, &course_data_pt->course)) return;
        if (jsonParseInteger(row_data, "status", &course_data_pt->status) ||
            jsonParseLong(row_data, "selection_time", &course_data_pt->selection_time)) {
            printErrorData("页面详细数据解析失败，请返回重试。");
        }
        jsonParseString(row_data, "locked_reason", course_data_pt->locked_reason);
        LinkList_Node *node = linkListObject_Append(course_data_list, course_data_pt); // 链表尾部追加元素
        if (node == NULL) {
            printErrorData("内存错误");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(102);
    printf("\n");
    printInMiddle_GBK("======= 课程·学生选课 =======\n", 104);
    printf("%-16s%-35s%-35s%-10s%-12s\n", "选课状态", "课程ID", "课程名称", "课程学分", "授课教师");
    printf("--------------------------------------------------------------------------------------------------------\n");
    for (LinkList_Node *pt = course_data_list->head; pt != NULL; pt = pt->next) {
        struct studentCourseSelection *tmp = pt->data;
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        else {
            switch (tmp->status) {
                case 1: // 已选
                    SetConsoleTextAttribute(windowHandle, 0x0e);
                    break;
                case 2: // 锁定
                    SetConsoleTextAttribute(windowHandle, 0x08);
                    break;
                default:
                    break;
            }
        }
        printf("[%4s]%3d/%-3d   %-35s%-35s%-10.2f%-12s\n",
               getCourseStatus(tmp->status),
               tmp->course.current_members,
               tmp->course.max_members,
               UTF8ToGBK(tmp->course.course_id),
               UTF8ToGBK(tmp->course.title),
               tmp->course.points,
               UTF8ToGBK(tmp->course.teacher.name));
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // 链表的最后一个节点
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total; i++) printf("\n"); // 补齐页面
    printf("\n");
    printInMiddle_GBK("=============================\n", 104);
    printf("[当前搜索条件] ");
    if (strlen(search_kw) > 0) printf("模糊搜索=%s AND ", search_kw);
    printf("排序方式=%s", SORT_METHOD_COURSE[sort_method]);
    printf("  [选课概况] %s学期，已选%.2f学分，最多可选%.2f学分\n\n", UTF8ToGBK(GLOBAL_semester), current_score, max_score);
    printf("[提示] 共%4d条数据，当前第%3d页，共%3d页（左方向键：前一页；右方向键：后一页；上/下方向键：切换选中数据）\n",
           total, page, max_page);
    printf("\n\t   <Enter>选课/退选 <K>课程模糊查询 <S>排序切换至%s <Esc>返回主菜单\n",
           SORT_METHOD_COURSE[sort_method + 1 > 2 ? 0 : sort_method + 1]);

    if (selectedRow == NULL) {
        printErrorData("暂无可选课程");
//        getch();
        if (strlen(search_kw) == 0)
            goto GC_Collect;
        else {
            strcpy(search_kw, "");
            goto GetCourseAndDisplay;
        }
    }

    struct studentCourseSelection *selected_selection = selectedRow->data;
    Course *selected_course = &selected_selection->course;

    printCourseData(selected_course);
    switch (selected_selection->status) {
        case 1: // 已选
            SetConsoleTextAttribute(windowHandle, 0x0e);
            break;
        case 2: // 锁定
            SetConsoleTextAttribute(windowHandle, 0x08);
            break;
        default:
            break;
    }
    printf("\t状  态：  %s[%d/%d人]%s%s%s%s\n",
           getCourseStatus(selected_selection->status),
           selected_course->current_members,
           selected_course->max_members,
           strlen(selected_selection->locked_reason) > 0 ? " - 不可选原因：" : "",
           UTF8ToGBK(selected_selection->locked_reason),
           selected_selection->selection_time > 0 ? " - 选课时间：" : "",
           selected_selection->selection_time > 0 ?
           getFormatTimeString(selected_selection->selection_time) : "");
    SetConsoleTextAttribute(windowHandle, 0x07);

    int keyboard_press;

    GetKey:
    keyboard_press = _getch();
    cJSON *xk_json = NULL;
    switch (keyboard_press) {
        case 224:
            keyboard_press = _getch();
            switch (keyboard_press) {
                case 80: // 下
                    if (selectedRow->next == NULL) selectedRow = course_data_list->head;
                    else selectedRow = selectedRow->next;
                    goto Refresh;
                case 72: // 上
                    if (selectedRow->prev == NULL) selectedRow = course_data_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto Refresh;
                case 75: // 左
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto GetCourseAndDisplay;
                case 77: // 右
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto GetCourseAndDisplay;
                default:
                    break;
            }
            break;
        case 's': // 修改排序顺序
        case 'S':
            selectedRow = 0;
            sort_method++;
            if (sort_method > 2) sort_method = 0;
            goto GetCourseAndDisplay;
        case 'k': // 搜索关键词
        case 'K':
            printf("\n[模糊搜索]请输入关键词（以\"Enter\"键结束）：");
            gets_safe(search_kw, 35);
            fflush(stdin);
            selectedRow = 0;
            goto GetCourseAndDisplay;
        case 13: // 选课选项
            printf("[提示] 正在请求服务器...");
            xk_json = cJSON_CreateObject();
            cJSON_AddItemToObject(xk_json, "course_id", cJSON_CreateString(selected_course->course_id));
            if (selected_selection->status == 0) {
                if (selected_course->points + current_score > max_score) {
                    printf("\n[选课失败] 课程:%s(%s)选课失败，您的学分上限为%.2f。\n",
                           UTF8ToGBK(selected_course->title),
                           UTF8ToGBK(selected_course->course_id),
                           max_score);
                    goto After;
                }
                ResponseData xk_resp = callBackend("course.select", xk_json);
                free(xk_resp.raw_string);
                if (xk_resp.status_code == 0) {
                    printf("\n[选课成功] 课程:%s(%s)已选择成功（按任意键继续）。\n",
                           UTF8ToGBK(selected_course->title),
                           UTF8ToGBK(selected_course->course_id));
                } else {
                    printf("\n[选课失败] 课程:%s(%s)选课失败（错误码：%d）, %s。\n",
                           UTF8ToGBK(selected_course->title),
                           UTF8ToGBK(selected_course->course_id),
                           xk_resp.status_code,
                           UTF8ToGBK(findExceptionReason(&xk_resp)));
                }
                cJSON_Delete(xk_resp.json);
            } else {
                if (selected_selection->status == 1) {
                    ResponseData xk_resp = callBackend("course.cancel", xk_json);
                    free(xk_resp.raw_string);
                    if (xk_resp.status_code == 0) {
                        printf("\n[退选成功] 课程:%s(%s)已成功退选（按任意键继续）。\n",
                               UTF8ToGBK(selected_course->title),
                               UTF8ToGBK(selected_course->course_id));
                    } else {
                        printf("\n[退选失败] 课程:%s(%s)退选失败（错误码：%d）, %s。\n",
                               UTF8ToGBK(selected_course->title),
                               UTF8ToGBK(selected_course->course_id),
                               xk_resp.status_code,
                               UTF8ToGBK(findExceptionReason(&xk_resp)));
                    }
                    cJSON_Delete(xk_resp.json);
                } else
                    printf("\n[选课失败] 您无法选择该课程：%s（按任意键继续）\n",
                           UTF8ToGBK(selected_selection->locked_reason));
            }
        After:
            cJSON_Delete(xk_json);
            getch();
            goto GetCourseAndDisplay;
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    linkListObject_Delete(course_data_list, 1);
    free(course_data_list);
    course_data_list = NULL;
}

/**
 * 输出课表到文件流中（包括控制台）
 *
 * @param _stream
 * @param scheduleList
 */
void printTableToStream(FILE *_stream, LinkList_Object scheduleList[7][13]) {
    for (int week_num = 0; week_num < 7; week_num++) {
        char has_course = 0; // 当前星期是否有课
        fprintf(_stream, "[星期%s]\n\n", NUMBER_CAPITAL[week_num + 1]);
        for (int course_num = 1; course_num <= 12; course_num++) {
            LinkList_Node *head = scheduleList[week_num][course_num].head;
            if (head == NULL) continue; // 该时段没有课
            has_course = 1;
            for (LinkList_Node *pt = head; pt != NULL; pt = pt->next) {
                Course *c_data = pt->data;
                fprintf(_stream, "  <第");
                for (int i = course_num; i <= 12; i++) {
                    if (!c_data->schedule[week_num][i]) break;
                    fprintf(_stream, " %d", i);
                }
                fprintf(_stream, " 节> %s\n", UTF8ToGBK(c_data->title));
                fprintf(_stream, "    授课教师：%s，授课周：第%d周至第%d周，学分：%.2f\n",
                        UTF8ToGBK(c_data->teacher.name),
                        c_data->week_start,
                        c_data->week_end,
                        c_data->points);
            }
        }
        if (!has_course) fprintf(_stream, "  无课程\n");
        fprintf(_stream, "\n");
    }
}


/**
 * 输出学生课表，并展示于控制台
 */
void printStudentLectureTable() {
    system("chcp 936>nul & cls & MODE CON COLS=70 LINES=9001"); // 这里行数一定要大，不然数据会被刷掉

    int total; // 总结果条数
    double score_total; // 总学分
    LinkList_Object scheduleList[7][13] = {0}; // 解析后的课表数据放在这里，对应节次等信息[7]表示周一(0)至周日(6)，[13]表示第一节课(1)至第十二节课(12)

    cJSON *req_json = NULL;

    // ... 请求过程
    printf("[提示] 正在请求服务器...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(GLOBAL_semester));
    ResponseData resp = callBackend("course.getStuCourseTable", req_json);
    cJSON_Delete(req_json);
    cJSON *ret_json = resp.json;
    if (resp.status_code != 0) {
        printErrorData(UTF8ToGBK(findExceptionReason(&resp)));
        if (ret_json != NULL) cJSON_Delete(ret_json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // 解析页面基础数据：页码等
    if (jsonParseInteger(ret_json, "total", &total) ||
        jsonParseInteger(ret_json, "total", &total) ||
        jsonParseFloat(ret_json, "score_total", &score_total)) {
        printErrorData("页面基础数据解析失败，请返回重试。");
        if (resp.json != NULL) cJSON_Delete(resp.json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // 解析课程数据
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        Course *course_data_pt = alloca(sizeof(Course));
        if (parseCourseData(row_data, course_data_pt)) return;
        for (int week = 0; week < 7; week++) {
            int continuous = 0; // 判断是否是连续的课程安排，如果是 则算在一起，不重复添加在课表中
            for (int seq = 1; seq <= 12; seq++) {
                if (course_data_pt->schedule[week][seq]) {  // 该时段有课
                    if (!continuous) {
                        linkListObject_Append(&scheduleList[week][seq], course_data_pt); // 将课程加入进去
                        continuous = 1; // 重复，flag激活
                    }
                } else {
                    continuous = 0; // 遇到一个不重复的，则flag重置
                }
            }
        }
    }

    Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(58);
    printf("\n");
    printInMiddle_GBK("======= 课程·当前学期课表 =======\n", 60);
    printTableToStream(stdout, scheduleList);
    printf("------------------------------------------------------------\n");
    printf("\n    [学生姓名] %s  [当前学期] %s  [已选学分] %.2f\n",
           UTF8ToGBK(GLOBAL_user_info.name),
           UTF8ToGBK(GLOBAL_semester),
           score_total);
    printf("\n\t <E>导出课表到文件中 <Esc>返回主菜单\n\n");

    int keyboard_press;

    GetKey:
    keyboard_press = _getch();
    switch (keyboard_press) {
        case 'e': // 导出课表
        case 'E': {
            time_t raw_time;
            time(&raw_time);
            char *timestamp = getFormatTimeString_("%Y%m%d%H%M%S", raw_time);
            char *name = UTF8ToGBK(GLOBAL_user_info.name);
            char *semester = UTF8ToGBK(GLOBAL_semester);
            unsigned long long len = strlen(timestamp) +
                                     strlen(name) +
                                     strlen(semester) +
                                     strlen("的学期课表_.txt") + 1;
            char file_name[len];
            memset(file_name, 0, len);
            sprintf(file_name, "%s的%s学期课表_%s.txt", name, GLOBAL_semester, timestamp);
            free(name);
            free(semester);
            free(timestamp);
            FILE *file = fopen(file_name, "w");
            printf("[提示] 正在将课表导出至：%s...\n", file_name);
            if (file == NULL) {
                printf("[导出失败] 无法创建文件\"%s\"，请检查是否有读写权限（按任意键继续）\n", file_name);
                goto Refresh;
            }
            printTableToStream(file, scheduleList);
            fclose(file);
            printf("[导出成功] 课表已导出至：%s（按任意键继续）\n", file_name);
            getch();
            goto Refresh;
        }
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    for (int i = 0; i < 7; i++)
        for (int j = 1; j <= 12; j++) linkListObject_Delete(&scheduleList[i][j], 0);
}


/**
 * 输出全校课表（管理员版本）
 * @param scene 场景 0 - 查看全校课表（管理员可管理） 1 - 教师操作（只显示自己教的课程，可新增）
 */
void printAllCourses(int scene) {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    char search_kw[36] = "", search_semester[36] = "";
    int sort_method = 0; // 排序方法 0 - 默认排序 1 - 学分降序 2 - 学分升序
    int total, page = 1, max_page, page_size = 15, current_total;

    LinkList_Node *selectedRow = NULL; // 被选中的行
    cJSON *req_json = NULL;
    LinkList_Object *course_data_list = NULL;

    GetCourseAndDisplay:
    system("chcp 936>nul & cls & MODE CON COLS=130 LINES=55");
    // ... 请求过程
    printf("[提示] 正在请求服务器...\n");
    req_json = cJSON_CreateObject();
    if (scene == 1) {
        cJSON_AddItemToObject(req_json, "manage", cJSON_CreateNumber(1));
    }
    cJSON_AddItemToObject(req_json, "page", cJSON_CreateNumber(page));
    cJSON_AddItemToObject(req_json, "size", cJSON_CreateNumber(page_size));
    cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(GBKToUTF8(search_semester)));
    cJSON_AddItemToObject(req_json, "kw", cJSON_CreateString(GBKToUTF8(search_kw)));
    cJSON_AddItemToObject(req_json, "sort_method", cJSON_CreateNumber(sort_method));
    ResponseData resp = callBackend("course.getCourseList", req_json);
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

    // 解析课程数据
    course_data_list = linkListObject_Init(); // 链表初始化
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < current_total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        Course *course_data_pt = calloc(1, sizeof(Course));
        if (parseCourseData(row_data, course_data_pt)) return;
        LinkList_Node *node = linkListObject_Append(course_data_list, course_data_pt); // 链表尾部追加元素
        if (node == NULL) {
            printErrorData("内存错误");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(120);
    printf("\n");
    printInMiddle_GBK("======= 课程·全校课表一览 =======\n", 122);
    printf("%-35s%-35s%-20s%-10s%-10s%-12s\n", "课程ID", "课程名称", "课程学期", "课程学分", "课程性质", "授课教师");
    printf("--------------------------------------------------------------------------------------------------------------------------\n");
    for (LinkList_Node *pt = course_data_list->head; pt != NULL; pt = pt->next) {
        Course *tmp = pt->data;
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        printf("%-35s%-35s%-20s%-10.2f%-10s%-12s\n",
               UTF8ToGBK(tmp->course_id),
               UTF8ToGBK(tmp->title),
               UTF8ToGBK(tmp->semester),
               tmp->points,
               LECTURE_TYPE[tmp->type],
               UTF8ToGBK(tmp->teacher.name));
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // 链表的最后一个节点
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total; i++) printf("\n"); // 补齐页面
    printf("\n");
    printInMiddle_GBK("=============================\n", 122);
    printf("[当前搜索条件] ");
    if (strlen(search_kw) > 0) printf("模糊搜索=%s AND ", search_kw);
    if (strlen(search_semester) > 0) printf("指定学期=%s AND ", search_semester);
    printf("排序方式=%s\n", SORT_METHOD_COURSE[sort_method]);
    printf("[提示] 共%4d条数据，当前第%3d页，共%3d页（左方向键：前一页；右方向键：后一页；上/下方向键：切换选中数据）\n",
           total, page, max_page);
    printf("\n");
    if (GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1)) { // 管理员可编辑课程
        printf("<A>开设/新建课程 <Enter>编辑课程 <P>查看学生名单 <D>删除课程");
    } else {
        printf("\t  ");
    }
    printf(" <K>课程模糊查询 <F>指定课程学期查询 <S>排序切换至%s",
           SORT_METHOD_COURSE[sort_method + 1 > 2 ? 0 : sort_method + 1]);
    printf(" <Esc>返回主菜单\n");

    if (selectedRow == NULL) {
        if (strlen(search_kw) || strlen(search_semester)) {
            printErrorData("没有查询到符合条件的课程");
            strcpy(search_semester, "");
            strcpy(search_kw, "");
            goto GetCourseAndDisplay;
        } else {
            if ((GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) { // 管理员可编辑课程
                printf("暂无课程，是否立即创建课程？(Y)");
                int ch = getch();
                if (ch == 'Y' || ch == 'y') {
                    editCourse(NULL);
                }
            } else {
                printErrorData("暂无课程");
            }
            goto GC_Collect;
        }
    }

    printCourseData(selectedRow->data);

    int keyboard_press;

    GetKey:
    keyboard_press = _getch();
    switch (keyboard_press) {
        case 224:
            keyboard_press = _getch();
            switch (keyboard_press) {
                case 80: // 下
                    if (selectedRow->next == NULL) selectedRow = course_data_list->head;
                    else selectedRow = selectedRow->next;
                    goto Refresh;
                case 72: // 上
                    if (selectedRow->prev == NULL) selectedRow = course_data_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto Refresh;
                case 75: // 左
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto GetCourseAndDisplay;
                case 77: // 右
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto GetCourseAndDisplay;
                default:
                    break;
            }
            break;
        case 's': // 修改排序顺序
        case 'S':
            selectedRow = 0;
            sort_method++;
            if (sort_method > 2) sort_method = 0;
            goto GetCourseAndDisplay;
        case 'k': // 搜索关键词
        case 'K':
            printf("\n[模糊搜索]请输入关键词（以\"Enter\"键结束）：");
            gets_safe(search_kw, 35);
            fflush(stdin);
            selectedRow = 0;
            goto GetCourseAndDisplay;
        case 'f': // 筛选学期
        case 'F':
            printf("\n[指定查询学期]请输入学期（以\"Enter\"键结束）：");
            gets_safe(search_semester, 35);
            fflush(stdin);
            selectedRow = 0;
            goto GetCourseAndDisplay;
        case 'A':
        case 'a':
            if (!(GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) break; // 权限判断
            editCourse(NULL);
            goto GetCourseAndDisplay;
        case 'D':
        case 'd': // 删除课程
            if (!(GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) break; // 权限判断
            {
                Course *pt = selectedRow->data;
                printf("\n[确认删除] 您确定要删除课程 %s(%s) 吗？（删除课程后，该课程所有的学生选课记录将被清空！若要继续，请输入该课程ID:%s确认）\n",
                       UTF8ToGBK(pt->title),
                       UTF8ToGBK(pt->course_id), UTF8ToGBK(pt->course_id));
                char input_char[33];
                gets_safe(input_char, 32);
                if (strcmp(input_char, pt->course_id) != 0) {
                    printf("课程ID不一致，已取消操作（按任意键继续）。\n");
                    getch();
                    goto GetCourseAndDisplay;
                }
                cJSON *cancel_req_json = cJSON_CreateObject();
                cJSON_AddItemToObject(cancel_req_json, "course_id", cJSON_CreateString(pt->course_id));
                ResponseData cancel_resp = callBackend("course.deleteCourse", cancel_req_json);
                cJSON_Delete(cancel_req_json);
                free(cancel_resp.raw_string);
                if (cancel_resp.status_code != 0) {
                    printf("[删除失败] 错误码：%d, %s（按任意键继续）\n", cancel_resp.status_code,
                           UTF8ToGBK(findExceptionReason(&cancel_resp)));
                    cJSON_Delete(cancel_resp.json);
                    getch();
                    goto GetCourseAndDisplay;
                }
                printf("[删除成功] 课程 %s(%s) 和相关选课记录已被删除（按任意键继续）。\n", UTF8ToGBK(pt->title), UTF8ToGBK(pt->course_id));
                getch();
                goto GetCourseAndDisplay;
            }
        case 'p':
        case 'P':
            if (!(GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) break; // 权限判断
            printStudentList((Course *) selectedRow->data);
            goto GetCourseAndDisplay;
        case 13: // 编辑课程
            if (!(GLOBAL_user_info.role == 2 || (GLOBAL_user_info.role == 1 && scene == 1))) break; // 权限判断
            editCourse((Course *) selectedRow->data);
            goto GetCourseAndDisplay;
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    linkListObject_Delete(course_data_list, 1);
    free(course_data_list);
    course_data_list = NULL;
}


/**
 * 解析课程数据，存入指针对应结构体
 *
 * @param json
 * @param _dest
 * @return 0 成功 非零 失败
 */
int parseStudentData(cJSON *json, struct teacherCourseSelection *_dest) {
    if (jsonParseLong(json, "selection_time", &_dest->selection_time)) {
        printErrorData("课程信息数据解析失败，请返回重试。");
        return -1;
    }
    jsonParseString(json, "uid", _dest->student.uid);
    jsonParseString(json, "name", _dest->student.name);
    return 0;
}


/**
 * 导出学生列表
 * @param linkList
 * @param course
 * @return
 */
int exportStudentList(LinkList_Object *linkList, Course *course) {
    time_t raw_time;
    time(&raw_time);
    char *timestamp = getFormatTimeString_("%Y%m%d%H%M%S", raw_time);
    char *title = UTF8ToGBK(course->title);
    char *course_id = UTF8ToGBK(course->course_id);
    unsigned long long len = strlen(timestamp) +
                             strlen(title) +
                             strlen(course_id) +
                             strlen("()学生名单_.csv") + 1;
    char file_name[len];
    memset(file_name, 0, len);
    sprintf(file_name, "%s(%s)学生名单_%s.csv", title, course_id, timestamp);
    free(title);
    free(course_id);
    free(timestamp);
    FILE *file = fopen(file_name, "w");
    printf("[提示] 正在将学生名单导出至：%s...\n", file_name);
    if (file == NULL) {
        printf("[导出失败] 无法创建文件\"%s\"，请检查是否有读写权限（按任意键继续）\n", file_name);
        return -1;
    }
    fprintf(file, "序号,姓名,学号,选课时间\n");
    int counter = 1;
    for (LinkList_Node *pt = linkList->head; pt != NULL; pt = pt->next) {
        struct teacherCourseSelection *p = pt->data;
        fprintf(file, "%d,%s,%s,%s\n",
                counter,
                UTF8ToGBK(p->student.name),
                UTF8ToGBK(p->student.uid),
                getFormatTimeString(p->selection_time));
        counter++;
    }
    fclose(file);
    printf("[导出成功] 学生名单已导出至：%s（按任意键继续）\n", file_name);
    return 0;
}


/**
 * 教师/管理员打印学生名单
 */
void printStudentList(Course *courseData) {
    system("chcp 936>nul & cls & MODE CON COLS=80 LINES=55");
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    int sort_method = 0; // 排序方法 0 - 默认排序 1 - 学分降序 2 - 学分升序
    int total, page = 1, page_size = 30, max_page;

    cJSON *req_json = NULL;
    LinkList_Object *student_list = NULL;
    LinkList_Node *selectedRow = NULL;

    Teacher_GetCourseAndDisplay:
    // ... 请求过程
    printf("[提示] 正在请求服务器...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "course_id", cJSON_CreateString(courseData->course_id));
    ResponseData resp = callBackend("course.getStudentSelectionList", req_json);
    cJSON_Delete(req_json);
    cJSON *ret_json = resp.json;
    if (resp.status_code != 0) {
        printErrorData(UTF8ToGBK(findExceptionReason(&resp)));
        if (ret_json != NULL) cJSON_Delete(ret_json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }

    // 解析名单总条数
    if (jsonParseInteger(ret_json, "total", &total)) {
        printErrorData("页面基础数据解析失败，请返回重试。");
        if (resp.json != NULL) cJSON_Delete(resp.json);
        if (resp.raw_string != NULL) free(resp.raw_string);
        return;
    }
    max_page = (int) ceilf((float) total / (float) page_size);

    // 解析名单数据
    student_list = linkListObject_Init(); // 链表初始化
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        struct teacherCourseSelection *data_pt = calloc(1, sizeof(struct teacherCourseSelection));
        if (parseStudentData(row_data, data_pt)) return;
        LinkList_Node *node = linkListObject_Append(student_list, data_pt); // 链表尾部追加元素
        if (node == NULL) {
            printErrorData("内存错误");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    Teacher_Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= 课程·课程名单 =======\n", 71);
    printf("%-6s%-15s%-20s%-30s\n", "序号", "姓名", "学号", "选课时间");
    printf("-----------------------------------------------------------------------\n");
    int counter = 0, current_total = 0;
    for (LinkList_Node *pt = student_list->head; pt != NULL; pt = pt->next) {
        counter++;
        if (ceilf((float) counter / (float) page_size) < (float) page) continue; // 定位到该页面，学生名单分页显示
        if (ceilf((float) counter / (float) page_size) > (float) page) break; // 一页显示完即可跳出循环
        current_total++;
        if (selectedRow == NULL) selectedRow = pt;
        struct teacherCourseSelection *tmp = pt->data;
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        printf("%4d  %-15s%-20s%-30s\n",
               counter,
               UTF8ToGBK(tmp->student.name),
               UTF8ToGBK(tmp->student.uid),
               getFormatTimeString(tmp->selection_time));
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // 链表的最后一个节点
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total - 1; i++) printf("\n"); // 补齐页面
    printf("\n");
    printInMiddle_GBK("=============================\n", 71);
    printf("    [课程名称] %s [课程ID] %s\n\n", UTF8ToGBK(courseData->title), courseData->course_id);
    printf("    [提示] 共%4d条数据，当前第%3d页，共%3d页\n\n    （左方向键：前一页；右方向键：后一页；上/下方向键：切换选中数据）\n",
           total, page, max_page);
    printf("\n  ");
    if (GLOBAL_user_info.role == 2) { // 管理员可编辑课程
        printf("<A>添加学生 <D>取消该学生选课 <I>批量导入学生");
    } else {
        printf("\t");
    }
    printf(" <E>导出学生名单 <Esc>返回主菜单\n");

    if (selectedRow == NULL) {
        printErrorData("暂无名单");
        if (GLOBAL_user_info.role == 2) { // 管理员可编辑课程
            printf("您是管理员，是否导入学生名单？(Y)");
            int ch = getch();
            if (ch == 'Y' || ch == 'y') {
                importStuCourseData();
            }
        }
        goto Teacher_GC_Collect;
    }
    int keyboard_press;

    Teacher_GetKey:
    keyboard_press = _getch();
    switch (keyboard_press) {
        case 224:
            keyboard_press = _getch();
            switch (keyboard_press) {
                case 80: // 下
                    if (selectedRow->next == NULL) selectedRow = student_list->head;
                    else selectedRow = selectedRow->next;
                    goto Teacher_Refresh;
                case 72: // 上
                    if (selectedRow->prev == NULL) selectedRow = student_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto Teacher_Refresh;
                case 75: // 左
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto Teacher_Refresh;
                case 77: // 右
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto Teacher_Refresh;
                default:
                    break;
            }
            break;
        case 'E':
        case 'e':
            exportStudentList(student_list, courseData);
            _getch();
            goto Teacher_Refresh;
        case 'I':
        case 'i': // 批量导入学生信息
            if (GLOBAL_user_info.role != 2) break; // 无法新增学生
            importStuCourseData();
            goto Teacher_GetCourseAndDisplay;
        case 'A':
        case 'a': // 新增选课学生
            if (GLOBAL_user_info.role != 2) break; // 无法新增学生
            {
                char user_id[31] = {0};
                if (inputStringWithRegexCheck("[课程新增学生] 请输入学生学号（如需批量导入学生信息，请使用“批量导入学生”功能）\n",
                                              USER_PATTERN,
                                              user_id,
                                              30) == -1)
                    goto Teacher_Refresh;
                cJSON *add_req = cJSON_CreateObject();
                cJSON *add_list = cJSON_CreateArray();
                cJSON *add_object = cJSON_CreateObject();
                cJSON_AddItemToObject(add_object, "course_id", cJSON_CreateString(courseData->course_id));
                cJSON_AddItemToObject(add_object, "uid", cJSON_CreateString(user_id));
                cJSON_AddItemToArray(add_list, add_object);
                cJSON_AddItemToObject(add_req, "data", add_list);
                ResponseData add_resp = callBackend("course.adminAdd", add_req);
                cJSON_Delete(add_req);
                free(add_resp.raw_string);
                if (add_resp.status_code != 0) {
                    printf("[新增学生失败] 错误码：%d, %s（按任意键继续）\n", add_resp.status_code,
                           UTF8ToGBK(findExceptionReason(&add_resp)));
                } else {
                    printf("[新增学生成功] 按任意键继续。\n");
                }
                cJSON_Delete(add_resp.json);
                getch();
                goto Teacher_GetCourseAndDisplay;
            }
            break;
        case 'D':
        case 'd': // 退选课程
            if (GLOBAL_user_info.role != 2) break;
            {
                struct teacherCourseSelection *pt = selectedRow->data;
                printf("\n[确认退选] 您确定要退选学生 %s(%s) 吗？（请输入该学生的学号:%s确认）\n", UTF8ToGBK(pt->student.name),
                       UTF8ToGBK(pt->student.uid), UTF8ToGBK(pt->student.uid));
                char input_char[31];
                gets_safe(input_char, 30);
                if (strcmp(input_char, pt->student.uid) != 0) {
                    printf("学号不一致，已取消操作（按任意键继续）。\n");
                    getch();
                    goto Teacher_GetCourseAndDisplay;
                }
                cJSON *cancel_req_json = cJSON_CreateObject();
                cJSON_AddItemToObject(cancel_req_json, "course_id", cJSON_CreateString(courseData->course_id));
                cJSON_AddItemToObject(cancel_req_json, "uid", cJSON_CreateString(pt->student.uid));
                ResponseData cancel_resp = callBackend("course.adminCancel", cancel_req_json);
                cJSON_Delete(cancel_req_json);
                free(cancel_resp.raw_string);
                if (cancel_resp.status_code != 0) {
                    printf("[退选失败] 错误码：%d, %s（按任意键继续）\n", cancel_resp.status_code,
                           UTF8ToGBK(findExceptionReason(&cancel_resp)));
                    cJSON_Delete(cancel_resp.json);
                    getch();
                    goto Teacher_GetCourseAndDisplay;
                }
                printf("[退选成功] 学生 %s(%s) 已被退选（按任意键继续）。\n", UTF8ToGBK(pt->student.name), UTF8ToGBK(pt->student.uid));
                getch();
                goto Teacher_GetCourseAndDisplay;
            }
            break;
        case 27:
            goto Teacher_GC_Collect;
        default:
            break;
    }
    goto Teacher_GetKey;

    Teacher_GC_Collect:
    linkListObject_Delete(student_list, 1);
    free(student_list);
    student_list = NULL;
}

/**
 * （控件）修改上课安排
 * @param schedule
 * @return
 */
Schedule editSchedule(int schedule[7][13]) {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (schedule == NULL) {
        schedule = alloca(sizeof(int[7][13]));
        memset(schedule, 0, sizeof(int[7][13]));
    }
    Schedule returnSchedule;
    memcpy(returnSchedule.schedule, schedule, sizeof(int[7][13]));
    int x = 1, y = 0;

    ScheduleEditor_Refresh:
    system("chcp 936>nul & cls & MODE CON COLS=65 LINES=32");
    ui_printHeader_GBK(50);
    printInMiddle_GBK("\n======= 课程·编辑授课安排 =======\n", 52);
    printf("-------------------------------------------------------\n");
    printf("   节次     周一  周二  周三  周四  周五  周六  周日\n");
    printf("-------------------------------------------------------\n");
    for (int i = 1; i <= 12; i++) {
        printf("  第%2d节课 ", i);
        for (int j = 0; j < 7; j++) {
            if (x == i && y == j) SetConsoleTextAttribute(windowHandle, 0x70);
            printf("  %2s  ", returnSchedule.schedule[j][i] == 1 ? "√" : " ");
            SetConsoleTextAttribute(windowHandle, 0x07);
        }
        if (i == 5 || i == 9) printf("\n"); // 分时段
        printf("\n");
    }

    printf("\n\n<Enter>切换选中状态 <Y>结束编辑 <ESC>取消编辑 <上/下/左/右>切换\n");

    int key;
    ScheduleEditor_GetKey:
    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 80: // 下
                    if (x < 12) x++;
                    else x = 1;
                    goto ScheduleEditor_Refresh;
                case 72: // 上
                    if (x > 1) x--;
                    else x = 12;
                    goto ScheduleEditor_Refresh;
                case 75: // 左
                    if (y > 0) y--;
                    else y = 6;
                    goto ScheduleEditor_Refresh;
                case 77: // 右
                    if (y < 6) y++;
                    else y = 0;
                    goto ScheduleEditor_Refresh;
                default:
                    break;
            }
            break;
        case 13:
            returnSchedule.schedule[y][x] = returnSchedule.schedule[y][x] == 1 ? 0 : 1;
            goto ScheduleEditor_Refresh;
        case 'y':
        case 'Y':
            return returnSchedule;
        case 27:
            memcpy(returnSchedule.schedule, schedule, sizeof(int[7][13]));
            return returnSchedule;
        default:
            break;
    }
    goto ScheduleEditor_GetKey;
}


/**
 * 新增课程
 * @return 取消则返回NULL，否则返回课程指针
 */
Course *addCourse() {
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= 课程·新增/修改课程 =======\n", 71);

    Course *course = calloc(1, sizeof(Course));
    if (inputStringWithRegexCheck("[新增课程] 请输入课程ID（长度为5-30，由字母、数字和-组成的字符串，建议格式<学年>-<学期>-<课程代号>-<班级>）\n",
                                  COURSE_ID_PATTERN,
                                  course->course_id,
                                  30) == -1)
        goto CancelAdd;
    if (GLOBAL_user_info.role == 2) { // 管理员开课需指定老师用户
        if (inputStringWithRegexCheck("[新增课程] 请输入开课教师ID（用户角色需为教师或管理员）\n",
                                      USER_PATTERN,
                                      course->teacher.uid,
                                      15) == -1)
            goto CancelAdd;
    } else {
        strcpy(course->teacher.uid, GLOBAL_user_info.userid); // 教师无法修改UID
    }
    if (inputStringWithRegexCheck("[新增课程] 请输入课程标题（长度为5-50字符，由字母、数字、中文和()-组成的字符串）\n",
                                  COURSE_TITLE_PATTERN,
                                  course->title,
                                  100) == -1)
        goto CancelAdd;
    if (inputStringWithRegexCheck("[新增课程] 请输入课程描述（长度为5-250字符）\n",
                                  COURSE_DESCRIPTION_PATTERN,
                                  course->description,
                                  500) == -1)
        goto CancelAdd;
    if (inputIntWithRegexCheck("[新增课程] 请输入课程类型：0-必修、1-选修、2-公选、3-辅修\n",
                               COURSE_TYPE_PATTERN,
                               &course->type))
        goto CancelAdd;
    if (inputStringWithRegexCheck("[新增课程] 请输入开课学期\n",
                                  SEMESTER_PATTERN,
                                  course->semester,
                                  10))
        goto CancelAdd;
    if (inputIntWithRegexCheck("[新增课程] 请输入课程开始周数（大于0的整数）\n",
                               NUMBER_PATTERN,
                               &course->week_start))
        goto CancelAdd;
    if (inputIntWithRegexCheck("[新增课程] 请输入课程结束周数（大于0的整数）\n",
                               NUMBER_PATTERN,
                               &course->week_end))
        goto CancelAdd;
    if (inputFloatWithRegexCheck("[新增课程] 请输入课程学分（大于0的小数，最多精确至小数点后2位）\n",
                                 POINTS_PATTERN,
                                 &course->points))
        goto CancelAdd;
    if (inputIntWithRegexCheck("[新增课程] 请输入最大选课人数（大于0的整数）\n",
                               POINTS_PATTERN,
                               &course->max_members))
        goto CancelAdd;
    printf("[新增课程] 按任意键开始编辑授课安排\n");
    getch();
    Schedule ret = editSchedule(NULL);
    memcpy(course->schedule, ret.schedule, sizeof(int[7][13]));

    goto Return;

    CancelAdd:  // 取消添加
    free(course);
    course = NULL;

    Return:  // 返回数据
    return course;
}


/**
 * 生成课表安排的JSON字符
 * @param schedule
 * @return
 */
char *getScheduleJsonString(int schedule[7][13]) {
    char *final_str = (char *) calloc(2000, sizeof(char));
    strcat(final_str, "{");
    if (final_str == NULL) return NULL;
    for (int i = 0; i < 7; i++) {
        char t_str[30];
        sprintf(t_str, "\"%d\":[", i + 1);
        strcat(final_str, t_str);
        char schedule_str[100] = "";
        char tt[10] = "";
        char has_course = 0;
        for (int j = 1; j <= 12; j++) {
            if (schedule[i][j]) {
                if (has_course) strcat(schedule_str, ",");
                has_course = 1;
                sprintf(tt, "%d", j);
                strcat(schedule_str, tt);
            }
        }
        strcat(final_str, schedule_str);
        if (i != 6)
            strcat(final_str, "],");
        else
            strcat(final_str, "]}");
    }
    return final_str;

}


/**
 * 新增/修改课程
 *
 * @param _course 当为NULL时，新增课程，否则为修改课程
 * @return 课程指针
 */
void editCourse(Course *_course) {
    // 如无课程信息，则新增课程
    int action = 0;
    Course *course = _course;

    if (course == NULL) {
        course = addCourse();
        action = 1;
        if (course == NULL) {
            return;
        }
    }

    int counter = 0, selected = 0, key;

    EditCourse_Refresh:

    counter = 0;
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= 课程·新增/修改课程 =======\n\n", 71);

    selfPlusPrint("\t\t课程ID        %s\n", &counter, selected, UTF8ToGBK(course->course_id)); // 0
    selfPlusPrint("\t\t课程名称      %s\n", &counter, selected, UTF8ToGBK(course->title)); // 1
    selfPlusPrint("\t\t课程简介      %s\n", &counter, selected, UTF8ToGBK(course->description)); // 2
    selfPlusPrint("\t\t课程性质      %s\n", &counter, selected, LECTURE_TYPE[course->type]); // 3
    selfPlusPrint("\t\t开课学期      %s\n", &counter, selected, UTF8ToGBK(course->semester)); // 4
    selfPlusPrint("\t\t授课周数      第%d周~第%d周\n", &counter, selected, course->week_start, course->week_end); // 5
    char *courseArrangeStr = printSchedule(course->schedule); // 6
    if (courseArrangeStr != NULL) {
        selfPlusPrint("\t\t授课安排      %s\n", &counter, selected, courseArrangeStr);
        free(courseArrangeStr);
    }
    selfPlusPrint("\t\t授课教师      %s(UID:%s)\n", &counter, selected, UTF8ToGBK(course->teacher.name), // 7
                  UTF8ToGBK(course->teacher.uid));
    selfPlusPrint("\t\t选修人数      %d/%d人\n", &counter, selected, course->current_members, course->max_members);  //8
    selfPlusPrint("\t\t课程学分      %.2f\n", &counter, selected, course->points);  //9
    selfPlusPrint("\t\t课程学时      %d学时\n", &counter, selected, // 10
                  getTotalWeekHour(course->schedule) * (course->week_end - course->week_start + 1));

    printf("\n");
    printInMiddle_GBK("<Enter>修改选中行 <Y>提交修改 <Esc>取消修改", 71);

    EditCourse_GetKey:

    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 80: // 下
                    if (selected < 10) selected++;
                    else selected = 0;
                    goto EditCourse_Refresh;
                case 72: // 上
                    if (selected > 0) selected--;
                    else selected = 10;
                    goto EditCourse_Refresh;
                default:
                    break;
            }
            break;
        case 13:
            switch (selected) {
                case 0:
                    inputStringWithRegexCheck("[修改课程] 请输入课程ID（长度为5-30，由字母、数字和-组成的字符串，建议格式<学年>-<学期>-<课程代号>-<班级>）\n",
                                              COURSE_ID_PATTERN,
                                              course->course_id,
                                              30);
                    break;
                case 1:
                    inputStringWithRegexCheck("[修改课程] 请输入课程标题（长度为5-50字符，由字母、数字、中文和()-组成的字符串）\n",
                                              COURSE_TITLE_PATTERN,
                                              course->title,
                                              100);
                    break;
                case 2:
                    inputStringWithRegexCheck("[修改课程] 请输入课程描述（长度为5-250字符）\n",
                                              COURSE_DESCRIPTION_PATTERN,
                                              course->description,
                                              500);
                    break;
                case 3:
                    inputIntWithRegexCheck("[修改课程] 请输入课程类型：0-必修、1-选修、2-公选、3-辅修\n",
                                           COURSE_TYPE_PATTERN,
                                           &course->type);
                    break;
                case 4:
                    inputStringWithRegexCheck("[修改课程] 请输入开课学期\n",
                                              SEMESTER_PATTERN,
                                              course->semester,
                                              10);
                    break;
                case 5:
                    if (inputIntWithRegexCheck("[修改课程] 请输入课程开始周数（大于0的整数）\n",
                                               NUMBER_PATTERN,
                                               &course->week_start) == -1)
                        break;
                    inputIntWithRegexCheck("[修改课程] 请输入课程结束周数（大于0的整数）\n",
                                           NUMBER_PATTERN,
                                           &course->week_end);
                    break;
                case 6: {
                    Schedule ret = editSchedule(course->schedule);
                    memcpy(course->schedule, ret.schedule, sizeof(int[7][13]));
                }
                    break;
                case 7:
                    // 管理员可修改开课教师
                    if (GLOBAL_user_info.role == 2) {
                        inputStringWithRegexCheck("[修改课程] 请输入开课教师ID（用户角色需为教师或管理员）\n",
                                                  USER_PATTERN,
                                                  course->teacher.uid,
                                                  15);
                    }
                    break;
                case 9:
                    inputFloatWithRegexCheck("[修改课程] 请输入课程学分（大于0的小数，最多精确至小数点后2位）\n",
                                             POINTS_PATTERN,
                                             &course->points);
                    break;
                case 8:
                    inputIntWithRegexCheck("[修改课程] 请输入最大选课人数（大于0的整数）\n",
                                           POINTS_PATTERN,
                                           &course->max_members);
                    break;
                default:
                    break;
            }
            goto EditCourse_Refresh;
        case 'y':
        case 'Y':
            if (course->week_start <= course->week_end && course->week_start > 0 && course->max_members > 0 &&
                course->points > 0) {
                printf("\n[提示] 正在请求服务器...\n");
                cJSON *req_json = cJSON_CreateObject();
                cJSON_AddItemToObject(req_json, "action", cJSON_CreateNumber(action));
                cJSON_AddItemToObject(req_json, "course_id", cJSON_CreateString(course->course_id));
                cJSON_AddItemToObject(req_json, "title", cJSON_CreateString(course->title));
                cJSON_AddItemToObject(req_json, "teacher", cJSON_CreateString(course->teacher.uid));
                cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(course->semester));
                cJSON_AddItemToObject(req_json, "description", cJSON_CreateString(course->description));
                cJSON_AddItemToObject(req_json, "max_members", cJSON_CreateNumber(course->max_members));
                cJSON_AddItemToObject(req_json, "points", cJSON_CreateNumber(course->points));
                cJSON_AddItemToObject(req_json, "week_start", cJSON_CreateNumber(course->week_start));
                cJSON_AddItemToObject(req_json, "week_end", cJSON_CreateNumber(course->week_end));
                cJSON_AddItemToObject(req_json, "type", cJSON_CreateNumber(course->type));
                cJSON_AddItemToObject(req_json, "schedule",
                                      cJSON_CreateString(getScheduleJsonString(course->schedule)));
                ResponseData resp = callBackend("course.submit", req_json);
                free(req_json);
                free(resp.raw_string);
                if (resp.status_code != 0) {
                    printf("[提交失败] 错误码：%d, %s(按任意键继续)\n", resp.status_code, UTF8ToGBK(findExceptionReason(&resp)));
                    getch();
                    cJSON_Delete(resp.json);
                    goto EditCourse_Refresh;
                }
                cJSON_Delete(resp.json);
                printf("[提交成功] 课程修改/新增成功（按任意键返回课程一览）\n");
                getch();
                goto GC_COLLECT;
            } else {
                printf("[提交失败] 开课周应小于等于结束周，且开课周、学分和最大人数应大于0。(按任意键继续)\n");
                getch();
                goto EditCourse_Refresh;
            }
            break;
        case 27:
            goto GC_COLLECT;
        default:
            break;
    }
    goto EditCourse_GetKey;

    GC_COLLECT:
    if (_course == NULL) free(course);
}


/**
 * 导入学生选课列表
 */
void importStuCourseData() {

    typedef struct _import_data {
        char uid[21], course_id[33];
    } ImportData;

    LinkList_Object *import_list = linkListObject_Init(); // 初始化链表
    int page = 1, max_page, page_size = 30, total = 0, key;

    Import_Refresh:

    max_page = (int) ceilf((float) total / (float) page_size);
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(54);
    printf("\n");
    printInMiddle_GBK("======= 课程·导入学生名单 =======\n\n", 56);
    printf("%-6s%-20s%-30s\n", "序号", "学号", "选修课程ID");
    printf("--------------------------------------------------------\n");
    int counter = 0, current_total = 0;
    for (LinkList_Node *pt = import_list->head; pt != NULL; pt = pt->next) {
        counter++;
        if (ceilf((float) counter / (float) page_size) < (float) page) continue; // 定位到该页面，学生名单分页显示
        if (ceilf((float) counter / (float) page_size) > (float) page) break; // 一页显示完即可跳出循环
        current_total++;
        ImportData *tmp = pt->data;
        printf("%4d  %-20s%-30s\n",
               counter,
               UTF8ToGBK(tmp->uid),
               UTF8ToGBK(tmp->course_id));
        if (pt->next == NULL) { // 链表的最后一个节点
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total - 1; i++) printf("\n"); // 补齐页面
    printf("\n");
    printInMiddle_GBK("=============================\n", 56);
    printf("    [提示] 共%4d条数据，当前第%3d页，共%3d页\n\n    （左方向键：前一页；右方向键：后一页）\n\n",
           total, page, max_page);
    printInMiddle_GBK("<T>导出模板 <I>选择文件 <Y>确认导入 <Esc>取消并返回", 56);
    printf("\n");

    Import_GetKey:

    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 75: // 左
                    if (page > 1) page--;
                    else break;
                    goto Import_Refresh;
                case 77: // 右
                    if (page < max_page) page++;
                    else break;
                    goto Import_Refresh;
                default:
                    break;
            }
            break;
        case 'T':
        case 't': {
            FILE *fp = fopen("import_template.csv", "w");
            if (fp == NULL) {
                printf("[提示] 导出模板失败，请检查文件权限。（按任意键继续）\n");
                getch();
                goto Import_Refresh;
            }
            fprintf(fp, "学号,课程ID\n2135060620(示例，请删除),2022-01-0000001-01(示例，请删除)\n");
            fclose(fp);
            printf("[提示] 导出模板成功，模板保存至\"import_template.csv\"文件中。（按任意键继续）\n");
            goto Import_Refresh;
        }
        case 'Y':
        case 'y': { // 批量导入
            if (total == 0) {
                printf("[导入学生失败] 列表为空，请先按\"I\"加载数据（按任意键继续）\n");
                getch();
                goto Import_Refresh;
            }
            cJSON *add_req = cJSON_CreateObject();
            cJSON *add_list = cJSON_CreateArray();
            for (LinkList_Node *pt = import_list->head; pt != NULL; pt = pt->next) {
                cJSON *add_object = cJSON_CreateObject();
                cJSON_AddItemToObject(add_object, "course_id",
                                      cJSON_CreateString(((ImportData *) pt->data)->course_id));
                cJSON_AddItemToObject(add_object, "uid", cJSON_CreateString(((ImportData *) pt->data)->uid));
                cJSON_AddItemToArray(add_list, add_object);
            }
            cJSON_AddItemToObject(add_req, "data", add_list);
            ResponseData add_resp = callBackend("course.adminAdd", add_req);
            cJSON_Delete(add_req);
            free(add_resp.raw_string);
            if (add_resp.status_code != 0) {
                printf("[导入学生失败] 错误码：%d, %s（按任意键继续）\n", add_resp.status_code,
                       UTF8ToGBK(findExceptionReason(&add_resp)));
                getch();
                goto Import_Refresh;
            } else {
                printf("[导入学生成功] 按任意键继续。\n");
                cJSON_Delete(add_resp.json);
                getch();
                goto Import_GC_Collect;
            }
            break;
        }
        case 'I': // 选择文件导入
        case 'i': {
            printf("[提示] 请在打开的对话框中选择文件，以完成进一步的操作...\n");
            char file_path[260] = {0};
            if (openFileDialog(file_path, "TemplateFile\0*.csv\0", "请选择导入模板") == 0) {
                printf("[提示] 用户取消了文件选择（按任意键继续）\n");
                getch();
                goto Import_Refresh;
            }
            char buf[1024];
            FILE *fp = fopen(file_path, "r");
            if (fp == NULL) {
                printf("[提示] 打开文件失败（按任意键继续）\n");
                getch();
                goto Import_Refresh;
            }
            // 建立哈希表，用于去除重复数据
            SimpleHashList *hash_list = hashList_init();
            fgets(buf, sizeof(buf), fp);  // 跳过第一行
            while (fgets(buf, sizeof(buf), fp)) {
                if (hashList_findString(hash_list, buf)) continue; // 如果已经存在该数据，则直接下一行
                hashList_appendString(hash_list, buf);// 加入哈希表
                ImportData *imp = calloc(1, sizeof(ImportData)); // 开辟内存
                buf[strlen(buf) - 1] = '\0'; // 去除行尾换行
                char *p;
                char *ptr = strtok_r(buf, ",", &p); // 字符串分割
                if (ptr == NULL) break; // 读到第一个空行或无效行
                strcpy(imp->uid, GBKToUTF8(ptr));
                ptr = strtok_r(NULL, ",", &p); // 字符串分割
                strcpy(imp->course_id, GBKToUTF8(ptr));
                if (regexMatch(USER_PATTERN, imp->uid) && regexMatch(COURSE_ID_PATTERN, imp->course_id)) {
                    linkListObject_Append(import_list, imp);
                    total++;
                } else {
                    free(imp);
                }
            }
            fclose(fp);
            hashList_delList(hash_list);// 释放哈希表占用内存
            printf("[导入成功] 哈希表去重并校验后，共导入%d条有效数据（按任意键继续）\n", total);
            getch();
            goto Import_Refresh;
            break;
        }
        case 27:
            goto Import_GC_Collect;
        default:
            break;
    }
    goto Import_GetKey;

    Import_GC_Collect:
    linkListObject_Delete(import_list, 1);
    free(import_list);
}