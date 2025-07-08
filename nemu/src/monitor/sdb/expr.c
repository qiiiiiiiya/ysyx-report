/**************************************************************************************
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
// #include <isa.h>

// /* We use the POSIX regex functions to process regular expressions.
//  * Type 'man regex' for more information about POSIX regex functions.
//  */
// #include <regex.h>
// #include <memory/vaddr.h>
// #include <memory/paddr.h>
// typedef struct token {
//     int type;
//     char str[32];
//  } Token;
// extern const char *regs[];
// enum {
//   TK_NOTYPE = 0,
//   TK_PLUS,TK_MINUS,TK_MUL,TK_DIV,//+-*/
//   TK_EQ,TK_NEQ,TK_GT,TK_LT,TK_GE,TK_LE,//==,!=,>,<,>=,<=
//   TK_AND,TK_OR,TK_NUM,TK_LPAREN,TK_RPAREN,//&&,||,十进制整数,(,)
//   TK_HEX,TK_REG,TK_DEREF,TK_MINUS_F//十六进制、寄存器、指针解引用、负数
// /*TODO: Add more token types */

// };

// static struct rule {
//   const char *regex;
//   int token_type;
// } rules[] = {

//   /* TODO: Add more rules.
//    * Pay attention to the precedence level of different rules.
//    */
//     {"0x[0-9a-fA-F]+",TK_HEX},  // 以"0x"开头
//     {"\\&?[a-z][a-z0-9]{0,3}",TK_REG}, // 寄存器
// /*    {"\\-",TK_MINUS_F},//负数*/
//     {" +",TK_NOTYPE},
//     {"\\(",TK_LPAREN},
//     {"\\)",TK_RPAREN},
//     {"\\+",TK_PLUS},
//     {"\\-",TK_MINUS},//减
//     {"\\*",TK_MUL},
//     {"/",TK_DIV},
//     {"==",TK_EQ},
//     {"!=",TK_NEQ},
//     {">=",TK_GE},
//     {"<=",TK_LE},
//     {">",TK_GT},
//     {"<",TK_LT},
//     {"&&",TK_AND},
//     {"\\|\\|",TK_OR},
//     {"[0-9]+",TK_NUM},//十进制
// /*    {"\\*",TK_DEREF},// 指针解引用
// */



// };

// #define NR_REGEX ARRLEN(rules)

// static regex_t re[NR_REGEX] = {};
// static Token tokens[32] __attribute__((used)) ={};
// static int nr_token  __attribute__((used))  = 0;
// /* Rules are used for many times.
//  * Therefore we compile them only once before any usage.
//  */
// void init_regex() {
//   int i;
//   char error_msg[128];
//   int ret;

//   for (i = 0; i < NR_REGEX; i ++) {
//     ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
//     if (ret != 0) {
//       regerror(ret, &re[i], error_msg, 128);
//       panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
//     }
//   }
// }

// static bool check_parentheses(int p,int q){
//     if(tokens[p].type!=TK_LPAREN ||tokens[q].type!=TK_RPAREN)
//         return false;
//     int count=0;
//     for(int i=p;i<=q;i++){
//     if(tokens[i].type==TK_LPAREN) count++;
//     else if(tokens[i].type==TK_RPAREN){
//         count--;
//         if(count<0) return false;
//         }
//         if(count==0 && i!=q) return false;
//     }
//     return count==0;
// }

