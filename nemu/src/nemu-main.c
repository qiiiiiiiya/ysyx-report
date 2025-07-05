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
/*
#include <common.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int main(int argc, char *argv[]) {
  Initialize the monitor.
  #ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  Start engine. 
  engine_start();

  return is_exit_status_bad();
}*/

#include <common.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
// 假设expr()函数声明（根据实际NEMU代码调整）
word_t expr(char *e, bool *success);

#define MAX_LINE_LEN 1024

int main(int argc, char *argv[]) {
    // 初始化监控器（保留NEMU原有初始化逻辑）
#ifdef CONFIG_TARGET_AM
    am_init_monitor();
#else
    init_monitor(argc, argv);
#endif

    // 检查参数（使用input文件作为测试用例）
    FILE *fp = fopen("/home/yyq03/ysyx-workbench/nemu/tools/gen-expr/build/input", "r");
    if (!fp) {
        perror("无法打开测试用例文件 input\n");
        return 1;
    }

    int total = 0, passed = 0;
    char line[MAX_LINE_LEN];
    while (fgets(line, MAX_LINE_LEN, fp)) {
        total++;
        // 去除换行符
        line[strcspn(line, "\n")] = '\0';
        if (strlen(line) == 0) continue;

        // 解析预期结果和表达式（格式："结果 表达式"）
        char *space_pos = strchr(line, ' ');
        if (!space_pos) {
            printf("第%d行格式错误：缺少空格分隔符\n", total);
            continue;
        }
        *space_pos = '\0';
        int expected = atoi(line);
        char *expr_str = space_pos + 1;

        // 调用expr()计算表达式
        bool success;
        word_t actual = expr(expr_str, &success);

        // 验证结果
        if (success && actual == expected) {
            passed++;
        } else {
            printf("测试用例 %d 失败：\n", total);
            printf("  表达式: %s\n", expr_str);
            printf("  预期结果: %d\n", expected);
            if (!success) {
                printf("  错误: 表达式解析失败\n");
            } else {
                printf("  实际结果: %d\n", actual);
            }
        }
    }

    fclose(fp);
    // 输出测试总结
    printf("\n测试完成：共 %d 条，通过 %d 条，通过率 %.2f%%\n",
           total, passed, (float)passed / total * 100);

    // 返回是否全部通过（0表示全部通过）
    return (passed == total) ? 0 : 1;
}
