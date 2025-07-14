GDB:调试bug
**根本功能**是检查状态机的状态和控制状态机的迁移。
n单步执行程序，但遇到函数调用时，不会进入程序内部
s单步执行程序，遇到函数调用时，会进入程序内部
gcc -g main.c -o a.out
gdb a.out
break 10
break main
run
print x
continue
quit
layout src可视化代码调试布局
   dd   finish 执行完当前函数后暂停





可观测的错误
error
bug
运行产生的错误状态
需求-设计-代码（状态机）-Fault(bug)-Error（程序状态错）-Faiure
不匹配
二分查找

实际中的调试：**观察状态机执行（trace）的某个侧面**
**printf**->自定义log的trace
无法精准定位，大量的logs管理起来比较麻烦

**gdb**->指令/语句级定位、任意查看程序内部状态
耗费大量时间

日志：
管道：
 

