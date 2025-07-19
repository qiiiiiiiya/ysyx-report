TRM 通常指 Truth Table Minimizer（真值表最小化器），是一种用于简化逻辑函数的工具或算法。
每一条指令具体行为的描述
指令opcode的编码表格
**立即数**
**inc **指令的基本功能是：将指定的寄存器或内存地址中的值加 1，并更新标志位（如零标志 ZF、溢出标志 OF 等）。
拼接宏：是一种特殊的预处理宏，通过 ## 操作符将两个独立的标识符连接成一个新的标识符
**学习符号扩展与位操作在计算机层面有什么意义呢**

BITMASK(bits):生成位掩码
#define BITMASK(bits) ((1ull << (bits)) -1 )
生成一个低bits位全为1、其余位全为0的64位无符号整数掩码
BITS(x,hi,lo):提取指定位域
#define BITS(x,hi,lo) (((x) >> (lo)) & BITMASK((hi)-(lo)+1))
从整数x中提取从位lo(最低有效位）到位hi（最高有效位）的位域，并将其右对齐到最低位
SEXT（x,len）:符号扩展 
#define SEXT(x,len) ({struct {int64_t n:len; } __x={ .n=x }; (uint64_t)__x.n; })
作用：将len位的有符号整数x扩展为64位有符号整数，保持数值不变


















