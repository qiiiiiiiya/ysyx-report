/***************************************************************************************
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
#include <cpu/difftest.h>
#include "../local-include/reg.h"

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  //把通用寄存器和PC与从DUT中读出的寄存器
  //的值进行比较. 若对比结果一致, 函数返回true
  for(int i=0;i<ARRLEN(cpu.gpr);i++){
    if(cpu.gpr[i]!=ref_r->gpr[i]){
      Log("Register gpr[%d] mismatch:DUT=0x%x,REF=0x%x",i,cpu.gpr[i],ref_r->gpr[i]);
      return false;
    }
    if(cpu.pc!=ref_r->pc){
      Log("Register pc mismatch:DUT=0x%x,REF=0x%x",cpu.pc,ref_r->pc);
      return false;
    }
  }
  printf("gooooooooooooooooooooooooooooooooooooooooood");
  return true;;
}

void isa_difftest_attach() {
}
