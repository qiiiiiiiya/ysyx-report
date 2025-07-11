/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

// #include "sdb.h"

// #define NR_WP 32

// typedef struct watchpoint {
//   int NO;
//   struct watchpoint *next;

//   /* TODO: Add more members if necessary */

// } WP;

// static WP wp_pool[NR_WP] = {};
// static WP *head = NULL, *free_ = NULL;

// void init_wp_pool() {
//   int i;
//   for (i = 0; i < NR_WP; i ++) {
//     wp_pool[i].NO = i;
//     wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
//   }

//   head = NULL;
//   free_ = wp_pool;
// }

// /* TODO: Implement the functionality of watchpoint */

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  word_t old_value;
  char expr[100];
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

//链表初始化
void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}
  
static WP* new_wp(){
  //没有空闲的监视点就直接终止程序
  assert(free_);
  WP* ret=free_;
  free_=free_->next;
  ret->next=head;
  head=ret;
  return ret;
}

static void free_wp(WP *wp){
  WP* h=head;
  if(h==wp) head=NULL;
  else{
    while(h&&h->next!=wp)h=h->next;
    assert(h);
    h->next=wp->next;
  }
  wp->next=free_;
  free_=wp;
}
 /* TODO: Implement the functionality of watchpoint */
// void wp_watch(char *expr,word_t res){
//     WP* wp=new_wp();
//     strcpy(wp->expr,expr);
//     wp->old_value=res;
//     bool success;
//     word_t new_value = expr(wp->expr, &success);
//     printf("Watchpoint %d:%s\n",wp->NO,expr);
//     printf("Old value: %d\n",wp->old_value);
//     printf("New value: %d\n",new_value);
//     //检查表达式是否变化  
// }

void wp_watch(char *expr_str, word_t res) {
    WP* wp = new_wp();
    strcpy(wp->expr, expr_str);
    wp->old_value = res;
    
    bool success;  // 声明success变量
    word_t new_value = expr(wp->expr, &success);  // 调用全局expr函数
    
    if (success) {
        printf("Watchpoint %d: %s\n", wp->NO, expr_str);
        printf("Old value: %u\n", wp->old_value);
        printf("New value: %u\n", new_value);
    } else {
        printf("Failed to evaluate expression\n");
        free_wp(wp);  // 表达式无效时释放监视点
    }
}


void wp_remove(int no){
    assert(no<NR_WP);
    WP* wp=&wp_pool[no];
    free_wp(wp);
    printf("Delete watchpoint %d: %s\n",wp->NO,wp->expr);
}

// void wp_iterate(){
//     WP* h=head;
//     if(!h){
//         puts("No watchpoints.");
//         return;
//     }
//     printf("%-8s%-8s%-8s\n","Num","Name","old_values","new_values");
//     while(h){
//         printf("%-8d%-8s%-8d\n",h->NO,h->expr,h->old_value,new_value);
//         h=h->next;
//     }
// }

void wp_iterate() {
    WP* h = head;
    if (!h) {
        puts("No watchpoints.");
        return;
    }
    printf("%-8s%-8s%-8s%-8s\n", "Num", "Expr", "Old Value", "New Value");
    while (h) {
        bool success;
        word_t new_value = expr(h->expr, &success);  // 动态计算新值
        printf("%-8d%-20s%-12u", h->NO, h->expr, h->old_value);
        if (success) {
            printf("%u", new_value);
            if (new_value != h->old_value) {
                printf("Changed!");  // 标记值变化
            }
        } else {
            printf("%s", "compute error");  // 表达式计算失败
        }
        printf("\n");
        h = h->next;
    }
}
