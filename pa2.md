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

编译器负责在**延迟槽**中放置一条有意义的指令：
  使得无论是否跳转，按照这一约定的执行顺序都能正确的执行程序
                                 (•́へ•́╬)   **  RISV-V**
**立即数**均进行**符号扩展**
  .可以直接表示负数
  .可以用addi来实现subi

通过尽量减少每个比特的来源位置情况，来减少选择器的输入端，从而节省实现成本 
在所有指令格式中，rd,rs1,rs2总是在相同的位置：实现时可减少不必要的选择器

12+20的考量

访存指令：
没有专门的push，pop指令，可通过以sp作为基址寄存器来实现
sp(stack point)栈指针寄存器

不对齐访存的处理
：让执行环境来决定是否支持不对齐访问
  若执行环境支持
  ：纯硬件实现：与x86类似
    纯软件实现：抛异常，软件用若干条lb的组合来实现

                                                        **程序的机器级表示**
                                                       **1.常数，变量和运算**
如何用RISC-V指令集实现C语言中的各种功能
体会C语言是一门高级汇编语言

32位常数的装入

rv32gcc -O2 -c a.c
rvobjdump -M no-aliases -d a.o

lui和addi拼接，先跨一大步在跨一小步。

用访问数据的延迟换取更短的指令序列

乱序执行的高性能CPU中，取指令的代价大于取数据
  在流水线中，指令是数据的上游
  若从内存中取数据，只需让取数逻辑等待，可先执行其他无关指令
  若从内存取指令，则整条流水线都要等待
伪指令

变量的分配：
{v1,v2,...}->{r1,r2,...,M}
在实际的电路中
:R访问速度较快（当前周期可读出），但容量有限
M访问速度较慢（需要上百个周期读出），但容量几乎无限
  DDR颗粒中的存储单元由一个晶体管+一个电容组成
  电容需要充放电，延迟普遍比晶体大，故通常M的访问速度较慢
分配策略：常用快，不常用慢
编译困难，难以分析预测哪个常用
所以所有变量先分配在M,需要访问时读入R

                                                         **程序的内存布局：**
与变量有关的三个内存区域：静态数据区，堆区，栈区
  静态=不动态增长和变化，编译时确定
四种需要分配的C变量 data区
全局变量      data区
静态局部变量   stack区
动态变量     heap区

变量分配不对齐会降低访问效率
  在x86上，当不对齐的数据跨越64字节边界时，可观察到约2x的性能下降

                                                         ** 运算和指令**
 add sub mul div rem
 mv,访存指令
 and or xor
 xori r,r, -1
 sll srl sra
 sltiu r,r ,1
 slt
编译优化-尽可能在寄存器中进行运算
有符号数和无符号数：
硬件层面，有符号加法和无符号加法的行为完全一致，可以使用同一个加法器模块进行计算。
                                                   **2.条件分支和循环**
if-else   
可通过一条无符号比较指令实现有符号数的区间检查
switch-case  
整数加法溢出：undefine behavior 
C标准中规定，移位结果超出表示范围是UB，编译器可以任意处理
gcc -fsanitize=undefined a.c && ./a.out
一种共同的约定，来规范函数调用在机器级表示的实现细节。这就是**调用约定**，通常是ABI的一部分
**需要约定如下内容：**
  参数和返回值如何传递
  控制权如何转移
  f和g都要使用寄存器，如何协商
1.参数和返回值 
栈





                                                     **abstract machine裸机运行**
                                                       **1.计算机的迭代发展** 

                                                  **2.从abstract machine理解计算机**
计算机系统的两个诀窍：
1.抽象     引入一个新的抽象层，把这些机制的功能抽象成API
2.模块化   把这些机制分模块，并按需组合
提供最少的API来实现现代计算机系统软件

                                                          **3.AM代码导读**
                                                            **4.AM的八卦**

                                                     
                                                     
                                                            **工具与基础设施**


                                                            **单周期处理器设计**
状态-时序逻辑电路
 PC-单个寄存器，初值为0
 R-通用寄存器组，可寻址，用带使能的D触发器搭建寄存器
 M-内存，可寻址，容量更大的存储器
