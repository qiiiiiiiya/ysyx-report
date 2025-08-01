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
#include <memory/paddr.h>
#include <elf.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include "sdb/ftrace.h"

#define error_exit(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0) // 提前定义

void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm();

static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
}

#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int difftest_port = 1234;

static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}
const char *elf_file = NULL;
static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
    //new
    {"elf"      , required_argument, NULL, 'e'},
    //new
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:f:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 'e': elf_file = optarg ;break;
      case 1: img_file = optarg; return 0;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\t-e,--elf=FILE           specify ELF file path (for NEMU)\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_ftrace(const char *elf_file); // 添加到文件开头的函数声明区域

#define error_exit(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)
// 用于存储函数符号信息的结构体
typedef struct {
    char *name;     // 函数名
    uint32_t addr;  // 函数起始地址
    uint32_t size;  // 函数大小
} FunctSymbol;
// 全局变量，用于存储从ELF文件中读取的函数符号信息
static FunctSymbol *funct_symbols = NULL;
static int funct_count = 0;
static char *strtab_data = NULL; // 字符串表的持久化存储
/*
 * C 语言 `qsort` 所需的比较函数
 * 这替代了原代码中错误的C++ lambda表达式
 */
static int compare_funct_symbols(const void *a, const void *b) {
    const FunctSymbol *sym_a = (const FunctSymbol *)a;
    const FunctSymbol *sym_b = (const FunctSymbol *)b;
    if (sym_a->addr < sym_b->addr) return -1;
    if (sym_a->addr > sym_b->addr) return 1;
    return 0;
}
/*
 * 初始化 ftrace，从指定的 ELF 文件中读取符号表。
 * 这整合并修正了你提供的 process_32elf 和 main 函数的逻辑。
 */
const char *find_function_name(word_t target_addr){
        for(int i=0;i<funct_count;i++){
            if(funct_symbols[i].addr <= target_addr && target_addr < funct_symbols[i].addr+funct_symbols[i].size){
                return funct_symbols[i].name;
            }
        }
        return 0;
    }

    
