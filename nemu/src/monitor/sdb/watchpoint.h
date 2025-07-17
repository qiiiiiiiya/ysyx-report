#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"  // 提供word_t等基础类型定义

typedef struct watchpoint {
  int NO;               // 唯一编号（0~NR_WP-1）
  struct watchpoint *next;  // 链表节点，用于串联活跃的watchpoint
  char expr[100];       // 被监控的表达式字符串（如"$t0 + 0x20"）
  word_t old_value;     // 表达式的旧值（用于检测变化）
} WP;


void init_wp_pool(void);

void wp_watch(const char *expr, word_t res);

void wp_remove(int no);

void wp_iterate(void);


#endif 
