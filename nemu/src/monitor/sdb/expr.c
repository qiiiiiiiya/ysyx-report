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
    //type表示token的类型
    int type;
    //str表示token的字符串内容
    char str[32];
} Token;

// tokens数组用于存储识别出的所有token
static Token tokens[10000] = {};
//nr_token表示已经被识别出的token数目
static int nr_token = 0;

// Token类型定义
enum {
    TK_NOTYPE = 0,
    TK_PLUS, TK_MINUS, TK_MUL, TK_DIV,
    TK_EQ, TK_NEQ, TK_GT, TK_LT, TK_GE, TK_LE,
    TK_AND, TK_OR, TK_NUM, TK_LPAREN, TK_RPAREN,
    TK_HEX, TK_REG, TK_DEREF, TK_MINUS_F,TK_PC
};

// Token类型的字符串表示
static struct rule {
    // regex表示正则表达式
    const char *regex;
    // token_type表示对应的token类型
    int token_type;
} 
rules[] = {
    //用//是因为有些有其它含义，，在这里只用它的字面意思
    {"0x[0-9a-fA-F]+", TK_HEX},          // 十六进制数
    {"\\$pc",TK_PC},                   // PC寄存器
    {"\\$[a-z][a-z0-9]{0,2}", TK_REG},  // 寄存器匹配规则（带或不带$）
    {"\\$\\$[0-9]", TK_REG},        
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

// 计算规则的数量
#define NR_REGEX ARRLEN(rules)

// 正则表达式数组，用于存储编译后的正则表达式
static regex_t re[NR_REGEX] = {};

// 初始化正则表达式
void init_regex() {
    char error_msg[128];
    for (int i = 0; i < NR_REGEX; i++) {
        //regcomp() is used to compile a regular expression into a form that is suitable for
        // subsequent regexec() searches.
        int ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
        //regerror() is used to turn the error codes that can be returned by both regcomp() 
        //and regexec() into error  message strings.
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}
//解引用和乘号区分
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
                tokens[i-1].type == TK_OR ||
                tokens[i-1].type == TK_GT ||
                tokens[i-1].type == TK_LT ||
                tokens[i-1].type == TK_GE ||
                tokens[i-1].type == TK_LE ||
                tokens[i-1].type == TK_DEREF ||
                tokens[i-1].type == TK_MINUS_F) {
                tokens[i].type = TK_DEREF;
            }
        }
    }
}
//负号和减号区分
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
                tokens[i-1].type == TK_OR ||
                tokens[i-1].type == TK_GT ||
                tokens[i-1].type == TK_LT ||
                tokens[i-1].type == TK_GE ||
                tokens[i-1].type == TK_LE ||
                tokens[i-1].type == TK_DEREF) {
                tokens[i].type = TK_MINUS_F;
            }
        }
    }
}

// 检查括号是否匹配
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
// 查找二元运算符的位置,主运算符
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
    };
    // 遍历tokens数组，寻找二元运算符
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LPAREN) bracket_count++;
        else if (tokens[i].type == TK_RPAREN) bracket_count--;
        else if (bracket_count == 0) {
            // 如果是二元运算符，检查其优先级
            if (tokens[i].type == TK_OR || tokens[i].type == TK_AND ||
                tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ ||
                tokens[i].type == TK_LT || tokens[i].type == TK_LE ||
                tokens[i].type == TK_GT || tokens[i].type == TK_GE ||
                tokens[i].type == TK_PLUS || tokens[i].type == TK_MINUS ||
                tokens[i].type == TK_MUL || tokens[i].type == TK_DIV) {
                
                int op_priority = priority[tokens[i].type];
                // 优先级相同则选择右侧运算符
                if (op_priority <= min_priority) {
                    min_priority = op_priority;
                    op_pos = i;
                }
            }
        }
    }
    
    return op_pos;
}
// 递归计算表达式的值
// p和q分别表示tokens数组的起始和结束索引
// success用于指示计算是否成功
static int64_t eval(int p, int q, bool *success) {
    if (p > q) {
        *success = false;
        return 0;
    }
    
    // 如果是括号内的表达式，递归计算
    if (check_parentheses(p, q)) {
        return eval(p + 1, q - 1, success);
    }
    
    // 如果只有一个token，直接返回其值
    if (p == q) {
        switch (tokens[p].type) {
            case TK_NUM: return strtol(tokens[p].str, NULL, 10);
            case TK_HEX: return strtol(tokens[p].str, NULL, 16);
            case TK_REG: {
                bool reg_success;
                word_t val = isa_reg_str2val(tokens[p].str, &reg_success);
                if (!reg_success) { *success = false; return 0; }
                return (word_t) val;
            }
            case TK_PC: {
                // 处理$pc寄存器
                return (word_t) cpu.pc;
            }
            default: *success = false; return 0;
        }
    }
    
    int op_pos = find_operator(p, q);
    if (op_pos != -1) {
        // 如果找到二元运算符，递归计算左右两侧的值
        int64_t left_val = eval(p, op_pos - 1, success);
        if (!*success) return 0;
        
        int64_t right_val = eval(op_pos + 1, q, success);
        if (!*success) return 0;
        // 根据运算符类型返回计算结果
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
    
    // 如果是一元运算符（负号或解引用），递归计算后续表达式
    if (tokens[p].type == TK_MINUS_F || tokens[p].type == TK_DEREF) {
        if (p + 1 > q) {
            *success = false;
            return 0;
        }
        
        // 递归计算后续表达式
        int64_t val = eval(p + 1, q, success);
        if (!*success) return 0;
        
        switch (tokens[p].type) {
            case TK_MINUS_F: return -val;  
            case TK_DEREF: return (word_t) vaddr_read((vaddr_t)val, 4);
            default: *success = false; return 0;
        }
    }
    
    // 如果没有找到合适的运算符，返回错误
    *success = false;
    return 0;
}

// e是指向输入的表达式的指针
//词法分析，识别表达式中的各种token,复制到tokens数组中
static bool make_token(char *e) {
    int position = 0;
    nr_token = 0;
    while (e[position] != '\0') {
        bool matched = false;
        //regmatch_t是正则表达式匹配结果的结构体类型，用于存储正则匹配的位置信息
        regmatch_t pmatch;
        for (int i = 0; i < NR_REGEX; i++) {
        //regexec() is used to match a null-terminated string against the compiled pattern buffer 
        //in *preg, which  must  have been initialised with regexec(). 
        //rm_so匹配开始的位置，eo结束的位置
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
                //严格限制复制长度，防止溢出
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
// expr函数的入口点
// 该函数用于计算表达式的值
word_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }
    // 识别解引用和负号
    recognize_deref();
    recognize_minus();

    *success = true;
    
    word_t result = eval(0, nr_token - 1, success);
    return (word_t)result;
}