void init_ftrace(const char *elf_file) {
    if (elf_file == NULL) {
        printf("Ftrace: No ELF file provided.\n");
        return;
    }
    // 使用 open 打开文件，返回一个整数类型的文件描述符
    int fd = open(elf_file, O_RDONLY);
    if (fd == -1) {
        // 使用 perror 打印更详细的错误信息
        fprintf(stderr, "Ftrace: Failed to open ELF file '%s'", elf_file);
        perror("");
        return;
    }
    Elf32_Ehdr ehdr;
    Elf32_Shdr *shdrs = NULL;
    char *shstrtab = NULL;
    Elf32_Sym *symbols_temp = NULL;
    // 读取ELF头部
    if (pread(fd, &ehdr, sizeof(ehdr), 0) != sizeof(ehdr)) {
        fprintf(stderr, "Ftrace: Failed to read ELF header.\n");
        goto cleanup;
    }
    // 校验ELF魔数
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Ftrace: '%s' is not a valid ELF file.\n", elf_file);
        goto cleanup;
    }
    // 读取节头表
    size_t shdrs_size = ehdr.e_shentsize * ehdr.e_shnum;
    shdrs = malloc(shdrs_size);
    if (!shdrs) { fprintf(stderr, "Ftrace: Memory allocation failed for section headers.\n"); goto cleanup; }
    if (pread(fd, shdrs, shdrs_size, ehdr.e_shoff) != shdrs_size) {
        fprintf(stderr, "Ftrace: Failed to read section headers.\n");
        goto cleanup;
    }
    // 读取节名字符串表
    Elf32_Shdr *shstrtab_hdr = &shdrs[ehdr.e_shstrndx];
    shstrtab = malloc(shstrtab_hdr->sh_size);
    if (!shstrtab) { fprintf(stderr, "Ftrace: Memory allocation failed for section name string table.\n"); goto cleanup; }
    if (pread(fd, shstrtab, shstrtab_hdr->sh_size, shstrtab_hdr->sh_offset) != shstrtab_hdr->sh_size) {
        fprintf(stderr, "Ftrace: Failed to read section name string table.\n");
        goto cleanup;
    }
    // 查找符号表 (.symtab) 和字符串表 (.strtab)
    Elf32_Shdr *symtab_hdr = NULL, *strtab_hdr = NULL;
    for (int i = 0; i < ehdr.e_shnum; i++) {
        char *sec_name = shstrtab + shdrs[i].sh_name;
        if (strcmp(sec_name, ".symtab") == 0) {
            symtab_hdr = &shdrs[i];
        }
        if (strcmp(sec_name, ".strtab") == 0) {
            strtab_hdr = &shdrs[i];
        }
    }
    if (!symtab_hdr || !strtab_hdr) {
        fprintf(stderr, "Ftrace: Symbol table or string table not found.\n");
        goto cleanup;
    }

    printf("\n======Section Headers(Total:%d)======\n",ehdr.e_shnum);
    printf("%-20s %-10s %-10s %-10s %-10s\n","Name","Type","Addr","Offset","Size");
    printf("==================================================\n");

    for(int i=0;i<ehdr.e_shnum;i++){
      Elf32_Shdr *shdr=&shdrs[i];
      char *sec_name=shstrtab+shdr->sh_name;
      printf("%-20s",sec_name);
      printf("0x%08x",shdr->sh_type);
      printf("0x%08x",shdr->sh_addr);
      printf("0x%08x",shdr->sh_offset);
      printf("0x%08x\n",shdr->sh_size);

    }

    // 读取字符串表数据到全局变量 strtab_data
    strtab_data = malloc(strtab_hdr->sh_size);
    if (!strtab_data) { fprintf(stderr, "Ftrace: Memory allocation failed for string table.\n"); goto cleanup; }
    if (pread(fd, strtab_data, strtab_hdr->sh_size, strtab_hdr->sh_offset) != strtab_hdr->sh_size) {
        fprintf(stderr, "Ftrace: Failed to read string table.\n");
        free(strtab_data); strtab_data = NULL; // 如果失败，释放内存
        goto cleanup;
    }
    // 读取符号表到一个临时缓冲区
    int num_symbols = symtab_hdr->sh_size / symtab_hdr->sh_entsize;
    symbols_temp = malloc(symtab_hdr->sh_size);
    if (!symbols_temp) { fprintf(stderr, "Ftrace: Memory allocation failed for symbol table.\n"); goto cleanup; }
    if (pread(fd, symbols_temp, symtab_hdr->sh_size, symtab_hdr->sh_offset) != symtab_hdr->sh_size) {
        fprintf(stderr, "Ftrace: Failed to read symbol table.\n");
        goto cleanup;
    }
    // 打印字符串表（strtab）
printf("===== String Table (strtab) =====\n");
printf("Size: %u bytes\n", (unsigned int)strtab_hdr->sh_size);
printf("Entries:\n");
// 字符串表以空字符('\0')分隔，遍历所有字符串
const char *str = (const char *)strtab_data;
const char *end = str + strtab_hdr->sh_size;
while (str < end) {
    // 打印字符串在表中的偏移量和内容
    printf("  Offset 0x%04x: \"%s\"\n",
           (unsigned int)(str - (const char *)strtab_data),
           str);
    // 移动到下一个字符串（跳过当前字符串和空字符）
    str += strlen(str) + 1;
    // 防止空字符串导致无限循环（应对表末尾可能的连续空字符）
    if (str >= end) break;
}

// 打印符号表（symtab）
printf("\n===== Symbol Table (symtab) =====\n");
printf("Number of symbols: %d\n", num_symbols);
printf("Each entry size: %u bytes\n", (unsigned int)symtab_hdr->sh_entsize);
printf("Entries (index: value type name):\n");

