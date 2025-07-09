/**************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS `PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>

extern const char *regs[];

//为各种token类型添加规则，使用了一些元字符
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
    // 二元运算符优先级定义
    const int priority[] = {
        [TK_OR]     = 1,
        [TK_AND]    = 2,
        [TK_EQ]     = 3, [TK_NEQ]   = 3,
        [TK_LT]     = 4, [TK_LE]    = 4, [TK_GT] = 4, [TK_GE] = 4,
        [TK_PLUS]   = 5, [TK_MINUS] = 5,
        [TK_MUL]    = 6, [TK_DIV]   = 6,
        [TK_DEREF]  = 7, // 解引用操作符优先级最高
        [TK_MINUS_F] = 7 // 一元负号也视为最高优
    };
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LPAREN) bracket_count++;
        else if (tokens[i].type == TK_RPAREN) bracket_count--;
        else if (bracket_count == 0) {
            //匹配所有二元运算符
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
    // 处理括号：若整体被括号包裹且匹配，直接计算内部
    if (check_parentheses(p, q)) {
        return eval(p + 1, q - 1, success);
    }
    // 处理一元操作符 (负号和解引用)
    if (tokens[p].type == TK_MINUS_F || tokens[p].type == TK_DEREF) {
        // 检查是否有足够的操作数
        if (p + 1 > q) {
            *success = false;
            return 0;
        }
        word_t unary_result;
        int sub_expr_end; // 一元运算符作用的子表达式结束索引
        // 情况1：连续一元运算符（如 --1 或 *-1）
        if (tokens[p+1].type == TK_MINUS_F || tokens[p+1].type == TK_DEREF) {
            // 递归计算后续一元运算符组成的子表达式，其结束索引为q
            word_t val = eval(p + 1, q, success);
            if (!*success) return 0;
            sub_expr_end = q; // 子表达式覆盖到q
            switch (tokens[p].type) {
                case TK_MINUS_F: unary_result = -val; break;
                case TK_DEREF: unary_result = vaddr_read(val, 4); break;
                default: *success = false; return 0;
            }
        }
        // 情况2：作用于原子值（数字、寄存器、括号表达式）
        else {
            word_t val;
            switch (tokens[p+1].type) {
                case TK_NUM:  
                    val = strtol(tokens[p+1].str, NULL, 10); 
                    sub_expr_end = p + 1; // 子表达式结束于当前数字
                    break;
                case TK_HEX:  
                    val = strtol(tokens[p+1].str, NULL, 16); 
                    sub_expr_end = p + 1; // 子表达式结束于当前十六进制数
                    break;
                case TK_REG: {
                    bool reg_success;
                    val = isa_reg_str2val(tokens[p+1].str, &reg_success);
                    if (!reg_success) { *success = false; return 0; }
                    sub_expr_end = p + 1; // 子表达式结束于当前寄存器
                    break;
                }
                case TK_LPAREN: {
                    // 括号表达式需完整匹配，子表达式结束于右括号（q）
                    if (!check_parentheses(p+1, q)) { *success = false; return 0; }
                    val = eval(p+2, q-1, success); // 计算括号内的值
                    if (!*success) return 0;
                    sub_expr_end = q; // 子表达式结束于右括号
                    break;
                }
                default:
                    *success = false;
                    return 0;
            }
            // 应用当前一元运算符
            switch (tokens[p].type) {
                case TK_MINUS_F: unary_result = -val; break;
                case TK_DEREF: unary_result = vaddr_read(val, 4); break;
                default: *success = false; return 0;
            }
        }
       // 核心：若一元运算的子表达式未覆盖全部范围（还有后续运算符），则继续处理二元运算
        if (sub_expr_end < q) {
            int op_pos = find_operator(sub_expr_end + 1, q); // 查找后续二元运算符
            if (op_pos == -1) { *success = false; return 0; }
            // 计算右操作数（二元运算符右侧的子表达式）
            word_t right_val = eval(op_pos + 1, q, success);
            if (!*success) return 0;
            // 应用二元运算符，返回最终结果
            switch (tokens[op_pos].type) {
                case TK_PLUS: return unary_result + right_val;
                case TK_MINUS: return unary_result - right_val;
                case TK_MUL: return unary_result * right_val;
                case TK_DIV: 
                    if (right_val == 0) { *success = false; return 0; }
                    return unary_result / right_val;
                case TK_EQ: return unary_result == right_val;
                case TK_NEQ: return unary_result != right_val;
                case TK_GT: return unary_result > right_val;
                case TK_LT: return unary_result < right_val;
                case TK_GE: return unary_result >= right_val;
                case TK_LE: return unary_result <= right_val;
                case TK_AND: return unary_result && right_val;
                case TK_OR: return unary_result || right_val;
                default: *success = false; return 0;
            }
        }
        // 若子表达式已覆盖全部范围，直接返回一元运算结果
        else {
            return unary_result;
        }
    }
    
    // 处理单个原子值（非一元运算符开头的情况）
    if (p == q) {
        switch (tokens[p].type) {
            case TK_NUM: return strtol(tokens[p].str, NULL, 10);
            case TK_HEX: return strtol(tokens[p].str, NULL, 16);
            case TK_REG: {
                bool reg_success;
                word_t val = isa_reg_str2val(tokens[p].str, &reg_success);
                if (!reg_success) { *success = false; return 0; }
                return val;
            }
            default: *success = false; return 0;
        }
    }
    // 处理二元运算符（非一元开头的表达式）
    int op_pos = find_operator(p, q);
    if (op_pos == -1) { *success = false; return 0; }
    word_t left_val = eval(p, op_pos - 1, success);
    if (!*success) return 0;
    word_t right_val = eval(op_pos + 1, q, success);
    if (!*success) return 0;
    switch (tokens[op_pos].type) {
        case TK_PLUS: return left_val + right_val;
        case TK_MINUS: return left_val - right_val;
        case TK_MUL: return left_val * right_val;
        case TK_DIV: 
            if (right_val == 0) { *success = false; return 0; }
            return left_val / right_val;
        case TK_EQ: return left_val == right_val;
        case TK_NEQ: return left_val != right_val;
        case TK_GT: return left_val > right_val;
        case TK_LT: return left_val < right_val;
        case TK_GE: return left_val >= right_val;
        case TK_LE: return left_val <= right_val;
        case TK_AND: return left_val && right_val;
        case TK_OR: return left_val || right_val;
        default: *success = false; return 0;
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