// static int find(int p,int q){
//     int priority=-1;
//     int pos=-1;
//     int bracket_count=0;
// //只考虑二元操作符
//     static const int OP_PRIORITY[256]={
//         [TK_OR]=1,
//         [TK_AND]=2,
//         [TK_EQ]=3,[TK_NEQ]=3,
//         [TK_LT]=4,[TK_LE]=4,[TK_GT]=4,[TK_GE]=4,
//         [TK_PLUS]=5,[TK_MINUS]=5,
//         [TK_MUL]=6,[TK_DIV]=6,
//     };
//     for (int i = p; i <= q; i++) {
//         if (tokens[i].type == TK_LPAREN)
//             bracket_count++;
//         else if (tokens[i].type == TK_RPAREN)
//             bracket_count--;
//         if (bracket_count == 0 && OP_PRIORITY[tokens[i].type] > priority) {
//             priority = OP_PRIORITY[tokens[i].type];
//             pos = i;
//         }
//     }
//     return pos;
// }
//     static void recognize_deref(){
//         for(int i=0;i<nr_token;i++){
//             if(tokens[i].type==TK_MUL){
//                 if(i==0||
//                     tokens[i-1].type==TK_PLUS||
//                     tokens[i-1].type==TK_MINUS||
//                     tokens[i-1].type==TK_MUL||
//                     tokens[i-1].type==TK_DIV||
//                     tokens[i-1].type==TK_LPAREN||
//                     tokens[i-1].type==TK_EQ||
//                     tokens[i-1].type==TK_NEQ||
//                     tokens[i-1].type==TK_AND||
//                     tokens[i-1].type==TK_OR){
//                     tokens[i].type=TK_DEREF;
//                 }
//             }
//         }
//     }
//     /*标记*/
//     static void recognize_minus(){
//         for(int i=0;i<nr_token;i++){
//             if(tokens[i].type==TK_MINUS){
//                 if(tokens[i-1].type!=TK_NUM)
//                 {
//                     tokens[i].type=TK_MINUS_F;}
//             }
//         }
//     }
// /*标记*/


// static bool make_token(char *e){
//         int position = 0;
//         regmatch_t pmatch;
//         nr_token = 0;
//         while (e[position] != '\0') {
//             int matched=0;
//             for (int i = 0; i < NR_REGEX; i ++) {
//                 if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
//                     char *substr_start = e + position;
//                     int substr_len = pmatch.rm_eo;

//                     //跳过空格
//                     if(rules[i].token_type==TK_NOTYPE){
//                         position+=substr_len;
//                         matched=1;
//                         break;
//                     }

//                     if(nr_token>=32){
//                         fprintf(stderr,"error,超出最大数量限制");
//                         return false;
//                     }
//                     int copy_len =substr_len < 31 ? substr_len : 31;
//                     strncpy(tokens[nr_token].str, substr_start, copy_len);
//                     tokens[nr_token].str[copy_len] = '\0';
//                     tokens[nr_token].type = rules[i].token_type;
//                     // 特殊处理寄存器符号
//                     if (rules[i].token_type == TK_REG) {
//                         tokens[nr_token].type = TK_REG;
//                     } nr_token++;
//                 position += substr_len;
//                 printf("match rules[%d] = \"%s\" at position %d with len %d: %.*s\n",i, rules[i].regex, position - substr_len, substr_len, substr_len, substr_start);
//                 matched = 1;
//                 break;
//             }
//         }
//         if(!matched){
//             fprintf(stderr,"无法识别字符%s",e+position);
//             return false;
//         }
//     }
//     return true;
// }
// /*标记*/
// // 计算寄存器数量（数组长度）
// static int reg_name2idx(const char* reg_name) {
// const int regs_num = 32;
// const char* name = reg_name;
// for(int i=0;i<regs_num;i++){
//     const char* reg=regs[i];
//     if(strcmp(name,reg)==0){
//         return i;
//     }
//     }return -1;
// }
// /*标记*/
// /*
// // 寄存器名转索引（返回-1表示无效）
// static int reg_name2idx(const char* reg_name) {
//     // 处理输入：去掉开头的'$'（统一格式）
//     const char* name = reg_name;
//     if (name[0] == '$') {
//         name++; // 跳过'$'，如"$ra"→"ra"，"$0"→"0"
//     }

//     // 遍历regs数组，查找匹配项
//     for (int i = 0; i < regs_num; i++) {
//         // 处理数组元素：去掉开头的'$'（如"$0"→"0"）
//         const char* reg = regs[i];
//         const char* reg_clean = (reg[0] == '$') ? (reg + 1) : reg;