// 遍历符号表中的每个符号（假设是32位ELF，使用Elf32_Sym）
for (int i = 0; i < num_symbols; i++) {
    // 根据ELF位数选择对应结构（32位用Elf32_Sym，64位用Elf64_Sym）
    Elf32_Sym *sym = (Elf32_Sym *)symbols_temp + i;

    // 解析符号类型（st_info的高4位）
    unsigned char type = ELF32_ST_TYPE(sym->st_info);
    const char *type_str;
    switch (type) {
        case STT_NOTYPE:   type_str = "NOTYPE"; break;
        case STT_OBJECT:   type_str = "OBJECT"; break;
        case STT_FUNC:     type_str = "FUNC";   break;
        case STT_SECTION:  type_str = "SECTION";break;
        case STT_FILE:     type_str = "FILE";   break;
        default:           type_str = "UNKNOWN";
    }

    // 符号名称在字符串表中的偏移量 -> 从strtab中获取名称
    const char *name = sym->st_name ? (const char *)strtab_data + sym->st_name : "";

    // 打印符号信息：索引、值（地址）、类型、名称
    printf("  %-4d: 0x%08x  %-7s  %s\n",
           i,
           (unsigned int)sym->st_value,
           type_str,
           name);
}
    // 筛选出 STT_FUNC 类型的符号并计数
    for (int i = 0; i < num_symbols; i++) {
        if (ELF32_ST_TYPE(symbols_temp[i].st_info) == STT_FUNC) {
            funct_count++;
        }
    }
    // 为函数符号分配我们自己的存储空间
    funct_symbols = malloc(sizeof(FunctSymbol) * funct_count);
    if (!funct_symbols) { fprintf(stderr, "Ftrace: Memory allocation failed for function symbols.\n"); funct_count = 0; goto cleanup; }
    // 填充函数符号信息
    int j = 0;
    for (int i = 0; i < num_symbols; i++) {
        if (ELF32_ST_TYPE(symbols_temp[i].st_info) == STT_FUNC) {
            funct_symbols[j].name = strtab_data + symbols_temp[i].st_name;
            funct_symbols[j].addr = symbols_temp[i].st_value;
            funct_symbols[j].size = symbols_temp[i].st_size;
            j++;
        }
    }
    // 对函数符号按地址排序，便于后续查找
    qsort(funct_symbols, funct_count, sizeof(FunctSymbol), compare_funct_symbols);
    printf("Ftrace: Initialized with %d function symbols from '%s'.\n", funct_count, elf_file);
    for(int i=0;i<funct_count;i++){
      printf("funft[%d] : %s\n",i,funct_symbols[i].name);
    }


    
// cleanup 标签和资源释放代码必须在函数内部
cleanup:
    // 释放所有临时分配的内存
    // if (shdrs) free(shdrs);
    // if (shstrtab) free(shstrtab);
    // if (symbols_temp) free(symbols_temp);
    // // 如果初始化未成功，则释放全局资源
    // if ( < funct_count) { // `j`是成功填充的函数数量
    //     if (strtab_data) { free(strtab_data); strtab_data = NULL; }
    //     if (funct_symbols) { free(funct_symbols); funct_symbols = NULL; }
    //     funct_count = 0;
    // }
    // 使用 close() 关闭文件描述符，而不是 pclose()
    if (fd != -1) close(fd);
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */
  // 初始化 ftrace
  // if (elf_file) {

  //   Log("Ftrace initialized with %d function symbols", func_count);
  // }
  /* Parse arguments. */
  parse_args(argc, argv);

  init_ftrace(elf_file);

  /* Set random seed. */
  init_rand();

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize the simple debugger. */
  init_sdb();

  IFDEF(CONFIG_ITRACE, init_disasm());

  /* Display welcome message. */
  welcome();
}
#else // CONFIG_TARGET_AM
static long load_img() {
  extern char bin_start, bin_end;
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}

void am_init_monitor() {
  init_rand();
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif
