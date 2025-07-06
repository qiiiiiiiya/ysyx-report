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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>
#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);
/*new*/
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
/*new*/

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  /**/
  { "si", "Single-step exection",cmd_si},
  { "info", "printf",cmd_info},
  { "x", "Scan memory", cmd_x},
  { "q", "Exit NEMU", cmd_q },
  { "p", "expression",cmd_p},
  
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}
/**/
static int cmd_si(char *args)
{
    char *arg=strtok(NULL," ");
    int num_per=0;
    if(arg==NULL)
    {
        cpu_exec(1);
    }else{
        num_per=atoi(arg);
        if(arg==0){
            printf("Unknown input,please try it in a standard format");
            return 0;}
        cpu_exec(num_per);}
    return 0;
}

//打印寄存器
static int cmd_info(char *args)
{
    char *arg=strtok(NULL," ");
    if(strcmp(arg, "r")==0) isa_reg_display();
    /*
    else if(strcmp(arg,"w")==0) display_wp();
    */
    else printf("Unknown input,please try it in a standard format");
    return 0;
}


//扫描内存

static int cmd_x(char *args)
{
    char *arg1=strtok(NULL," ");
    if(arg1==NULL) {
        printf("Unknown input,please try it in a standard format");
        return 0;
    }
    char *arg2=strtok(NULL," ");
    if(arg2==NULL) {
printf("Unknown input,please try it in a standard format");
        return 0;
    }
    int n=strtol(arg1,NULL,10);
    vaddr_t base_addr=strtol(arg2,NULL,16);
    for(int i=0;i<n;)
    {
        printf("0x%08x:",base_addr);
        for(int j=0;i<n&&j<4;i++,j++)
        {
            word_t data=vaddr_read(base_addr,4);
            printf("0x%08x\t",data);
            base_addr+=4;
        }
        printf("\n");
    }
    return 0;
}

/*static int cmd_p(char *args){
    char *e=strtok(args,"\n");
    bool t=true;
    expr(e,&t);
    return 0;
}*/
/*
static int cmd_p(char *args) {
  if (args == NULL || *args == '\0') {
    printf("Usage: p <expression>\n");
    return;
  }
  char *e=strtok(args,"\n");
  bool t=true;
  expr(e,&t);
  //extern const int *result[];
  if (t) {
    // 输出结果（支持十六进制和十进制）
    printf("0x%x (%u)\n", (unsigned)result, (unsigned)result);
  } else {
    printf("Failed to evaluate expression: %s\n", args);
  }
  return 0;
}
*/
static int cmd_p(char *args) {
  if (args == NULL || *args == '\0') {  // 检查是否有输入的表达式
    printf("Usage: p <expression>\n");
    return 0;
  }

  bool success;  // 标记表达式计算是否成功
  // 调用expr计算表达式，将结果保存到result变量中
  word_t result = expr(args, &success);

  if (success) {
    // 输出结果（以十六进制和十进制两种形式）
    printf("0x%x (%u)\n", (unsigned)result, (unsigned)result);
  } else {
    printf("Failed to evaluate expression: %s\n", args);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