状态转移-组合逻辑电路
  数据通路-数据移动和计算所经过的路径及相关部件
 
  控制信号-控制数据在数据通路中如何移动
整个inst_cycle()可以看作是数据通路
其中的if，switch-case充当了控制信号的作用

设计数据通路和控制信号
1.分析功能需求
2.确定相应的部件，连接成完整的数据通路
3.确定控制信号
4.汇总控制信号，每种状态下的所有控制信号的取值
5.状态到控制信号的逻辑表达式，设计控制信号
功能需求=指令行为

微结构设计

指令和功能单元
编写可维护的代码




                                                                    **ELF文件和链接**
                                                                  **1.ELF目标文件格式**
调试需要调试信息
  是否打开Enable debug information 会影响NEMU的大小
  addr2line根据调试信息将地址转换为源文件位置（gdb）
权限管理
  代码可读可执行，但不能写
  可读可写，但不能执行
  需要知道代码和数据之间的边界

设计文件格式
需求：代码和数据分离
  记录它们在文件中的位置
  记录它们的长度
  记录它们的其他属性
  记录他们的名称
数据结构
typedef struct{
off_t offset;
size_t len;
uint32_t attr1,attr2;
char name[32];
}SectionHeader;
这个数据结构记录的程序对象称为“节”
这个数据结构（元数据）称为“节头”
ELF中常见的节
.text 代码
.data 可写数据 已初始化的全局变量和局部静态变量
.rodata 只读数据 
.bss 未初始化数据和局部静态变量
.debug 调试信息
.line 行号信息

                                              **如果节的名称太长**
把元数据的字符串集中放在另一个区域
  节头中的name换成一个"指针"
    记录 字符串在文件中的位置
或者把这个区域当做一个新的节，节头中的name换成一个偏移量
  记录字符串在新节中的位置
  ELF格式采用此方案，这个专门的节叫.strtab(string table)
  
                                       **可以通过节头找到一个节，那该如何找到节头**
  约定一个位置
  方案1：直接约定将节头放在距离文件开始x字节的位置
      不太灵活，其他文件内容要绕开
      如果还有其他内容也需要这么找，就要约定x,y,z
  方案2：节头的位置无需固定，但将其位置信息记录在一个约定的位置
    ELF格式采用此方案，将节头的位置记录在ELF头中，并约定ELF头位于ELF文件的起始位置
typedef struct{
  off_t sh_offset; //section header offset  
  }ELFHeader

                                            **找到一个节头后，如何找到下一个**
 需要考虑如何组织多个节头
 方案1：链表
 编译后一般无需动态增删
 方案2：数组，多个节头连续存放，形成一张"节头表"
ELF格式采用此方案，并将节头的数量记录在ELF头
typedef struct{
 offset_t sh_offset;//section header table offset
 uint16_t sh_num;//number of section header
}ELFHeader

readelf -S a.out #查看ELF文件中的节

                                                     **静态链接**
预处理编译汇编链接执行
**汇编**-根据指令集手册，把汇编代码（指令的符号化表示）翻译成二级制目标文件（指令的编码表示）
  这一步得到目标文件
**链接**-合并多个目标文件，生成可执行文件
编译/汇编都无法处理跨节的函数和数据引用
extern void f();
rv32gcc -fno-pic -c -O2 main.c
rv32gcc -fno-pic -c -O2 f.c
riscv64-unknown-elf-gcc -march=rv32i -mabi=ilp32 -fno-pic -c -O2 extern.c
# -march=rv32i 指定 32位 RISC-V 架构，-mabi=ilp32 指定 32位 ABI 
编译和汇编均以文件为单位进行，标记*的指令/数据无法得知最终位置



rvobjudmp -d main.o
riscv64-unknown-elf-objdump -m riscv:rv32 -d extern.o
# -m riscv:rv32 强制指定为 32位 RISC-V 架构

坑1-库的顺序
坑2-name mangling  
gcc -c a.c && g++ -c b.cpp && gcc a.o b.o
坑3-符号解析时无法检查变量类型
gcc main.c f.c g.c && ./a.out
坑4-C语言的试探性定义
gcc mainn.c ff.c
gcc mainn.c ff.c -fcommon && ./a.out





