
#ifndef __FTRACE_H__
#define __FTRACE_H__

#include <common.h>
#include <stdint.h>

typedef struct {
  char *name;     // 函数名
  uint32_t addr;  // 函数起始地址
  uint32_t size;  // 函数大小
} FuncSymbol;

void init_ftrace(const char *elf_file);
const char *find_function_name(uint32_t pc);
void trace_func_entry(vaddr_t pc);
void trace_func_call(vaddr_t pc, vaddr_t dnpc);  // 移除第三个参数
void trace_func_ret(vaddr_t pc);

#endif