// 导入 DPI-C 函数声明
import "DPI-C" function void npc_ebreak();

//auipc,lui, jalr,jal,sw
// 扩展ALU以支持跳转指令的返回地址写入
module ALU #(parameter DATA_WIDTH=32)(
    input [DATA_WIDTH-1:0] rs1,
    input [DATA_WIDTH-1:0] pc,
    input [DATA_WIDTH-1:0] imm_ext,
    input isjal,
    input isjalr,
    input isaddi,
    input isauipc,
    input islui,
    output reg [DATA_WIDTH-1:0] y
);
always @(*) begin
    case (1'b1)
        isaddi:  y = rs1 + imm_ext;
        isauipc: y = pc + imm_ext;
        islui:   y = imm_ext;
        // 跳转指令：返回地址为pc+4
        isjal:   y = pc + 4;
        isjalr:  y = pc + 4;
        default: y = 32'd0;
    endcase
end
endmodule

module RegisterFile #(parameter ADDR_WIDTH=5, DATA_WIDTH=32) (
    input clk,
    input rst,
    // 写端口
    input [DATA_WIDTH-1:0] wdata,
    input [ADDR_WIDTH-1:0] waddr,
    input wen,
    // 读端口（新增rs2）
    input [ADDR_WIDTH-1:0] raddr1,
    input [ADDR_WIDTH-1:0] raddr2,  // sw需要读取rs2
    output reg [DATA_WIDTH-1:0] rdata1,
    output reg [DATA_WIDTH-1:0] rdata2  // rs2数据输出
);
reg [DATA_WIDTH-1:0] rf [0:(1<<ADDR_WIDTH)-1];

// 复位和写操作（不变）
integer i;
always @(posedge clk or posedge rst) begin
    if (rst) begin
        for (i = 0; i < (1<<ADDR_WIDTH); i = i + 1)
            rf[i] <= 0;
    end
    else if (wen && waddr != 0) begin
        rf[waddr] <= wdata;
    end
end

// 读操作（新增rdata2）
always @(*) begin
    rdata1 = (raddr1 == 0) ? 0 : rf[raddr1];
    rdata2 = (raddr2 == 0) ? 0 : rf[raddr2];  // 读取rs2
end
endmodule

module top(
    input wire clk,
    input wire rst,
    output reg halt,
    output [31:0] mem_addr,
    input [31:0] mem_rdata,
    output reg mem_wen,
    output reg [31:0] mem_wdata,
    output reg [31:0] pc
);

wire [31:0] inst = mem_rdata;

// 1. 指令解码
wire [6:0] opcode = inst[6:0];
wire [2:0] funct3 = inst[14:12];
wire [4:0] rd     = inst[11:7];
wire [4:0] rs1    = inst[19:15];
wire [4:0] rs2    = inst[24:20];

// 2. 立即数提取与扩展
wire [11:0] imm_i = inst[31:20];
wire [19:0] imm_u = inst[31:12];
wire [19:0] imm_j = {inst[31], inst[19:12], inst[20], inst[30:21]};
wire [11:0] imm_s = {inst[31:25], inst[11:7]};

wire [31:0] imm_i_ext = {{20{imm_i[11]}}, imm_i};
wire [31:0] imm_u_ext = {imm_u, 12'd0};
wire [31:0] imm_j_ext = {{11{imm_j[19]}}, imm_j, 1'b0};
wire [31:0] imm_s_ext = {{20{imm_s[11]}}, imm_s};

// 3. 跳转目标地址计算（移至此处）
wire [31:0] jal_target  = pc + imm_j_ext;
wire [31:0] jalr_target = (rs1_data + imm_i_ext) & 32'hfffffffe;

// 4. 控制信号定义
wire isaddi  = (opcode == 7'b0010011) && (funct3 == 3'b000);
wire isauipc = (opcode == 7'b0010111);
wire islui   = (opcode == 7'b0110111);
wire isjal   = (opcode == 7'b1101111);
wire isjalr  = (opcode == 7'b1100111) && (funct3 == 3'b000);
wire issw    = (opcode == 7'b0100011) && (funct3 == 3'b010);
wire is_ebreak = (opcode == 7'b1110011) && (inst[11:0] == 12'b000000000001);

// 5. 数据通路
wire [31:0] rs1_data;
wire [31:0] rs2_data;
wire [31:0] alu_out;

// 跳转目标地址计算
wire [31:0] jal_target  = pc + imm_j_ext;                  // jal目标地址
wire [31:0] jalr_target = (rs1_data + imm_i_ext) & 32'hfffffffe; // jalr目标地址（对齐）

// 数据前递逻辑
//wire [31:0] rs1_data_forwarded;
//assign rs1_data_forwarded = (wen && (rs1 == rd) && (rs1 != 0)) ? alu_out : rs1_data;

// 存储器接口
assign mem_addr = pc;
assign mem_wen = 1'b0;  // 只读

// 寄存器文件实例（连接rs2读端口）
RegisterFile rf (
    .clk(clk),
    .rst(rst),
    .waddr(rd),
    .wdata(alu_out),
    .wen(wen),
    .raddr1(rs1),
    .raddr2(rs2),  // 连接sw的rs2
    .rdata1(rs1_data),
    .rdata2(rs2_data)  // 输出rs2数据
);

// ALU实例
ALU alu (
    .rs1(rs1_data),         // 源寄存器1数据
    .pc(pc),                // 当前PC值
    .imm_ext(               // 根据指令类型选择对应的立即数扩展信号
        isaddi  ? imm_i_ext :
        isauipc ? imm_u_ext :
        islui   ? imm_u_ext :
        isjalr  ? imm_i_ext :
                  32'd0     // 其他情况默认0
    ),
    .isjal(isjal),          // jal指令控制信号
    .isjalr(isjalr),        // jalr指令控制信号
    .isaddi(isaddi),        // addi指令控制信号
    .isauipc(isauipc),      // auipc指令控制信号
    .islui(islui),          // lui指令控制信号
    .y(alu_out)             // ALU输出结果（最后一个端口可省略逗号）
);

// 内存地址和写信号控制
always @(*) begin
    // 访存地址：sw使用rs1+imm_s_ext，其他指令默认用pc（取指）
    mem_addr = issw ? (rs1_data + imm_s_ext) : pc;
    // 内存写使能：仅sw指令有效
    mem_wen = issw;
    // 内存写数据：sw写入rs2的数据
    mem_wdata = rs2_data;
end

// PC更新（在原有always块中扩展）
always @(posedge clk or posedge rst) begin
    if (rst) begin
        pc <= 32'h80000000;
        halt <= 1'b0;
    end 
    else if (is_ebreak) begin  // 优先处理ebreak
        npc_ebreak();
        halt <= 1'b1;
    end
    else if (!halt) begin
        // 根据跳转指令选择新PC，否则默认pc+4
        pc <= isjal  ? jal_target  :
              isjalr ? jalr_target :
                       pc + 4;     // 非跳转指令，PC+4
    end
end
endmodule
