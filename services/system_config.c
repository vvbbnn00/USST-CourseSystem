//
// Created by admin on 2022/7/2.
//

#include "system_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "ui_gbk.h"
#include "global.h"
#include "string_ext.h"
#include "link_list_object.h"
#include <Windows.h>

const char *SORT_METHOD_SEMESTER[] = {"学期降序", "学期升序"};

/**
 * 解析学期限制数据，存入指针对应结构体
 *
 * @param json
 * @param _dest
 * @return 0 成功 非零 失败
 */
int parseSemesterData(cJSON *json, SemesterLimit *_dest) {
    if (jsonParseFloat(json, "max_points", &_dest->max_points)) {
        printErrorData("学期信息数据解析失败，请返回重试。");
        return -1;
    }
    jsonParseString(json, "semester", _dest->semester);
    return 0;
}


/**
 * 新增学期限制
 * @return 取消则返回NULL，否则返回学期配置指针
 */
SemesterLimit *addSemesterLimit() {
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= 系统设置·新增学期配置 =======\n", 71);

    SemesterLimit *semester = calloc(1, sizeof(SemesterLimit));
    if (inputStringWithRegexCheck("[新增学期] 请输入学期（长度为 4-10 字符，由字母、数字和-组成的字符串，建议格式<学年>-<学期>）\n",
                                  SEMESTER_PATTERN,
                                  semester->semester,
                                  10) == -1)
        goto CancelAdd;
    if (inputFloatWithRegexCheck("[新增学期] 请设置学期最大学分，最高精确至两位小数\n",
                                 POINTS_PATTERN,
                                 &semester->max_points) == -1)
        goto CancelAdd;
    goto Return;

    CancelAdd:  // 取消添加
    free(semester);
    semester = NULL;

    Return:  // 返回数据
    return semester;
}


/**
 * 新增/修改学期配置
 *
 * @param _semester 当为NULL时，新增学期配置，否则为修改学期配置
 * @return 课程指针
 */
void editSemester(SemesterLimit *_semester) {
    // 如无学期信息，则新增学期
    int action = 0;
    SemesterLimit *semester = _semester;

    if (semester == NULL) {
        semester = addSemesterLimit();
        action = 1;
        if (semester == NULL) {
            return;
        }
    }

    int counter = 0, selected = 0, key;

    EditSemester_Refresh:

    counter = 0;
    system("chcp 936>nul & cls & MODE CON COLS=75 LINES=55");
    printf("\n");
    ui_printHeader_GBK(69);
    printf("\n");
    printInMiddle_GBK("======= 系统设置·新增/修改学期配置 =======\n\n", 71);

    selfPlusPrint("\t\t学期           %s\n", &counter, selected, UTF8ToGBK(semester->semester)); // 0
    selfPlusPrint("\t\t最大学分限制   %.2f\n", &counter, selected, semester->max_points); // 1
    printf("\n");
    printInMiddle_GBK("<Enter>修改选中行 <Y>提交修改 <Esc>取消修改", 71);

    EditSemester_GetKey:

    key = _getch();
    switch (key) {
        case 224:
            key = _getch();
            switch (key) {
                case 80: // 下
                    if (selected < 1) selected++;
                    else selected = 0;
                    goto EditSemester_Refresh;
                case 72: // 上
                    if (selected > 0) selected--;
                    else selected = 1;
                    goto EditSemester_Refresh;
                default:
                    break;
            }
            break;
        case 13:
            if (selected == 1) {
                inputFloatWithRegexCheck("[修改学分限制] 请设置学期最大学分，最高精确至两位小数\n",
                                         POINTS_PATTERN,
                                         &semester->max_points);
            }
            goto EditSemester_Refresh;
        case 'y':
        case 'Y': {
            printf("\n[提示] 正在请求服务器...\n");
            cJSON *req_json = cJSON_CreateObject();
            cJSON_AddItemToObject(req_json, "action", cJSON_CreateNumber(action));
            cJSON_AddItemToObject(req_json, "semester", cJSON_CreateString(semester->semester));
            cJSON_AddItemToObject(req_json, "max_points", cJSON_CreateNumber(semester->max_points));
            ResponseData resp = callBackend("semester.submit", req_json);
            free(req_json);
            free(resp.raw_string);
            if (resp.status_code != 0) {
                printf("[提交失败] 错误码：%d, %s(按任意键继续)\n", resp.status_code, UTF8ToGBK(findExceptionReason(&resp)));
                getch();
                cJSON_Delete(resp.json);
                goto EditSemester_Refresh;
            }
            cJSON_Delete(resp.json);
            printf("[提交成功] 学期信息修改/新增成功（按任意键返回）\n");
            getch();
            goto GC_COLLECT;
        }
        case 27:
            goto GC_COLLECT;
        default:
            break;
    }
    goto EditSemester_GetKey;

    GC_COLLECT:
    if (_semester == NULL) free(semester);
}


