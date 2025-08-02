// #include <am.h>
// #include <klib.h>
// #include <klib-macros.h>
// #include <stdarg.h>

// #if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// int printf(const char *fmt, ...){


// // static int int_to_str(char *out,int num){
// //     char bud[32];
// //     int i=0;
// //     unsigned int n;
// //     int is_negative=0;
// //     if(num<0){
// //         is_negative=1;
// //         n=
// // int vsprintf(char *out, const char *fmt, va_list ap){
// // }

// // int sprintf(char *out, const char *fmt, ...) {
// //   va_list args;
// //   va_start(args, fmt);
// //   char *start = out;  // 记录输出起始地址，用于计算长度

// //   while (*fmt != '\0') {  // 显式判断字符串结束，更清晰
// //     if (*fmt == '%') {
// //       fmt++;  // 移动到格式符
// //       if (*fmt == '\0') {  // 处理末尾单独的 '%'，避免越界
// //         *start++ = '%';    // 直接输出 '%' 后结束
// //         break;
// //       }

// //       switch (*fmt) {
// //         case 'd': {
// //           int num = va_arg(args, int);
// //           char buffer[32];  // 扩大缓冲区（int 最大 10 位数字+符号，32 足够）
// //           int i = 0;
// //           unsigned int n;   // 用 unsigned 避免负数转换溢出

// //           // 处理符号
// //           if (num < 0) {
// //             *start++ = '-';
// //             n = (unsigned int)(-(num + 1)) + 1;  // 安全处理最小负数（如 -2147483648）
// //           } else {
// //             n = (unsigned int)num;
// //           }

// //           // 处理 0 的特殊情况
// //           if (n == 0) {
// //             buffer[i++] = '0';
// //           } else {
// //             // 提取数字（逆序存入缓冲区）
// //             while (n > 0) {
// //               buffer[i++] = '0' + (n % 10);
// //               n /= 10;
// //             }
// //           }

// //           // 逆序输出数字（恢复正确顺序）
// //           while (i > 0) {
// //             *start++ = buffer[--i];
// //           }
// //           fmt++;  // 移动到下一个格式符
// //           break;
// //         }

// //         case 's': {
// //           char *str = va_arg(args, char *);
// //           if (str == NULL) {  // 处理空指针，避免崩溃
// //             const char *null_str = "(null)";  // 模拟标准库行为
// //             while (*null_str) {
// //               *start++ = *null_str++;
// //             }
// //           } else {
// //             // 输出字符串（依赖 str 以 '\0' 结尾，若需更安全可限制长度）
// //             while (*str != '\0') {
// //               *start++ = *str++;
// //             }
// //           }
// //           fmt++;  // 移动到下一个格式符
// //           break;
// //         }

// //         default: {  // 处理未定义的格式符，直接输出 '%' 和字符
// //           *start++ = '%';
// //           *start++ = *fmt;
// //           fmt++;
// //           break;
// //         }
// //       }
// //     } else {  // 非格式符，直接复制字符
// //       *start++ = *fmt++;
// //     }
// //   }

// //   *start = '\0';  // 结尾添加字符串终止符
// //   va_end(args);
// //   return start - out;  // 返回输出的字符数（不含终止符）
// // }
// int snprintf(char *out, size_t n, const char *fmt, ...) {
//   panic("Not implemented");
// }

// int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
//   panic("Not implemented");
// }

// // #endif
// // 　　 case 'x':
// // 　　 itoa(tmp, *((int*)p_next_arg));
// // 　　 strcpy(p, tmp);
// // 　　 p_next_arg += 4;
// // 　　 p += strlen(tmp);
// // 　　 break;
// // 　　 case 's':
// // 　　 break;
// // 　　 default:
// // 　　 break;
// // 　　 }
// // 　　 }
// // 　　
// // 　　 return (p - buf);
// // 　　}
// // }

// // int sprintf(char *out, const char *fmt, ...) {
// //   va_list args;
// //   va_start(args, fmt);
// //   char *start = out;  // 记录输出起始地址，用于计算长度

// //   while (*fmt != '\0') {  // 显式判断字符串结束，更清晰
// //     if (*fmt == '%') {
// //       fmt++;  // 移动到格式符
// //       if (*fmt == '\0') {  // 处理末尾单独的 '%'，避免越界
// //         *start++ = '%';    // 直接输出 '%' 后结束
// //         break;
// //       }

// //       switch (*fmt) {
// //         case 'd': {
// //           int num = va_arg(args, int);
// //           char buffer[32];  // 扩大缓冲区（int 最大 10 位数字+符号，32 足够）
// //           int i = 0;
// //           unsigned int n;   // 用 unsigned 避免负数转换溢出

// //           // 处理符号
// //           if (num < 0) {
// //             *start++ = '-';
// //             n = (unsigned int)(-(num + 1)) + 1;  // 安全处理最小负数（如 -2147483648）
// //           } else {
// //             n = (unsigned int)num;
// //           }

// //           // 处理 0 的特殊情况
// //           if (n == 0) {
// //             buffer[i++] = '0';
// //           } else {
// //             // 提取数字（逆序存入缓冲区）
// //             while (n > 0) {
// //               buffer[i++] = '0' + (n % 10);
// //               n /= 10;
// //             }
// //           }

// //           // 逆序输出数字（恢复正确顺序）
// //           while (i > 0) {
// //             *start++ = buffer[--i];
// //           }
// //           fmt++;  // 移动到下一个格式符
// //           break;
// //         }

// //         case 's': {
// //           char *str = va_arg(args, char *);
// //           if (str == NULL) {  // 处理空指针，避免崩溃
// //             const char *null_str = "(null)";  // 模拟标准库行为
// //             while (*null_str) {
// //               *start++ = *null_str++;
// //             }
// //           } else {
// //             // 输出字符串（依赖 str 以 '\0' 结尾，若需更安全可限制长度）
// //             while (*str != '\0') {
// //               *start++ = *str++;
// //             }
// //           }
// //           fmt++;  // 移动到下一个格式符
// //           break;
// //         }

// //         default: {  // 处理未定义的格式符，直接输出 '%' 和字符
// //           *start++ = '%';
// //           *start++ = *fmt;
// //           fmt++;
// //           break;
// //         }
// //       }
// //     } else {  // 非格式符，直接复制字符
// //       *start++ = *fmt++;
// //     }
// //   }

// //   *start = '\0';  // 结尾添加字符串终止符
// //   va_end(args);
// //   return start - out;  // 返回输出的字符数（不含终止符）
// // }
// int snprintf(char *out, size_t n, const char *fmt, ...) {
//   panic("Not implemented");
// }

// int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
//   panic("Not implemented");
// }

// #endif
