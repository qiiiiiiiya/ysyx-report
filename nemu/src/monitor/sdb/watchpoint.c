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
void wp_watch(const char *expr,word_t res){
    WP* wp=new_wp();
    strcpy(wp->expr,expr);
    wp->old_value=res;
    printf("Watchpoint %d:%s\n",wp->NO,expr);
}
void wp_remove(int no){
    assert(no<NR_WP);
    WP* wp=&wp_pool[no];
    free_wp(wp);
    printf("Delete watchpoint %d: %s\n",wp->NO,wp->expr);
}

void wp_iterate(){
    WP* h=head;
    if(!h){
        puts("No watchpoints.");
        return;
    }
    printf("%-8s%-8s%-8s\n","Num","What","types");
    while(h){
        printf("%-8d%-8s%-8d\n",h->NO,h->expr,h->old_value);
        h=h->next;
    }
}
bool check_watchpoint() {
	bool check = false;
	bool success = false;
	uint32_t temp = 0;
	for(int i = 0; i < NR_WP; i++) {
		if(wp_pool[i].expr[0] != '\0') {
			temp = expr(wp_pool[i].expr, &success);
			if(temp != wp_pool[i].old_value) {
				printf("Watchpoint %d: %s\n", wp_pool[i].NO, wp_pool[i].expr);
				printf("old value: %d\n", wp_pool[i].old_value);
				printf("new value: %d\n\n", temp);
				wp_pool[i].old_value = temp;
				check = true;
			}
		}
	}
	return check;
}