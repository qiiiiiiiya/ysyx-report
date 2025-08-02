#include "ftrace.h"
#include <common.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>

#define MAX_SYMBOLS 1024

static FuncSymbol func_table[MAX_SYMBOLS];
static int func_count = 0;
static const char *current_func = "???";
static int call_depth = 0;

// 比较函数用于qsort
static int compare_func_symbol(const void *a, const void *b) {
  const FuncSymbol *fa = (const FuncSymbol *)a;
  const FuncSymbol *fb = (const FuncSymbol *)b;
  if (fa->addr < fb->addr) return -1;
  if (fa->addr > fb->addr) return 1;
  return 0;
}

const char *find_function_name(uint32_t pc) {
  // 二分查找
  int left = 0, right = func_count - 1;
  while (left <= right) {
    int mid = (left + right) / 2;
    if (pc >= func_table[mid].addr && 
        pc < func_table[mid].addr + func_table[mid].size) {
      return func_table[mid].name;
    } else if (pc < func_table[mid].addr) {
      right = mid - 1;
    } else {
      left = mid + 1;
    }
  }
  return "???";
}

void trace_func_entry(vaddr_t pc) {
  const char *new_func = find_function_name(pc);
  if (strcmp(new_func, current_func) != 0) {
    current_func = new_func;
    printf("%*s--> %s() [0x%08x]\n", call_depth * 2, "", current_func, pc);
  }
}

void trace_func_call(vaddr_t pc, vaddr_t dnpc) {
  const char *caller = find_function_name(pc);
  const char *callee = find_function_name(dnpc);
  
  call_depth++;
  printf("%*s  call %s() -> %s() [0x%08x]\n", 
         call_depth * 2, "", caller, callee, dnpc);
}

void trace_func_ret(vaddr_t pc) {
  const char *func = find_function_name(pc);
  if (call_depth > 0) call_depth--;
  printf("%*s  return from %s() [0x%08x]\n", 
         call_depth * 2, "", func, pc);
}

void init_ftrace(const char *elf_file) {
  if (!elf_file) {
    printf("Ftrace: No ELF file provided\n");
    return;
  }
  
  int fd = open(elf_file, O_RDONLY);
  if (fd < 0) {
    perror("Ftrace: Failed to open ELF file");
    return;
  }
  
  // 读取ELF头
  Elf32_Ehdr ehdr;
  if (read(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
    close(fd);
    printf("Ftrace: Failed to read ELF header\n");
    return;
  }
  
  // 验证ELF魔数
  if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
    close(fd);
    printf("Ftrace: Invalid ELF magic number\n");
    return;
  }
  
  // 读取节头表
  lseek(fd, ehdr.e_shoff, SEEK_SET);
  Elf32_Shdr shdrs[ehdr.e_shnum];
  if (read(fd, shdrs, sizeof(Elf32_Shdr) * ehdr.e_shnum) != 
      sizeof(Elf32_Shdr) * ehdr.e_shnum) {
    close(fd);
    printf("Ftrace: Failed to read section headers\n");
    return;
  }
  
  // 读取节名字符串表
  Elf32_Shdr shstrtab_hdr = shdrs[ehdr.e_shstrndx];
  char *shstrtab = malloc(shstrtab_hdr.sh_size);
  if (!shstrtab) {
    close(fd);
    printf("Ftrace: Memory allocation failed\n");
    return;
  }
  
  lseek(fd, shstrtab_hdr.sh_offset, SEEK_SET);
  if (read(fd, shstrtab, shstrtab_hdr.sh_size) != shstrtab_hdr.sh_size) {
    free(shstrtab);
    close(fd);
    printf("Ftrace: Failed to read section name string table\n");
    return;
  }
  
  // 查找符号表和字符串表
  Elf32_Shdr *symtab_hdr = NULL, *strtab_hdr = NULL;
  for (int i = 0; i < ehdr.e_shnum; i++) {
    char *name = shstrtab + shdrs[i].sh_name;
    if (strcmp(name, ".symtab") == 0) symtab_hdr = &shdrs[i];
    if (strcmp(name, ".strtab") == 0) strtab_hdr = &shdrs[i];
  }
  
  if (!symtab_hdr || !strtab_hdr) {
    free(shstrtab);
    close(fd);
    printf("Ftrace: Symbol table or string table not found\n");
    return;
  }
  
  // 读取字符串表
  char *strtab = malloc(strtab_hdr->sh_size);
  if (!strtab) {
    free(shstrtab);
    close(fd);
    printf("Ftrace: Memory allocation failed\n");
    return;
  }
  
  lseek(fd, strtab_hdr->sh_offset, SEEK_SET);
  if (read(fd, strtab, strtab_hdr->sh_size) != strtab_hdr->sh_size) {
    free(strtab);
    free(shstrtab);
    close(fd);
    printf("Ftrace: Failed to read string table\n");
    return;
  }
  
  // 读取符号表
  int num_symbols = symtab_hdr->sh_size / sizeof(Elf32_Sym);
  Elf32_Sym *syms = malloc(symtab_hdr->sh_size);
  if (!syms) {
    free(strtab);
    free(shstrtab);
    close(fd);
    printf("Ftrace: Memory allocation failed\n");
    return;
  }
  
  lseek(fd, symtab_hdr->sh_offset, SEEK_SET);
  if (read(fd, syms, symtab_hdr->sh_size) != symtab_hdr->sh_size) {
    free(syms);
    free(strtab);
    free(shstrtab);
    close(fd);
    printf("Ftrace: Failed to read symbol table\n");
    return;
  }
  
  // 提取函数符号
  func_count = 0;
  for (int i = 0; i < num_symbols && func_count < MAX_SYMBOLS; i++) {
    if (ELF32_ST_TYPE(syms[i].st_info) == STT_FUNC && syms[i].st_size > 0) {
      func_table[func_count].name = strdup(strtab + syms[i].st_name);
      func_table[func_count].addr = syms[i].st_value;
      func_table[func_count].size = syms[i].st_size;
      func_count++;
    }
  }
  
  // 排序符号表
  qsort(func_table, func_count, sizeof(FuncSymbol), compare_func_symbol);
  
  // 清理资源
  free(syms);
  free(strtab);
  free(shstrtab);
  close(fd);
  
  printf("Ftrace: Initialized with %d function symbols\n", func_count);
  // 在初始化完成后打印符号表
  for (int i = 0; i < func_count; i++) {
    printf("Symbol %d: %s @ 0x%08x (size: %u)\n", 
           i, func_table[i].name, 
           func_table[i].addr, 
           func_table[i].size);
  }
}
