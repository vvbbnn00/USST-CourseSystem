//
// Created by admin on 2022/7/2.
//

#ifndef PROJECT_SIMPLE_STRING_HASH_LIST_OBJ_H
#define PROJECT_SIMPLE_STRING_HASH_LIST_OBJ_H

// 哈希表最大长度为1024*1024
#define HASH_LIST_MAX_LENGTH 1024 * 1024

typedef struct hashNode__ {
    struct hashNode__ *next; // 后指针
    char *data; // 字符串存储
} SimpleHashList_Node;

typedef struct hash_list__ {
    SimpleHashList_Node *address[HASH_LIST_MAX_LENGTH]; // 哈希主表，用于快速定位
    SimpleHashList_Node **records; // 变长数组，顺序记录节点内存地址，便于释放
    unsigned int list_length; // 哈希列表长度
} SimpleHashList;

extern SimpleHashList *hashList_init();

extern int hashList_appendString(SimpleHashList *list, char *string);

extern char hashList_findString(SimpleHashList *list, char *string);

extern void hashList_delList(SimpleHashList *list);

#endif //PROJECT_SIMPLE_STRING_HASH_LIST_OBJ_H
