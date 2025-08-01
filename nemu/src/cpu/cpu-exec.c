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
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include "./../monitor/sdb/ftrace.h"
#include <cpu/ifetch.h>
#include <unistd.h>
#include <fcntl.h>
/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 :*/
#define MAX_INST_TO_PRINT 10//最多打印十条指令

CPU_state cpu = {};//记录CPU的状态（执行到了哪个地址PC，寄存器值等）
uint64_t g_nr_guest_inst = 0;//统计总共执行了多少条“guest”指令
static uint64_t g_timer = 0; // unit: us//记录模拟器运行花费了多少时间（单位微秒）
static bool g_print_step = false;//标记是否需要单步打印指令详情

void device_update();
extern bool check_watchpoint(void);
static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {

 #ifdef CONFIG_WATCHPOINT
  bool check=check_watchpoint();
if(check){
  nemu_state.state = NEMU_STOP;
  printf("Watchpoint triggered, execution paused.\n");
  return;
}
#endif

#ifdef CONFIG_ITRACE_COND
  if (ITRACE_COND) { log_write("%s\n", _this->logbuf); }
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));

}

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
  
static void iringbuf_log_to_file() {
  // 使用Log函数输出到主日志文件
  Log("\n===== Recent Instructions (Last %d) =====", ring_buf_count);
  for (int i = 0; i < ring_buf_count; i++) {
    int pos = (ring_buf_head + i) % INST_RING_BUF_SIZE;
    if (inst_ring_buf[pos] != NULL) {
      Log("[%d] %s", i + 1, inst_ring_buf[pos]);
    }
  }
}


// 添加全局状态变量
static const char *current_func = "???";
static int call_depth = 0;
static vaddr_t last_call_pc = 0;
extern void init_ftrace(const char *elf_file);
extern const char *elf_file;
static void exec_once(Decode *s, vaddr_t pc) {
  Log("nonononono---------------------nonononono");
  int fd = open(elf_file, O_RDONLY);
  if (fd != -1){
  //查找当前函数
  Log("yesyesyes------------------------yesyes");
  const char *new_func = find_function_name(pc);
  
  // 如果进入新函数
  if (new_func != current_func) {
    current_func = new_func;
    // 输出函数入口
    printf("%*s--> %s() [0x%08x]\n", call_depth * 2, "", current_func, pc);
  }
  extern uint32_t inst_fetch(vaddr_t *pc, int len);  // 声明外部函数，指定参数和返回值类型
  // 解码指令
  word_t inst_ = inst_fetch(&s->pc, 4);
  word_t opcode = inst_ & 0x7F;
  word_t rd = (inst_ >> 7) & 0x1F;
  word_t rs1 = (inst_ >> 15) & 0x1F;
  word_t imm = (int32_t)(inst_ & 0xFFF00000) >> 20; // 符号扩展
  
  // JAL指令：函数调用
  if (opcode == 0x6F) {
    if (rd == 1) { // rd=1 (ra) 表示函数调用
      vaddr_t target = s->dnpc;
      const char *target_func = find_function_name(target);
      
      // 记录调用信息
      last_call_pc = pc;
      call_depth++;
      
      // 输出调用信息
      printf("%*s  call %s() -> %s() [0x%08x]\n", 
             call_depth * 2, "", current_func, target_func, target);
    }
  }
  // JALR指令：函数调用或返回
  else if (opcode == 0x67) {
    if (rd != 0) { // 函数调用
      vaddr_t target = s->dnpc;
      const char *target_func = find_function_name(target);
      
      last_call_pc = pc;
      call_depth++;
      
      printf("%*s  call %s() -> %s() [0x%08x]\n", call_depth * 2, "", current_func, target_func, target);
    }
    else if (rd == 0 && rs1 == 1 && imm == 0) { // 函数返回
      if (call_depth > 0) call_depth--;
      printf("%*s  return from %s() [0x%08x]\n", call_depth * 2, "", current_func, pc);
    }
  }
}
  // 原始执行逻辑
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

static void execute(uint64_t n) {
  Decode s;
  for (;n > 0; n --) {
    exec_once(&s, cpu.pc);
    iringbuf_push(&s);
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
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
  iringbuf_init();
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
    case NEMU_RUNNING: nemu_state.state = NEMU_STOP;iringbuf_log_to_file(); break;
    
    case NEMU_END: case NEMU_ABORT:
    
      Log("nemuaccess: %s at pc = " FMT_WORD,
          (nemu_state.state == NEMU_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
           (nemu_state.halt_ret == 0 ? ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
            ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
          nemu_state.halt_pc);
          if(nemu_state.state == NEMU_ABORT) iringbuf_log_to_file();
          if(nemu_state.halt_ret!=0) iringbuf_log_to_file();
      // fall through
    case NEMU_QUIT: statistic();
  }
}
