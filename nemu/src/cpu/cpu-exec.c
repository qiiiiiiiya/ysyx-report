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

static void exec_once(Decode *s, vaddr_t pc) {
  // 函数入口跟踪
  IFDEF(CONFIG_FTRACE, trace_func_entry(pc));
  
  s->pc = pc;
  s->snpc = pc;
  isa_exec_once(s);
  
  // 函数调用/返回跟踪
  
    uint32_t inst = s->isa.inst;  // 移除.val
    uint32_t opcode = inst & 0x7F;
    uint32_t rd = (inst >> 7) & 0x1F;
    uint32_t rs1 = (inst >> 15) & 0x1F;
    int32_t imm = (int32_t)(inst & 0xFFF00000) >> 20;
    
    // JAL指令：函数调用
    if (opcode == 0x6F && rd == 1) { // rd=1 (ra) 表示函数调用
      trace_func_call(pc, s->dnpc);
    }
    // JALR指令：函数调用或返回
    else if (opcode == 0x67) {
      if (rd != 0) { // 函数调用
        trace_func_call(pc, s->dnpc);
      }
      else if (rd == 0 && rs1 == 1 && imm == 0) { // 函数返回 (ret)
        trace_func_ret(pc);
      }
    }

  
  cpu.pc = s->dnpc;
  #ifdef CONFIG_ITRACE
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = s->snpc - s->pc;
  int i;
  uint8_t *inst_bytes = (uint8_t *)&s->isa.inst;  // 重命名指针变量
#ifdef CONFIG_ISA_x86
  for (i = 0; i < ilen; i ++) {
#else
  for (i = ilen - 1; i >= 0; i --) {
#endif
    p += snprintf(p, 4, " %02x", inst_bytes[i]);  // 使用新变量名
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
