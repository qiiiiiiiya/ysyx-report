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
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
// #include <assert.h>
// #include <string.h>

// static char buf[65536] = {};
// static size_t buf_pos = 0;
// static char code_buf[65536 + 128] = {};
// static char *code_format =
// "#include <stdio.h>\n"
// "#include <stdint.h>\n"
// "int main() { "
// "  int result = /*(uint32_t)*/(%s); "
// "  printf(\"%%u\", result); "
// "  return 0; "
// "}";

// int choose(int n) {
//     return rand() % n;
// }

// static void gen(char c) {
//     buf[buf_pos++] = c;
// }

// // 生成普通数字（允许0）
// static void gen_num() {
//     int num = rand() % 100;
//     int len = 0, tmp = num;
//     while (tmp) {
//         tmp /= 10;
//         len++;
//     }
//     int x = (len <= 1) ? 1 : (len - 1) * 10;
//     while (num) {
//         char c = num / x + '0';
//         buf[buf_pos++] = c;
//         num %= x;
//         x /= 10;
//     }
//     if (len == 0) {
//         buf[buf_pos++] = '0';
//     }
// }

// static char gen_rand_op() {
//     char op[4] = {'+', '-', '*', '/'};
//     int pos = rand() % 4;
//     buf[buf_pos++] = op[pos];
//     return op[pos];
// }

// static void gen_rand_expr() {
//     if (buf_pos > 65530) {
//         printf("oversize\n");
//         return;
//     }
//     switch (choose(3)) {
//         case 0:
//             gen_num();
//             break;
//         case 1:
//             gen('(');
//             gen_rand_expr();
//             gen(')');
//             break;
//         default: {
//             gen_rand_expr();
            
//             gen_rand_op();
            
//             gen_rand_expr();
//             break;
//         }
//     }
// }

// int main(int argc, char *argv[]) {
//     int seed = time(0);
//     srand(seed);
//     int loop = 1;
//     if (argc > 1) {
//         sscanf(argv[1], "%d", &loop);
//     }
//     int i;
//     for (i = 0; i < loop; i++) {
//         buf_pos = 0;
//         gen_rand_expr();
//         buf[buf_pos] = '\0';

//         sprintf(code_buf, code_format, buf);

//         FILE *fp = fopen("/tmp/.code.c", "w");
//         assert(fp != NULL);
//         fputs(code_buf, fp);
//         fclose(fp);
//         // 编译语句不变
//         int ret = system("gcc /tmp/.code.c -o /tmp/.expr ");
//         // 2>/dev/null
//         if (ret != 0) continue;


//         // 运行并检查
//         fp = popen("/tmp/.expr", "r");
//         if (fp == NULL) continue;
//         if (fp) {
//         char line[1024];
//         while (fgets(line, sizeof(line), fp)) {
//         if (strstr(line, "warning: division by zero") || 
//             strstr(line, "warning: integer overflow")) {
//             pclose(fp);
//             continue; // 跳过含警告的表达式
        

        
//         }
//     }
// }
//         int result;
//         ret = fscanf(fp, "%d", &result);
//         pclose(fp);
//         printf("%u %s\n", result, buf); // 输出%d

        
//     }
//     return 0;
// }
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

static char buf[65536] = {};
static size_t buf_pos = 0;
static char code_buf[65536 + 128] = {};
static char *code_format =
"#include <stdio.h>\n"
"#include <stdint.h>\n"
"int main() { "
"  int result = /*(uint32_t)*/(%s); "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

int choose(int n) {
    return rand() % n;
}

static void gen(char c) {
    buf[buf_pos++] = c;
}

// 生成普通数字（允许0）
static void gen_num() {
    int num = rand() % 100;
    int len = 0, tmp = num;
    while (tmp) {
        tmp /= 10;
        len++;
    }
    int x = (len <= 1) ? 1 : (len - 1) * 10;
    while (num) {
        char c = num / x + '0';
        buf[buf_pos++] = c;
        num %= x;
        x /= 10;
    }
    if (len == 0) {
        buf[buf_pos++] = '0';
    }
}

static char gen_rand_op() {
    char op[4] = {'+', '-', '*', '/'};
    int pos = rand() % 4;
    buf[buf_pos++] = op[pos];
    return op[pos];
}

static void gen_rand_expr() {
    if (buf_pos > 65530) {
        gen('$');
        printf("oversize\n");
        return;
    }
    switch (choose(3)) {
        case 0:
            gen_num();
            break;
        case 1:
            gen('(');
            gen_rand_expr();
            gen(')');
            break;
        default: {
            gen_rand_expr();
            
            gen_rand_op();
            
            gen_rand_expr();
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    int seed = time(0);
    srand(seed);
    int loop = 1;
    if (argc > 1) {
        sscanf(argv[1], "%d", &loop);
    }
    int i;
    for (i = 0; i < loop; i++) {
        buf_pos = 0;
        gen_rand_expr();
        buf[buf_pos] = '\0';

        sprintf(code_buf, code_format, buf);

        FILE *fp = fopen("/tmp/.code.c", "w");
        assert(fp != NULL);
        fputs(code_buf, fp);
        fclose(fp);

        
        int ret = system("gcc -Wall -Werror /tmp/.code.c -o /tmp/.expr 2>/dev/null");

        if (ret != 0) continue;

        // 运行并检查
        fp = popen("/tmp/.expr", "r");
        if (fp == NULL) continue;
        
        
        int result;
        ret = fscanf(fp, "%d", &result);
        pclose(fp);
        printf("%u %s\n", result, buf); // 输出%d

        
    }
    return 0;
}