#include <stdio.h>
#include <stdlib.h>
#include <verilated.h>
#include <Vtop.h>  // Verilator生成的类
#include "svdpi.h"

// 全局变量
Vtop* top_ptr = nullptr;  // 全局指针，用于访问 top 模块
bool encountered_ebreak = false;

// DPI-C函数，用于处理ebreak指令
extern "C" void npc_ebreak() {
    encountered_ebreak = true;
    if (top_ptr) {
        printf("DPI-C: EBREAK instruction detected at PC=0x%08x\n", top_ptr->pc);
    } else {
        printf("DPI-C: EBREAK instruction detected, but top_ptr is null\n");
    }
}

// 简单内存模型（存储指令）
struct Memory {
    uint32_t* mem;
    size_t size;
};

void Memory_init(Memory* mem, size_t size) {
    mem->size = size;
    mem->mem = (uint32_t*)malloc(size * sizeof(uint32_t));
    
    // 清零内存
    for (int i = 0; i < size; i++) {
        mem->mem[i] = 0;
    }
    
    // 在0x80000000地址处加载指令
    mem->mem[0] = 0x00200093;  // addi x1, x0, 2  (PC=0x80000000)
    mem->mem[1] = 0x00308093;  // addi x1, x1, 3  (PC=0x80000004)
    // mem->mem[2] = 0x00300013;  // addi x0, x0, 3  (PC=0x80000008)
    mem->mem[3] = 0x00308493;  // addi x9, x1, 3  (PC=0x8000000c)
    mem->mem[4] = 0x00100073;  // ebreak         (PC=0x80000010)
}

uint32_t Memory_read(Memory* mem, uint32_t addr) {
    // 将地址转换为数组索引（减去0x80000000偏移）
    uint32_t index = (addr - 0x80000000) / 4;
    if (index < mem->size) {
        return mem->mem[index];
    } else {
        printf("Memory read out of bounds: 0x%08x\n", addr);
        return 0;
    }
}

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vtop* top = new Vtop;
    top_ptr = top;  // 设置全局指针
    
    Memory mem;
    Memory_init(&mem, 1024);

    // 初始化信号
    top->clk = 0;
    top->rst = 1;
    top->mem_rdata = 0;

    // 初始复位序列
    top->rst = 1;
    top->clk = 0;
    top->eval();
    top->clk = 1;
    top->eval();
    top->clk = 0;
    top->rst = 0;  // 释放复位
    top->eval();

    while (!encountered_ebreak) {
        // 打印当前状态
        printf("PC: 0x%08x, Instruction: 0x%08x\n", top->pc, top->mem_rdata);
        
        // 从内存读取下一条指令（在时钟上升沿之前）
        uint32_t next_pc = top->pc + 4;
        top->mem_rdata = Memory_read(&mem, next_pc - 0x80000000);
        
        // 上升沿：执行当前指令
        top->clk = 1;
        top->eval();
        
        // 下降沿：准备下一个周期
        top->clk = 0;
        top->eval();
        
        // 检查ebreak
        if (top->mem_rdata == 0x00100073) { // ebreak指令
            encountered_ebreak = true;
        }
    }

    // 清理
    free(mem.mem);
    delete top;
    top_ptr = nullptr;  // 重置全局指针
    return 0;
}
