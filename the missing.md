<img width="192" height="192" alt="logo-O35E636P" src="https://github.com/user-attachments/assets/d4ffc5db-b02e-4ae8-9b92-2d15c81e777c" />ls
所有文件（包括隐藏文件）：-a
文件打印以人类可以理解的格式输出 (例如，使用 454M 而不是 454279954): -h
文件以最近访问顺序排序：-t
以彩色文本显示输出结果 --color=auto

切换目录：
1.**z 工具（最常用）**
z 会自动记录你访问过的目录，根据访问频率排序，输入目录名的部分关键词即可跳转（无需完整路径）。
安装（以 Ubuntu 为例）：
bash：
sudo apt install zsh  # z依赖zsh，先安装zsh
sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"  # 安装oh-my-zsh（可选，简化配置）
git clone https://github.com/agkozak/zsh-z $ZSH_CUSTOM/plugins/zsh-z  # 安装z插件
然后在 ~/.zshrc 中启用插件：plugins=(... zsh-z)，执行 source ~/.zshrc 生效。
使用：
先正常访问几次目录（让z记录）：cd /home/user/work/report
之后直接输入 z report 即可跳转（输入部分关键词即可，无需完整路径）
2.cd -感觉和上键效果是一样的
3.alias proj=‘cd /home/usr’临时定义别名
4.永久别名：编辑 ~/.bashrc 或 ~/.bash_profile.


首先创建所需的文件
  mkdir html_root
  cd html_root
  touch {1..10}.html
  mkdir html
  cd html
  touch xxxx.html
  ├── html_root
  │   ├── 1.html
  │   ├── 10.html
  │   ├── 2.html
  │   ├── 3.html
  │   ├── 4.html
  │   ├── 5.html
  │   ├── 6.html
  │   ├── 7.html
  │   ├── 8.html
  │   ├── 9.html
  │   └── html
  │       └── xxxx.html
执行 find 命令
  # for Linux
  find . -type f -name "*.html" | xargs -d '\n'  tar -cvzf html.zip
  a html_root/9.html
  a html_root/5.html
  a html_root/4.html
  a html_root/8.html
  a html_root/3.html
  a html_root/html/xxxx.html
  a html_root/2.html
  a html_root/1.html
  a html_root/10.html
  a html_root/7.html
  a html_root/6.html

  find . -name "*.html" 从当前目录开始查找所有以.html结尾的文件
  xargs的核心作用是“读取标准输入的内容，将其作为参数后传递给后续命令”
  zip 用于创建 ZIP 格式的压缩文件，基本用法：zip [压缩包名] [要压缩的文件列表]
默认情况下，xargs 会把 “空格” 当作参数的分隔符，会将 test file.html 误判为两个文件（test 和 file.html），导致压缩失败。
  find 和 xargs 提供了配套参数处理这种情况：
find 的 -print0：输出文件路径时，用 “null 字符（\0）” 作为分隔符（而非默认的换行）；
xargs 的 -0：以 “null 字符（\0）” 作为输入的分隔符（识别 find -print0 的输出）。
**find . -name "*.html" -print0 | xargs -0 zip htmls.zip**



按照最近的使用时间列出文件 find . -type f -print0 | xargs -0 ls -lt | head -1
当文件数量较多时，增加 -mmin 条件，先将最近修改的文件进行初步筛选再交给 ls 进行排序显示 find . -type f -mmin -60 -print0 | xargs -0 ls -lt | head -10

linux模式：
正常模式：在文件中四处移动光标进行修改
插入模式：插入文本
替换模式：替换文本
可视化模式（一般，行，块）：选中文本块
命令模式：用于执行命令
（不出意外我只用过前两个）
在不同的操作模式下，键盘敲击的含义也不同。比如，x 在**插入模式**会**插入字母 x**，但是在**正常模式** 会**删除当前光标所在的字母**，在**可视模式**下则会**删除选中文块**。
在正常模式，键入 i 进入插入 模式，R 进入替换模式，v 进入可视（一般）模式，V 进入可视（行）模式，<C-v> （Ctrl-V, 有时也写作 ^V）进入可视（块）模式，: 进入命令模式。
**命令行**：
:e {文件名} 打开要编辑的文件
:ls 显示打开的缓存
:help {标题} 打开帮助文档
:help :w 打开 :w 命令的帮助文档
:help w 打开 w 移动的帮助文档
**移动**：
词： w （下一个词）， b （词初）， e （词尾）
行： 0 （行初）， ^ （第一个非空格字符）， $ （行尾）
屏幕： H （屏幕首行）， M （屏幕中间）， L （屏幕底部）
翻页： Ctrl-u （上翻）， Ctrl-d （下翻）
文件： gg （文件头）， G （文件尾）
行数： :{行数}<CR> 或者 {行数}G ({行数}为行数)
杂项： % （找到配对，比如括号或者 /* */ 之类的注释对）
查找： f{字符}， t{字符}， F{字符}， T{字符}
查找/到 向前/向后 在本行的{字符}
, / ; 用于导航匹配
搜索: /{正则表达式}, n / N 用于导航匹配
**编辑：**
O / o 在之上/之下插入行
d{移动命令} 删除 {移动命令}
例如，dw 删除词, d$ 删除到行尾, d0 删除到行头。
c{移动命令} 改变 {移动命令}
例如，cw 改变词
比如 d{移动命令} 再 i
x 删除字符（等同于 dl）
s 替换字符（等同于 xi）
可视化模式 + 操作
选中文字, d 删除 或者 c 改变
u 撤销, <C-r> 重做
y 复制 / “yank” （其他一些命令比如 d 也会复制）
p 粘贴
更多值得学习的: 比如 ~ 改变字符的大小写


**正则表达式**
. 除换行符之外的 “任意单个字符”
* 匹配前面字符零次或多次
+ 匹配前面字符一次或多次
[abc] 匹配 a, b 和 c 中的任意一个
(RX1|RX2) 任何能够匹配 RX1 或 RX2 的结果
^ 行首
$ 行尾