//         // 比较处理后的字符串
//         if (strcmp(name, reg_clean) == 0) {
//             return i; // 找到，返回索引（数组下标）
//         }
//     }

//     // 未找到
//     return -1;
// }*/

// static long eval(int p, int q,bool *success) {
//     if (p > q){
//         *success=false;
//         return 0;
//     }
//     if (p == q) {  // 单个token处理（数字、寄存器）
//         switch (tokens[p].type) {
//             case TK_NUM:  // 十进制数
//                 return strtol(tokens[p].str, NULL, 10);
//             case TK_HEX:  // 十六进制数（0x开头）
//                 return strtol(tokens[p].str, NULL, 16);
//             case TK_REG: {
//                 const char* reg_name = tokens[p].str; // 输入的寄存器名（如"$ra"或"ra"）
//                 int idx = reg_name2idx(reg_name);     // 转换为索引
//                 if (idx == -1) { // 无效寄存器
//                 *success = false;
//                 return 0;
//             }
//                 return cpu.gpr[idx];
//             }
//             case TK_MINUS_F:{
//                 return -TK_MINUS_F;
//             }
//             default:
//                 *success = false;
//                 return 0;
//         }
//     }

//     if (check_parentheses(p, q)) {  // 处理括号
//         return eval(p + 1, q - 1,success);
//     }
//     int op_pos = find(p, q);
//     if(op_pos==-1){
//         *success=false;
//         return 0;
//     }
// recognize_minus();
//     // 找最高优先级运算符
//     word_t left = eval(p, op_pos - 1,success);
//     if(!*success) return 0;
//     word_t right = eval(op_pos + 1, q,success);
//     if(!*success) return 0;
//     // 计算运算符
//     switch (tokens[op_pos].type) {
        
//         case TK_REG: {
//                 // 使用 isa_reg_str2val API 获取寄存器值
//                 bool reg_success;
//                 word_t reg_val = isa_reg_str2val(tokens[p].str, &reg_success);
//                 if (!reg_success) {
//                     *success = false;
//                     return 0;
//                 }
//                 return reg_val;
//             }
//         case TK_PLUS:
//             return left + right;
//         case TK_MINUS:
//             return left - right;
//         case TK_MUL:
//             return left * right;
//         case TK_DIV:
//         if(right==0){
//             *success=false;
//             return 0;
//         }
//             return left / right;
//         case TK_EQ:
//             return left==right;
//         case TK_NEQ:
//             return left!=right;
//         case TK_GT:
//             return left>right;
//         case TK_LT:
//             return left<right;
//         case TK_GE:
//             return left>=right;
//         case TK_LE:
//             return left<=right;
//         case TK_AND:
//             return left&&right;
//         case TK_OR:
//             return left||right;
//         // 其他运算符（如逻辑、比较）需额外处理
//         default:
//             *success=false;
//             return 0;
//     }
// }