/**
 * 输出学期信息等
 */
void printSemesterData() {
    HANDLE windowHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    char search_kw[36] = "";
    int sort_method = 0; // 排序方法 0 - 学期升序 1 - 学期降序
    int total, page = 1, max_page, page_size = 15, current_total;

    LinkList_Node *selectedRow = NULL; // 被选中的行
    cJSON *req_json = NULL;
    LinkList_Object *semester_data_list = NULL;

    Semester_GetAndDisplay:

    system("chcp 936>nul & cls & MODE CON COLS=60 LINES=50");
    // ... 请求过程
    printf("[提示] 正在请求服务器...\n");
    req_json = cJSON_CreateObject();
    cJSON_AddItemToObject(req_json, "page", cJSON_CreateNumber(page));
    cJSON_AddItemToObject(req_json, "size", cJSON_CreateNumber(page_size));
    cJSON_AddItemToObject(req_json, "kw", cJSON_CreateString(GBKToUTF8(search_kw)));
    cJSON_AddItemToObject(req_json, "sort_method", cJSON_CreateNumber(sort_method));
    ResponseData resp = callBackend("semester.getLimitList", req_json);
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
    jsonParseString(ret_json, "school", GLOBAL_school);
    jsonParseString(ret_json, "current_semester", GLOBAL_semester);

    // 解析学期信息数据
    semester_data_list = linkListObject_Init(); // 链表初始化
    cJSON *data = cJSON_GetObjectItem(ret_json, "data");
    for (int i = 0; i < current_total; i++) {
        cJSON *row_data = cJSON_GetArrayItem(data, i);
        SemesterLimit *semester_data_pt = calloc(1, sizeof(SemesterLimit));
        if (parseSemesterData(row_data, semester_data_pt)) return;
        LinkList_Node *node = linkListObject_Append(semester_data_list, semester_data_pt); // 链表尾部追加元素
        if (node == NULL) {
            printErrorData("内存错误");
        }
        if (i == 0) selectedRow = node;
    }
    cJSON_Delete(ret_json);
    free(resp.raw_string);

    Semester_Refresh:

    system("cls");
    printf("\n");
    ui_printHeader_GBK(38);
    printf("\n");
    printInMiddle_GBK("======= 系统配置·学期信息一览 =======\n", 38);
    printf("       %-14s%-10s       \n", "学期", "最大可选学分");
    printf("      --------------------------\n");
    for (LinkList_Node *pt = semester_data_list->head; pt != NULL; pt = pt->next) {
        SemesterLimit *tmp = pt->data;
        printf("      ");
        if (pt == selectedRow) SetConsoleTextAttribute(windowHandle, 0x70);
        printf("%-14s%-10.2f\n",
               UTF8ToGBK(tmp->semester),
               tmp->max_points);
        SetConsoleTextAttribute(windowHandle, 0x07);
        if (pt->next == NULL) { // 链表的最后一个节点
            printf("\n");
        }
    }
    for (int i = 0; i < page_size - current_total; i++) printf("\n"); // 补齐页面
    printf("\n");
    printInMiddle_GBK("=============================\n", 38);
    printf("[当前搜索条件] ");
    if (strlen(search_kw) > 0) printf("模糊搜索=%s AND ", search_kw);
    printf("排序方式=%s\n", SORT_METHOD_SEMESTER[sort_method]);
    printf("\n[当前学期] %s [当前学校名称] %s\n\n", UTF8ToGBK(GLOBAL_semester), UTF8ToGBK(GLOBAL_school));
    printf("[提示] 共%4d条数据，当前第%3d页，共%3d页\n（左右方向：前/后一页；上/下方向：切换选中数据）\n",
           total, page, max_page);
    printf("\n<A>新增学期 <Enter>编辑学期信息 <D>删除学期\n<K>学期模糊查询 <S>排序切换至%s <Z>修改学校名称\n<X>修改当前学期为选中行 <Esc>返回主菜单\n",
           SORT_METHOD_SEMESTER[sort_method + 1 > 1 ? 0 : sort_method + 1]);

    if (selectedRow == NULL) {
        if (strlen(search_kw)) {
            printErrorData("没有查询到符合条件的学期");
            strcpy(search_kw, "");
            goto Semester_GetAndDisplay;
        } else {
            printf("暂无学期，您是管理员，是否新建学期？(Y)");
            int ch = getch();
            if (ch == 'Y' || ch == 'y') {
                editSemester(NULL);
                goto Semester_GetAndDisplay;
            } else {
                goto GC_Collect;
            }
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
                    if (selectedRow->next == NULL) selectedRow = semester_data_list->head;
                    else selectedRow = selectedRow->next;
                    goto Semester_Refresh;
                case 72: // 上
                    if (selectedRow->prev == NULL) selectedRow = semester_data_list->foot;
                    else selectedRow = selectedRow->prev;
                    goto Semester_Refresh;
                case 75: // 左
                    page = (page > 1) ? (page - 1) : 1;
                    selectedRow = 0;
                    goto Semester_GetAndDisplay;
                case 77: // 右
                    page = (page < max_page) ? (page + 1) : (max_page);
                    selectedRow = 0;
                    goto Semester_GetAndDisplay;
                default:
                    break;
            }
            break;
        case 's': // 修改排序顺序
        case 'S':
            selectedRow = 0;
            sort_method++;
            if (sort_method > 1) sort_method = 0;
            goto Semester_GetAndDisplay;
        case 'k': // 搜索关键词
        case 'K':
            printf("\n[模糊搜索] 请输入关键词（以\"Enter\"键结束）：");
            gets_safe(search_kw, 35);
            fflush(stdin);
            selectedRow = 0;
            goto Semester_GetAndDisplay;
        case 'A':
        case 'a': // 新建学期
            editSemester(NULL);
            goto Semester_GetAndDisplay;
        case 'D':
        case 'd': // 删除学期
        {
            SemesterLimit *pt = selectedRow->data;
            printf("\n[确认删除] 您确定要删除学期 %s 吗？（若要继续，请输入%s确认）\n",
                   UTF8ToGBK(pt->semester), UTF8ToGBK(pt->semester));
            char input_char[31];
            gets_safe(input_char, 30);
            if (strcmp(input_char, pt->semester) != 0) {
                printf("学期不一致，已取消操作（按任意键继续）。\n");
                getch();
                goto Semester_GetAndDisplay;
            }
            cJSON *cancel_req_json = cJSON_CreateObject();
            cJSON_AddItemToObject(cancel_req_json, "semester", cJSON_CreateString(pt->semester));
            ResponseData cancel_resp = callBackend("semester.deleteSemester", cancel_req_json);
            cJSON_Delete(cancel_req_json);
            free(cancel_resp.raw_string);
            if (cancel_resp.status_code != 0) {
                printf("[删除失败] 错误码：%d, %s（按任意键继续）\n", cancel_resp.status_code,
                       UTF8ToGBK(findExceptionReason(&cancel_resp)));
                cJSON_Delete(cancel_resp.json);
                getch();
                goto Semester_GetAndDisplay;
            }
            printf("[删除成功] 学期 %s 已被删除（按任意键继续）。\n", UTF8ToGBK(pt->semester));
            getch();
            goto Semester_GetAndDisplay;
        }
        case 13: // 编辑学期
            editSemester((SemesterLimit *) selectedRow->data);
            goto Semester_GetAndDisplay;
        case 'x':
        case 'X': // 修改学期为选中行
        {
            cJSON *req = cJSON_CreateObject();
            cJSON_AddItemToObject(req, "semester", cJSON_CreateString(((SemesterLimit *) selectedRow->data)->semester));
            struct responseData x_resp = callBackend("semester.changeSemester", req);
            cJSON_Delete(req);
            free(x_resp.raw_string);
            if (x_resp.status_code != 0) {
                printf("[修改学期失败] 错误码：%d, %s（按任意键继续）\n", x_resp.status_code, UTF8ToGBK(findExceptionReason(&x_resp)));
            } else {
                printf("[修改学期成功] 当前学期已被设置为%s（按任意键继续）\n", UTF8ToGBK(((SemesterLimit *) selectedRow->data)->semester));
                strcpy(GLOBAL_semester, ((SemesterLimit *) selectedRow->data)->semester);
            }
            cJSON_Delete(x_resp.json);
            getch();
            goto Semester_GetAndDisplay;
        }
        case 'z':
        case 'Z': // 修改学校名称
        {
            char school_name[100];
            if (inputStringWithRegexCheck("[修改校名] 请设置校名（长度为3-50字符，由字母、数字、中文和空格组成的字符串）\n", SCHOOL_PATTERN, school_name,
                                          99) == -1)
                goto Semester_GetAndDisplay;
            cJSON *req = cJSON_CreateObject();
            cJSON_AddItemToObject(req, "school_name",
                                  cJSON_CreateString(school_name));
            struct responseData x_resp = callBackend("semester.changeSchoolName", req);
            cJSON_Delete(req);
            free(x_resp.raw_string);
            if (x_resp.status_code != 0) {
                printf("[修改校名失败] 错误码：%d, %s（按任意键继续）\n", x_resp.status_code, UTF8ToGBK(findExceptionReason(&x_resp)));
            } else {
                printf("[修改校名成功] 当前校名已被设置为%s（按任意键继续）\n", UTF8ToGBK(school_name));
                strcpy(GLOBAL_school, school_name);
            }
            cJSON_Delete(x_resp.json);
            getch();
            goto Semester_GetAndDisplay;
        }
        case 27:
            goto GC_Collect;
        default:
            break;
    }
    goto GetKey;

    GC_Collect:
    linkListObject_Delete(semester_data_list, 1);
    free(semester_data_list);
    semester_data_list = NULL;
}
