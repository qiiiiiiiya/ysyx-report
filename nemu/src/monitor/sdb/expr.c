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
#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
typedef struct token {
    int type;
    char str[32];
 } Token;
extern const char *regs[];
enum {
  TK_NOTYPE = 0,
  TK_PLUS,TK_MINUS,TK_MUL,TK_DIV,//+-*/
  TK_EQ,TK_NEQ,TK_GT,TK_LT,TK_GE,TK_LE,//==,!=,>,<,>=,<=
  TK_AND,TK_OR,TK_NUM,TK_LPAREN,TK_RPAREN,//&&,||,十进制整数,(,)
  TK_HEX,TK_REG,TK_DEREF,TK_MINUS_F//十六进制、寄存器、指针解引用、负数
/*TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
    {"0x[0-9a-fA-F]+",TK_HEX},  // 以"0x"开头
    {"\\&?[a-z][a-z0-9]{0,3}",TK_REG}, // 寄存器
/*    {"\\-",TK_MINUS_F},//负数*/
    {" +",TK_NOTYPE},
    {"\\(",TK_LPAREN},
    {"\\)",TK_RPAREN},
    {"\\+",TK_PLUS},
    {"\\-",TK_MINUS},//减
    {"\\*",TK_MUL},
    {"/",TK_DIV},
    {"==",TK_EQ},
    {"!=",TK_NEQ},
    {">=",TK_GE},
    {"<=",TK_LE},
    {">",TK_GT},
    {"<",TK_LT},
    {"&&",TK_AND},
    {"\\|\\|",TK_OR},
    {"[0-9]+",TK_NUM},//十进制
/*    {"\\*",TK_DEREF},// 指针解引用
*/



};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};
static Token tokens[32] __attribute__((used)) ={};
static int nr_token  __attribute__((used))  = 0;
/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

static bool check_parentheses(int p,int q){
    if(tokens[p].type!=TK_LPAREN ||tokens[q].type!=TK_RPAREN)
        return false;
    int count=0;
    for(int i=p;i<=q;i++){
    if(tokens[i].type==TK_LPAREN) count++;
    else if(tokens[i].type==TK_RPAREN){
        count--;
        if(count<0) return false;
        }
        if(count==0 && i!=q) return false;
    }
    return count==0;
}

static int find(int p,int q){
    int priority=-1;
    int pos=-1;
    int bracket_count=0;

    static const int OP_PRIORITY[256]={
        [TK_OR]=7,
        [TK_AND]=6,
        [TK_EQ]=5,[TK_NEQ]=5,
        [TK_LT]=4,[TK_LE]=4,[TK_GT]=4,[TK_GE]=4,
        [TK_PLUS]=3,[TK_MINUS]=3,
        [TK_MUL]=2,[TK_DIV]=2,
        [TK_DEREF]=1,[TK_MINUS_F]=1,
    };
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == TK_LPAREN)
            bracket_count++;
        else if (tokens[i].type == TK_RPAREN)
            bracket_count--;
        if (bracket_count == 0 && OP_PRIORITY[tokens[i].type] > priority) {
            priority = OP_PRIORITY[tokens[i].type];
            pos = i;
        }
    }
    return pos;
}
    static void recognize_deref(){
        for(int i=0;i<nr_token;i++){
            if(tokens[i].type==TK_MUL){
                if(i==0||
                    tokens[i-1].type==TK_PLUS||
                    tokens[i-1].type==TK_MINUS||
                    tokens[i-1].type==TK_MUL||
                    tokens[i-1].type==TK_DIV||
                    tokens[i-1].type==TK_LPAREN||
                    tokens[i-1].type==TK_EQ||
                    tokens[i-1].type==TK_NEQ||
                    tokens[i-1].type==TK_AND||
                    tokens[i-1].type==TK_OR){
                    tokens[i].type=TK_DEREF;
                }
            }
        }
    }
    /*标记*/
    static void recognize_minus(){
        for(int i=0;i<nr_token;i++){
            if(tokens[i].type==TK_MINUS){
                if(tokens[i-1].type!=TK_NUM)
                {
                    tokens[i].type=TK_MINUS_F;}
            }
        }
    }
/*标记*/


