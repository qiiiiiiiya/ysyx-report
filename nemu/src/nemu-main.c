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
#include <common.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 函数声明
void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
word_t expr(char *e, bool *success);

#define MAX_LINE_LEN 1024

int main(int argc, char *argv[]) {
    // 初始化监控器
#ifdef CONFIG_TARGET_AM
    am_init_monitor();
#else
    init_monitor(argc, argv);
#endif

    // 检查是否启用测试模式（通过命令行参数控制）
    bool run_test = true;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            run_test = true;
            break;
        }
    }

    if (run_test) {
        // 测试模式：执行测试用例验证expr()
        FILE *fp = fopen("/home/yyq03/ysyx-workbench/nemu/tools/gen-expr/input", "r");
        if (!fp) {
            perror("无法打开测试用例文件 input");
            return 1;
        }

        int total = 0, passed = 0;
        char line[MAX_LINE_LEN];
        while (fgets(line, MAX_LINE_LEN, fp)) {
            total++;
            line[strcspn(line, "\n")] = '\0';
            if (strlen(line) == 0) continue;

            char *space_pos = strchr(line, ' ');
            if (!space_pos) {
                printf("第%d行格式错误：缺少空格分隔符\n", total);
                continue;
            }
            *space_pos = '\0';
            int expected = atoi(line);
            char *expr_str = space_pos + 1;

            bool success;
            word_t actual = expr(expr_str, &success);

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
        printf("\n测试完成：共 %d 条，通过 %d 条，通过率 %.2f%%\n",
               total, passed, (float)passed / total * 100);

        return (passed == total) ? 0 : 1;
    } else {
        // 正常模式：启动引擎进入后续运行界面
        engine_start();
        return is_exit_status_bad();
    }
}
