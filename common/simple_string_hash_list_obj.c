//
// Created by admin on 2022/7/2.
//

/*
 * 简单实现哈希表，大致思路如下
 * 计算哈希方法：遍历字符串，将每个值对应的ASCII码相加并乘以127再 % MAX_LENGTH，得到最终的值
 * 解决哈希冲突方法：哈希表指向链表头，遇到冲突则移动至链表尾部新增（此处为了节省内存，开辟了简易版的链表数据结构）
 */

#include <stdlib.h>
#include <string.h>
#include "simple_string_hash_list_obj.h"

/**
 * 获取string的相对Hash
 *
 * @param string
 * @return
 */
unsigned int getStringHash(char *string) {
    char *pt = string;
    unsigned int hash = 0;
    int seed = 127;
    while (*pt != '\0') {
        hash = (hash * seed + *pt) % (HASH_LIST_MAX_LENGTH);
        pt++;
    }
    return hash % (HASH_LIST_MAX_LENGTH);
}

/**
 * 初始化哈希表
 *
 * @return
 */
SimpleHashList *hashList_init() {
    SimpleHashList *hash = calloc(1, sizeof(SimpleHashList));
    hash->list_length = 0;
    hash->records = malloc(0);
    return hash;
}

/**
 * 哈希表插入字符串，记录字符串的内存地址
 *
 * @param list
 * @param string
 * @return
 */
int hashList_appendString(SimpleHashList *list, char *string) {
    unsigned int hash = getStringHash(string); // 计算字符串哈希值
    SimpleHashList_Node *head = list->address[hash];
    if (head == NULL) {
        list->address[hash] = calloc(1, sizeof(SimpleHashList_Node));
        if (list->address[hash] == NULL) return 1;
        list->records = realloc(list->records, sizeof(SimpleHashList_Node) * (list->list_length + 1));
        list->records[list->list_length] = list->address[hash];
        list->list_length++;
        list->address[hash]->data = calloc(strlen(string) + 1, sizeof(char));
        strcpy(list->address[hash]->data, string);
        return 0;
    }
    // 如果已经存在值，则定位到最后一个满足的位置
    while (head->next != NULL) {
        head = head->next;
    }
    head->next = malloc(sizeof(SimpleHashList_Node));
    if (head->next == NULL) return 0;
    head->next->data = string;
    return 0;
}

/**
 * 查找哈希表是否存在该元素
 *
 * @param list
 * @param string
 * @return 找到返回1 找不到返回0
 */
char hashList_findString(SimpleHashList *list, char *string) {
    unsigned int hash = getStringHash(string); // 计算字符串哈希值
    if (list->address[hash] == NULL) return 0;
    SimpleHashList_Node *pt = list->address[hash];
    // 如果哈希存在，则顺次判断至链表尾部
    while (pt != NULL) {
        if (strcmp(pt->data, string) == 0) return 1;
        pt = pt->next;
    }
    return 0;
}

/**
 * 释放哈希表占用内存
 *
 * @param list
 */
void hashList_delList(SimpleHashList *list) {
    for (int i = 0; i < list->list_length; i++) {
        SimpleHashList_Node *pt = list->records[i], *tmp;
        while (pt != NULL) {
            tmp = pt->next;
            free(pt->data);
            free(pt);
            pt = tmp;
        }
    }
    free(list->records);
    free(list);
}