static bool make_token(char *e){
        int position = 0;
        regmatch_t pmatch;
        nr_token = 0;
        while (e[position] != '\0') {
            int matched=0;
            for (int i = 0; i < NR_REGEX; i ++) {
                if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                    char *substr_start = e + position;
                    int substr_len = pmatch.rm_eo;

                    //跳过空格
                    if(rules[i].token_type==TK_NOTYPE){
                        position+=substr_len;
                        matched=1;
                        break;
                    }

                    if(nr_token>=32){
                        fprintf(stderr,"error,超出最大数量限制");
                        return false;
                    }
                    int copy_len =substr_len < 31 ? substr_len : 31;
                    strncpy(tokens[nr_token].str, substr_start, copy_len);
                    tokens[nr_token].str[copy_len] = '\0';
                    tokens[nr_token].type = rules[i].token_type;
                    // 特殊处理寄存器符号
                    if (rules[i].token_type == TK_REG) {
                        tokens[nr_token].type = TK_REG;
                    } nr_token++;
                position += substr_len;
                printf("match rules[%d] = \"%s\" at position %d with len %d: %.*s\n",i, rules[i].regex, position - substr_len, substr_len, substr_len, substr_start);
                matched = 1;
                break;
            }
        }
        if(!matched){
            fprintf(stderr,"无法识别字符%s",e+position);
            return false;
        }
    }
    return true;
}
/*标记*/
// 计算寄存器数量（数组长度）
static int reg_name2idx(const char* reg_name) {
const int regs_num = 32;
const char* name = reg_name;
for(int i=0;i<regs_num;i++){
    const char* reg=regs[i];
    if(strcmp(name,reg)==0){
        return i;
    }
    }return -1;
}
/*标记*/
/*
// 寄存器名转索引（返回-1表示无效）
static int reg_name2idx(const char* reg_name) {
    // 处理输入：去掉开头的'$'（统一格式）
    const char* name = reg_name;
    if (name[0] == '$') {
        name++; // 跳过'$'，如"$ra"→"ra"，"$0"→"0"
    }

    // 遍历regs数组，查找匹配项
    for (int i = 0; i < regs_num; i++) {
        // 处理数组元素：去掉开头的'$'（如"$0"→"0"）
        const char* reg = regs[i];
        const char* reg_clean = (reg[0] == '$') ? (reg + 1) : reg;

        // 比较处理后的字符串
        if (strcmp(name, reg_clean) == 0) {
            return i; // 找到，返回索引（数组下标）
        }
    }

    // 未找到
    return -1;
}*/

static long eval(int p, int q,bool *success) {
    if (p > q){
        *success=false;
        return 0;
    }
    if (p == q) {  // 单个token处理（数字、寄存器）
        switch (tokens[p].type) {
            case TK_NUM:  // 十进制数
                return strtol(tokens[p].str, NULL, 10);
            case TK_HEX:  // 十六进制数（0x开头）
                return strtol(tokens[p].str, NULL, 16);
            case TK_REG: {
                const char* reg_name = tokens[p].str; // 输入的寄存器名（如"$ra"或"ra"）
                int idx = reg_name2idx(reg_name);     // 转换为索引
                if (idx == -1) { // 无效寄存器
                *success = false;
                return 0;
            }
                return cpu.gpr[idx];
            }
            case TK_MINUS_F:{
                return -TK_MINUS_F;
            }
            default:
                *success = false;
                return 0;
        }
    }

    if (check_parentheses(p, q)) {  // 处理括号
        return eval(p + 1, q - 1,success);
    }
    int op_pos = find(p, q);
    if(op_pos==-1){
        *success=false;
        return 0;
    }
recognize_minus();
    // 找最高优先级运算符
    word_t left = eval(p, op_pos - 1,success);
    if(!*success) return 0;
    word_t right = eval(op_pos + 1, q,success);
    if(!*success) return 0;
    // 计算运算符
    switch (tokens[op_pos].type) {
        
        case TK_REG: {
                // 使用 isa_reg_str2val API 获取寄存器值
                bool reg_success;
                word_t reg_val = isa_reg_str2val(tokens[p].str, &reg_success);
                if (!reg_success) {
                    *success = false;
                    return 0;
                }
                return reg_val;
            }
        case TK_PLUS:
            return left + right;
        case TK_MINUS:
            return left - right;
        case TK_MUL:
            return left * right;
        case TK_DIV:
        if(right==0){
            *success=false;
            return 0;
        }
            return left / right;
        case TK_EQ:
            return left==right;
        case TK_NEQ:
            return left!=right;
        case TK_GT:
            return left>right;
        case TK_LT:
            return left<right;
        case TK_GE:
            return left>=right;
        case TK_LE:
            return left<=right;
        case TK_AND:
            return left&&right;
        case TK_OR:
            return left||right;
        // 其他运算符（如逻辑、比较）需额外处理
        default:
            *success=false;
            return 0;
    }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  recognize_deref();
  bool eval_success=true;
  word_t result=eval(0,nr_token-1,&eval_success);
  *success=eval_success;
  return result;
  /* TODO: Insert codes to evaluate the expression. */
  return 0;
}