养成良好的编程习惯，尽量不要使用全局变量
若要使用，尽量加static修饰（局部符号不参与符号解析）
若要使用全局符号
  借助名字空间管理全局变量
    C语言不支持，但可以手动给变量添加前缀（AM的框架代码）
  不要使用试探性定义
    定义变量时初始化（符号已解析）
    外部变量使用extern修饰
#查看gcc的默认链接脚本
riscv64-linux-gnu-gcc -Wl,--verbose a.c
riscv64-unknown-elf-gcc -Wl,--verbose a.c

readelf -r hello.o
riscv64-unknown-elf-objdump -m riscv:rv32 -Dr hello.o

**code model**
-mcmodel=medlow
           Generate code for the medium-low code model. The program and its statically defined symbols must lie within a  single  2  GiB  address
           range and must lie between absolute addresses -2 GiB and +2 GiB. Programs can be statically or dynamically linked. This is the default
           code model.

       -mcmodel=medany
           Generate  code  for  the medium-any code model. The program and its statically defined symbols must be within any single 2 GiB address
           range. Programs can be statically or dynamically linked.

           The code generated by the medium-any code model is position-independent, but is not guaranteed to function correctly when linked  into
           position-independent executables or libraries.

符号表-s
字符串表readelf -S |grep "strtab"



















链接的本质工作：
1.符号解析-处理符号的引用 
  将符号的引用与符号的定义建立关联
2.重定位-合并相同的节
  确定每一个符号的最终地址，并填写到引用处

汇编语言
  用助记符表示操作码 add 0101
  用符号表示位置     L0 0101
  用助记符表示寄存器  
  最终进行汇编、链接
更高级编程语言出现
  符号定义
  符号的引用
链接的步骤
1）确定符号引用关系（符号解析）
        2）合并相关.o文件
重定位   3）确定每个符号的地址        
        4）在指令中填入新地址



链接：先确定L0的位置，再在jump指令中填入L0的位置

一个可执行目标文件是由多个可重定位目标文件链接合并后生成的
局部变量分配在栈中，不会在过程（函数）外被引用，因此不是符号定义

三类目标文件：
可重定位目标文件.o
可执行目标文件a.out
共享的目标文件*.so

节是ELF文件中具有相同特征的最小可处理单元
由不同的段组成，描述节如何映射到存储段中，可多个节映射到同一段。
  如：可合并.data节和.bss节，并映射到一个可读可写数据段中
elf格式强调目标链接的两种视图：
  链接视图
  执行视图 

  readelf -h main.o
  

 readelf

                                                             ** 基础快递员**
printf     把包裹送到大门口显示屏（标准输出，通常是屏幕）
    printf("hello %d","world");
sprintf     把包裹存到仓库的纸箱（字符数组）里
    sprintf(buf,"ID:%d", 123);
    可能缓冲区溢出
fprintf    把包裹送到指定地址（任意文件）
    fprintf(file,"Log:%s","error");
    fprintf(stderr,"Error!"); //发送到错误信息区
                                                            **安全快递员（带保护）**
snprintf  **安全版** sprintf,带纸箱尺寸测量仪
  snprintf(buf,sizeof(buf), "%s",big_str);->自动截断超长内容
  防止缓冲区溢出

dprintf    直接送到门牌号地址（文件扫描符）
    dprintf(3, "Data:%d",42);->写入文件描述符3对应的文件
    适用场景：底层系统编程
                                                             **组装快递员(高级用法)**
vprintf    把已经打包过的包裹（va_list）送到大门口显示屏
  void my_printf(const char *fmt, ...){
    va_list args;
    va_start(args,fmt);
    vprintf(fmt,args);
    va_end(args);
    }
  使用场景：当你想自定义一个打印函数，但最终输出到屏幕时
vfprintf   自定义日志系统
vsprintf  
vsnprintf  安全的将打包好的参数存到仓库纸箱（带尺寸保护）是唯一安全的字符串格式化方式
vdprintf