// word_t expr(char *e, bool *success) {
//   if (!make_token(e)) {
//     *success = false;
//     return 0;
//   }
//   recognize_deref();
//   bool eval_success=true;
//   word_t result=eval(0,nr_token-1,&eval_success);
//   *success=eval_success;
//   return result;
//   /* TODO: Insert codes to evaluate the expression. */
//   return 0;
//}
#include <isa.h>
#include <regex.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
typedef struct token {
    int type;
    char str[32];
} Token;
static Token tokens[10000] = {};
static int nr_token = 0;
enum {
    TK_NOTYPE = 0,
    TK_PLUS, TK_MINUS, TK_MUL, TK_DIV,
    TK_EQ, TK_NEQ, TK_GT, TK_LT, TK_GE, TK_LE,
    TK_AND, TK_OR, TK_NUM, TK_LPAREN, TK_RPAREN,
    TK_HEX, TK_REG, TK_DEREF, TK_MINUS_F
};
static struct rule {
    const char *regex;
    int token_type;
} rules[] = {
    {"0x[0-9a-fA-F]+", TK_HEX},          // 十六进制数
    {"\\$?[a-z][a-z0-9]{0,2}", TK_REG},  // 寄存器匹配规则（带或不带$）
    {" +", TK_NOTYPE},                    // 空格
    {"\\(", TK_LPAREN},                   // 左括号
    {"\\)", TK_RPAREN},                   // 右括号
    {"\\+", TK_PLUS},                     // 加号
    {"\\-", TK_MINUS},                    // 减号
    {"\\*", TK_MUL},                      // 乘号
    {"/", TK_DIV},                        // 除号
    {"==", TK_EQ},                        // 等于
    {"!=", TK_NEQ},                       // 不等于
    {">=", TK_GE},                        // 大于等于
    {"<=", TK_LE},                        // 小于等于
    {">", TK_GT},                         // 大于
    {"<", TK_LT},                         // 小于
    {"&&", TK_AND},                       // 逻辑与
    {"\\|\\|", TK_OR},                    // 逻辑或
    {"[0-9]+", TK_NUM},                   // 十进制数
};
#define NR_REGEX ARRLEN(rules)
static regex_t re[NR_REGEX] = {};
void init_regex() {
    char error_msg[128];
    for (int i = 0; i < NR_REGEX; i++) {
        int ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}
static void recognize_deref() {
    for (int i = 0; i < nr_token; i++) {
        if (tokens[i].type == TK_MUL) {
            if (i == 0 ||
                tokens[i-1].type == TK_PLUS ||
                tokens[i-1].type == TK_MINUS ||
                tokens[i-1].type == TK_MUL ||
                tokens[i-1].type == TK_DIV ||
                tokens[i-1].type == TK_LPAREN ||
                tokens[i-1].type == TK_EQ ||
                tokens[i-1].type == TK_NEQ ||
                tokens[i-1].type == TK_AND ||
                tokens[i-1].type == TK_OR) {
                tokens[i].type = TK_DEREF;
            }
        }
    }
}
static void recognize_minus() {
    for (int i = 0; i < nr_token; i++) {
        if (tokens[i].type == TK_MINUS) {
            if (i == 0 ||
                tokens[i-1].type == TK_PLUS ||
                tokens[i-1].type == TK_MINUS ||
                tokens[i-1].type == TK_MINUS_F ||
                tokens[i-1].type == TK_MUL ||
                tokens[i-1].type == TK_DIV ||
                tokens[i-1].type == TK_LPAREN ||
                tokens[i-1].type == TK_EQ ||
                tokens[i-1].type == TK_NEQ ||
                tokens[i-1].type == TK_AND ||
                tokens[i-1].type == TK_OR) {
                tokens[i].type = TK_MINUS_F;
            }
        }
    }
}
static bool check_parentheses(int p, int q) {
    if (tokens[p].type != TK_LPAREN || tokens[q].type != TK_RPAREN)
        return false;
    int count = 0;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LPAREN) count++;
        else if (tokens[i].type == TK_RPAREN) {
            count--;
            if (count < 0) return false;
            if (count == 0 && i != q) return false;
        }
    }
    return count == 0;
}

static int find_operator(int p, int q) {
    int bracket_count = 0;
    int op_pos = -1;
    int min_priority = 999;
    // 二元运算符优先级定义（同之前）
    const int priority[] = {
        [TK_OR]     = 1,
        [TK_AND]    = 2,
        [TK_EQ]     = 3, [TK_NEQ] = 3,
        [TK_LT]     = 4, [TK_LE] = 4, [TK_GT] = 4, [TK_GE] = 4,
        [TK_PLUS]   = 5, [TK_MINUS] = 5,
        [TK_MUL]    = 6, [TK_DIV] = 6,
    };
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LPAREN) bracket_count++;
        else if (tokens[i].type == TK_RPAREN) bracket_count--;
        else if (bracket_count == 0) {
            // 明确匹配所有二元运算符（排除一元运算符）
            if (tokens[i].type == TK_OR || tokens[i].type == TK_AND ||
                tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ ||
                tokens[i].type == TK_LT || tokens[i].type == TK_LE ||
                tokens[i].type == TK_GT || tokens[i].type == TK_GE ||
                tokens[i].type == TK_PLUS || tokens[i].type == TK_MINUS ||
                tokens[i].type == TK_MUL || tokens[i].type == TK_DIV) {
                int op_priority = priority[tokens[i].type];
                // 优先级相同则选择右侧运算符（左结合）
                if (op_priority <= min_priority) {
                    min_priority = op_priority;
                    op_pos = i;
                }
            }
        }
    }
    return op_pos;
}

