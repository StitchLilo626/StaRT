#ifndef __LIST_H_
#define __LIST_H_
#include "sdef.h"

/**
 * rt_container_of - return the member address of ptr, if the type of ptr is the
 * struct type.
 */
#define s_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))


/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define s_list_entry(node, type, member) \
    s_container_of(node, type, member)


// 初始化链表节点
void s_list_init(s_plist l);

// 在链表节点 l 之后插入节点 n
void s_list_insert_after(s_plist l,s_plist n);

// 在链表节点 l 之前插入节点 n
void s_list_insert_before(s_plist l,s_plist n);

// 删除链表节点 d
void s_list_delete(s_plist d);

// 判断链表是否为空（返回1为空，0为非空）
int s_list_isempty(s_plist l);

#endif
