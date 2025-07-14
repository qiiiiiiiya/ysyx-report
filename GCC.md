一.基础编译与输出控制
gcc hello.c -o hello//指定输出文件名
gcc -c hello1.c hello2.c//仅编译（生成目标文件.o）不进行链接
gcc -S hello.c //生成hello.s汇编文件
gcc -E hello.c -o hello.i //仅执行预处理阶段，生成预处理后的hello.i文件
gcc -v hello.c -o hello //查看完整编译流程

二.警告与错误检查
gcc -Wall test.c -o test//编译时显示常见警告
gcc -Wall -Werror test.c -o test //警告即报错
三.优化选项

-00
-01/-0
-02
-03
四.调试与符号信息


gcc -g hello.c -o hello//生成带调试信息的可执行文件，支持GDB调试 
-ggdb //优化GDB调试体验
-g3

五.链接选项（处理库依赖）
-l<lib> 
-L<dir>

argc(argument count)参数数量：整数，代表程序运行时传入的参数总数量（包含程序名本身）
argv(argument vector)：字符串数组，存储具体的参数内容，argv[0]是程序名，argv[1]是第一个参数。
#include<stdio.h>
int main(int argc,char *argv[]){
    //如果没有传入额外参数（仅程序名本身）
    if(argc==1){
        printf("Hello!请传入参数\n");
    }else{
        printf("参数总数：%d\n",argc);
        for(int i=0;i<argc;i++){
            printf("参数%d:%s\n",i,argv[i]);
        }
    }
    return 0;
}






