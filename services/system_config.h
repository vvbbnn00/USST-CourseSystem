//
// Created by admin on 2022/7/2.
//

#ifndef PROJECT_SYSTEM_CONFIG_H
#define PROJECT_SYSTEM_CONFIG_H

typedef struct semester_limit__ {
    char semester[11]; // 学期
    double max_points; // 学期最大学分
} SemesterLimit;

extern void printSemesterData();

#endif //PROJECT_SYSTEM_CONFIG_H
