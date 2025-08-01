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

#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>

/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INST_TO_PRINT 10//最多打印十条指令

CPU_state cpu = {};//记录CPU的状态（执行到了哪个地址PC，寄存器值等）
uint64_t g_nr_guest_inst = 0;//统计总共执行了多少条“guest”指令
static uint64_t g_timer = 0; // unit: us//记录模拟器运行花费了多少时间（单位微秒）
static bool g_print_step = false;//标记是否需要单步打印指令详情

void device_update();

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {
#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
  watchpoint_check();  // 关键：执行观察点检查
}
static void watchpoint_check() {
  WP *cur = head;  // 假设 head 是观察点链表的头指针（需确保已正确初始化）
  while (cur) {
    bool success;
    word_t new_val = expr(cur->expr, &success);  // 解析表达式
    if (!success) {
      printf("表达式有误：%s\n", cur->expr);
      cur = cur->next;
      continue;
    }
    if (new_val != 0) {  // 表达式结果为非0（真），触发观察点
      printf("\nWatchpoint %d triggered:\n", cur->NO);
      printf("Expression: %s\n", cur->expr);
      printf("Condition is true at pc = 0x%x\n", cpu.pc);  // 输出当前PC
      nemu_state.state = NEMU_STOP;  // 暂停CPU执行
      return;
    }
    cur = cur->next;
  }
}

static void exec_once(Decode *s, vaddr_t pc) {
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  cpu.pc = s->dnpc;
#ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst;
#ifdef CONFIG_ISA_x86
  for (i = 0; i < ilen; i ++) {
#else
  for (i = ilen - 1; i >= 0; i --) {
#endif
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = MUXDEF(CONFIG_ISA_x86, 8, 4);
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst, ilen);
#endif
}

//new 指令环形缓冲区
//只需要记录存储固定数量的指令，满了就把最早的指令覆盖掉，出错的时候把存的指令打印出来
// static void iringbuf(Decode *s, char **inst_ring_buf, int max) {
//   const int max=10;
//   s->logbuf[0] = '\0'; // 初始化日志缓冲区
//   *inst_ring_buf = malloc(max * sizeof(char *));
//   assert(*inst_ring_buf);
//   for (int i = 0; i < max; i++) {
//     (*inst_ring_buf)[i] = NULL; // 初始化指令环形缓冲区
//   }
//   int i=0;
//   for(int j=0;j<max;j++){
//     while(i<max){
//       if((*inst_ring_buf)[i] == NULL) {
//         (*inst_ring_buf)[i] = s->logbuf; // 将当前指令存入环形缓冲区
//         i++;
//         break;
//       }
//       i++;
//     }
//     if(i==max){
//       for(int k=0;k<max-1;k++){
//         (*inst_ring_buf)[k+1] = (*inst_ring_buf)[k]; // 将最早的指令覆盖掉
//       }
//       (*inst_ring_buf)[0] = s->logbuf; // 覆盖最早的指令
//     }
//   }
// }
// //如果出错，就把环形区的指令添加到log日志中
// //new
// static void iringbuf_log_to_file(){
//   FILE *log_fp=fopen("~/nemu-log.txt","a");
//   if(log_fp=NULL){
//     printf("failed to open log file for inst buffer\n");
//     return;
//   }
//   fprintf(log_fp,"\n===== Recent Instruction(Last %d)=====\n",ring_buf_count);
//   for(int i=0;i<10;i++){
//     fprintf(log_fp,"[%d] %s\n",i+1,inst_ring_buf[i]);
//   }
//   fclose(log_fp);
// }

// 全局变量：环形缓冲区及控制参数
#define INST_RING_BUF_SIZE 10  // 固定缓冲区大小为10
static char *inst_ring_buf[INST_RING_BUF_SIZE] = {NULL};  // 存储指令字符串
static int ring_buf_head = 0;  // 指向最新指令的位置
static int ring_buf_count = 0; // 当前存储的指令数量

// 初始化环形缓冲区（只需调用一次）
static void iringbuf_init() {
  // 释放可能存在的旧内存（防止重复初始化导致内存泄漏）
  for (int i = 0; i < INST_RING_BUF_SIZE; i++) {
    if (inst_ring_buf[i] != NULL) {
      free(inst_ring_buf[i]);
      inst_ring_buf[i] = NULL;
    }
  }
  ring_buf_head = 0;
  ring_buf_count = 0;
}

// 向环形缓冲区添加一条指令
static void iringbuf_push(Decode *s) {
  // 复制指令日志（避免原缓冲区被覆盖）
  char *new_log = strdup(s->logbuf);
  assert(new_log != NULL && "Failed to allocate memory for log");

  // 如果缓冲区已满，先释放最早的指令
  if (ring_buf_count >= INST_RING_BUF_SIZE) {
    int oldest_pos = (ring_buf_head) % INST_RING_BUF_SIZE;
    free(inst_ring_buf[oldest_pos]);
    ring_buf_count--; // 临时减少计数，后续会恢复
  }

  // 存入新指令
  int pos = (ring_buf_head + ring_buf_count) % INST_RING_BUF_SIZE;
  inst_ring_buf[pos] = new_log;
  ring_buf_count++;
  // 当缓冲区未满时，head不变；满了之后head跟随最新位置移动
  if (ring_buf_count == INST_RING_BUF_SIZE) {
    ring_buf_head = (ring_buf_head + 1) % INST_RING_BUF_SIZE;
  }
}

// 异常时将环形缓冲区内容写入日志文件
static void iringbuf_log_to_file() {
  // 使用绝对路径或正确的相对路径（避免~在C中无法解析）
  FILE *log_fp = fopen("./nemu-log.txt", "a");
  if (log_fp == NULL) { 
    printf("Failed to open log file for instruction buffer\n");
    return;
  }

  // 按时间顺序输出（从最早到最新）
  fprintf(log_fp, "\n===== Recent Instructions (Last %d) =====\n", ring_buf_count);
  for (int i = 0; i < ring_buf_count; i++) {
    int pos = (ring_buf_head + i) % INST_RING_BUF_SIZE;
    fprintf(log_fp, "[%d] %s\n", i + 1, inst_ring_buf[pos]);
  }

  fclose(log_fp);
}


static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    IFDEF(CONFIG_ITRACE, iringbuf_push(&s));
    if (nemu_state.state != NEMU_RUNNING) break;
    IFDEF(CONFIG_DEVICE, device_update());
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT MUXDEF(CONFIG_TARGET_AM, "%", "%'") PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
}

void assert_fail_msg() {
  isa_reg_display();
  statistic();
}

/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (nemu_state.state) {
    case NEMU_END: case NEMU_ABORT: case NEMU_QUIT:
      printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
      return;
    default: nemu_state.state = NEMU_RUNNING;
  }

  uint64_t timer_start = get_time();

  execute(n);

  uint64_t timer_end = get_time();
  g_timer += timer_end - timer_start;

  switch (nemu_state.state) {
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP; 
    iringbuf_log_to_file();
    
    break;
    
    case NEMU_END: case NEMU_ABORT:
      // if(nemu_state.state=NEMU_ABORT){
      //   iringbuf_log_to_file();
      // }
      
      Log("nemuassecc: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
      // fall through
    case NEMU_QUIT: statistic();
    case NEMU_STOP: return sdb_mainloop();
  }
}  