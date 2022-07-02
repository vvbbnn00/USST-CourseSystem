//
// Created by admin on 2022/6/26.
//

#ifndef PROJECT_COURSE_H
#define PROJECT_COURSE_H

#include "user.h"

// 用于传递一个完整的数组，获取后再使用memcpy传递给course的schedule
typedef struct schedule_ {
    int schedule[7][13];
} Schedule;

typedef struct course {
    char course_id[33], // 课程ID
    title[101],     // 课程标题
    description[501], // 课程描述
    semester[11]; // 开课学期
    User teacher; // 开课老师
    int type, // 课程类型：0-必修 1-选修 2-公选 3-辅修
    week_start, // 开课周
    week_end, // 结课周
    current_members, // 当前报名人数
    max_members; // 最大可报名人数
    double points; // 学分
    int schedule[7][13]; // 每星期课程安排
} Course;

struct studentCourseSelection { // 学生选课用
    int status; // 是否可选该门课程
    char locked_reason[100]; // 该门课不能选择的原因
    long long selection_time; // 选择该课程的时间
    Course course; // 课程信息
};

struct teacherCourseSelection { // 学生选课管理结构体
    User student; // 学生信息
    Course course; // 课程信息
    long long selection_time; // 选择该课程的时间
};


extern void printStudentCourseSelection();

extern void printStudentLectureTable();

extern void printAllCourses(int scene);

extern void editCourse(Course *_course);

extern Schedule editSchedule(int schedule[7][13]);

extern void importStuCourseData();

void printStudentList(Course *courseData);

int inputStringWithRegexCheck(char *message, char *pattern, char *_dest, int max_length);

#endif //PROJECT_COURSE_H