static word_t eval(int p, int q, bool *success) {
    if (p > q) {
        *success = false;
        return 0;
    }

    // 处理一元运算符（负号和解引用）
    if (tokens[p].type == TK_MINUS_F || tokens[p].type == TK_DEREF) {
        // 递归计算一元运算符的操作数（从p+1到q的完整子表达式）
        word_t val = eval(p + 1, q, success);
        if (!*success) return 0;
        switch (tokens[p].type) {
            case TK_MINUS_F: return -val;
            case TK_DEREF: return vaddr_read(val, 4);
            default: return 0;
        }
    }
    if (p == q) {
        switch (tokens[p].type) {
            case TK_NUM:  return strtol(tokens[p].str, NULL, 10);
            case TK_HEX:  return strtol(tokens[p].str, NULL, 16);
            case TK_REG: {
                bool reg_success;
                word_t reg_val = isa_reg_str2val(tokens[p].str, &reg_success);
                if (!reg_success) {
                    *success = false;
                    return 0;
                }
                return reg_val;
            }
            default:
                *success = false;
                return 0;
        }
    }
    if (check_parentheses(p, q)) {
        return eval(p + 1, q - 1, success);
    }
    int op_pos = find_operator(p, q);
    if (op_pos == -1) {
        *success = false;
        return 0;
    }
    word_t left_val = eval(p, op_pos - 1, success);
    if (!*success) return 0;
    word_t right_val = eval(op_pos + 1, q, success);
    if (!*success) return 0;
    switch (tokens[op_pos].type) {
        case TK_PLUS:   return left_val + right_val;
        case TK_MINUS:  return left_val - right_val;
        case TK_MUL:    return left_val * right_val;
        case TK_DIV:    
            if (right_val == 0) {
                *success = false;
                return 0;
            }
            return left_val / right_val;
        case TK_EQ:     return left_val == right_val;
        case TK_NEQ:    return left_val != right_val;
        case TK_GT:     return left_val > right_val;
        case TK_LT:     return left_val < right_val;
        case TK_GE:     return left_val >= right_val;
        case TK_LE:     return left_val <= right_val;
        case TK_AND:    return left_val && right_val;
        case TK_OR:     return left_val || right_val;
        default:
            *success = false;
            return 0;
    }
}
static bool make_token(char *e) {
    int position = 0;
    nr_token = 0;
    while (e[position] != '\0') {
        bool matched = false;
        regmatch_t pmatch;
        for (int i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                int len = pmatch.rm_eo;
                if (rules[i].token_type == TK_NOTYPE) {
                    position += len;
                    matched = true;
                    break;
                }
                if (nr_token >= 10000) {
                    fprintf(stderr, "错误: 超出最大token数量限制\n");
                    return false;
                }
                int copy_len = len < 31 ? len : 31;
                strncpy(tokens[nr_token].str, e + position, copy_len);
                tokens[nr_token].str[copy_len] = '\0';
                tokens[nr_token].type = rules[i].token_type;
                nr_token++;
                position += len;
                matched = true;
                break;
            }
        }
        if (!matched) {
            fprintf(stderr, "无法识别字符: %s\n", e + position);
            return false;
        }
    }
    return true;
}
word_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }
    recognize_deref();
    recognize_minus();
    *success = true;
    word_t result = eval(0, nr_token - 1, success);
    return *success ? result : 0;
}

