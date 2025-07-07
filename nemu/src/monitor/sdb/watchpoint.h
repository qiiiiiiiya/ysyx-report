#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"  // 提供word_t等基础类型定义

// 单个watchpoint的结构体定义
typedef struct watchpoint {
  int NO;               // 唯一编号（0~NR_WP-1）
  struct watchpoint *next;  // 链表节点，用于串联活跃的watchpoint
  char expr[100];       // 被监控的表达式字符串（如"$t0 + 0x20"）
  word_t old_value;     // 表达式的旧值（用于检测变化）
} WP;

// 全局函数声明（供外部调用）
#ifdef CONFIG_WATCHPOINT
// 初始化watchpoint池（分配空闲节点）
void init_wp_pool(void);

// 创建新的watchpoint（监控表达式，记录当前值）
// 参数：expr-表达式字符串；res-表达式当前值
void wp_watch(const char *expr, word_t res);

// 删除指定编号的watchpoint
// 参数：no-要删除的watchpoint编号
void wp_remove(int no);

// 遍历并打印所有活跃的watchpoint
void wp_iterate(void);

// 全局链表头（指向第一个活跃的watchpoint，供cpu.c检查时遍历）
extern WP *head;
#else
// 未开启配置时，空定义避免编译错误
#define init_wp_pool() do {} while (0)
#define wp_watch(expr, res) do {} while (0)
#define wp_remove(no) do {} while (0)
#define wp_iterate() do {} while (0)
#endif  // CONFIG_WATCHPOINT

#endif  // __WATCHPOINT_H__
