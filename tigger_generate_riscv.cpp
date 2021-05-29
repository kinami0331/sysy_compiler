//
// Created by KiNAmi on 2021/5/19.
//
#include "tigger.hpp"

void TiggerRootNode::generateRiscv(ostream &out, int indent) {
    for(auto ptr:childList)
        ptr->generateRiscv(out, indent);
}

void TiggerGlobalDeclNode::generateRiscv(ostream &out, int indent) {
    auto varInfo = TiggerRootNode::tiggerGlobalSymbol[name];
    if(varInfo.isArray) {
        out << ".comm " << name << ", " << varInfo.arraySize * 4 << ", 4\n";
    } else {
        out << ".global " << name << "\n";
        out << ".section .sdata\n";
        out << ".align 2\n";
        out << ".type     global_var, @object\n";
        out << ".size     global_var, 4\n";
        out << name << ":\n";
        out << ".word 0\n";
    }
}

void TiggerAssignNode::generateRiscv(ostream &out, int indent) {
    switch(assignType) {
        case TiggerAssignType::RR_BINARY:
            switch(opType) {
                case OpType::opPlus:
                    out << "add " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    break;
                case OpType::opDec:
                    out << "sub " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    break;
                case OpType::opMul:
                    out << "mul " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    break;
                case OpType::opDiv:
                    out << "div " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    break;
                case OpType::opMod:
                    out << "rem " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    break;
                case OpType::opG:
                    out << "sgt " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    break;
                case OpType::opL:
                    out << "slt " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    break;
                case OpType::opGE:
                    out << "slt " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    out << "seqz " << leftReg << "," << leftReg << "\n";
                    break;
                case OpType::opLE:
                    out << "sgt " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    out << "seqz " << leftReg << "," << leftReg << "\n";
                    break;
                case OpType::opE:
                    out << "xor " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    out << "seqz " << leftReg << "," << leftReg << "\n";
                    break;
                case OpType::opNE:
                    out << "xor " << leftReg << "," << rightReg1 << "," << rightReg2 << "\n";
                    out << "snez " << leftReg << "," << leftReg << "\n";
                    break;
                default:
                    exit(-1);
            }
            break;
        case TiggerAssignType::RN_BINARY:
            assert(false);
        case TiggerAssignType::R_UNARY:
            switch(opType) {
                case OpType::opDec:
                    out << "neg " << leftReg << "," << rightReg1 << "\n";
                    break;
                case OpType::opNot:
                    out << "seqz " << leftReg << "," << rightReg1 << "\n";
                    break;
                default:
                    assert(false);
            }
            break;
        case TiggerAssignType::R:
            out << "mv " << leftReg << "," << rightReg1 << "\n";
            break;
        case TiggerAssignType::N:
            out << "li " << leftReg << "," << rightNum << "\n";
            break;
        case TiggerAssignType::LEFT_ARRAY:
            assert(rightNum >= -2048 && rightNum <= 2047);
            out << "sw " << rightReg1 << "," << rightNum << "(" << leftReg << ")\n";
            break;
        case TiggerAssignType::RIGHT_ARRAY:
            assert(rightNum >= -2048 && rightNum <= 2047);
            out << "lw " << leftReg << "," << rightNum << "(" << rightReg1 << ")\n";
            break;
    }
}

void TiggerLoadNode::generateRiscv(ostream &out, int indent) {
    if(isGlobal) {
        out << "lui " << tarReg << ",%hi(" << globalVarName << ")\n";
        out << "lw " << tarReg << ",%lo(" << globalVarName << ")(" << tarReg << ")\n";
    } else {
        out << "li " + tarReg << "," << stackPos * 4 << "\n";
        out << "add " << tarReg << ", " << tarReg << ", sp\n";
        out << "lw " << tarReg << ", 0(" + tarReg << ")\n";
    }
}

void TiggerLoadAddrNode::generateRiscv(ostream &out, int indent) {
    if(isGlobal) {
        out << "la " << tarReg << "," << globalVarName << "\n";
    } else {
        out << "li " << tarReg << ", " << stackPos * 4 << "\n";
        out << "add " << tarReg << ",sp," + tarReg << "\n";
    }
}

void TiggerStoreNode::generateRiscv(ostream &out, int indent) {
    if(tarReg == "s0") {
        out << "li t0," << tarPos * 4 << "\n";
        out << "add t0, t0, sp\n";
        out << "sw " << tarReg << ",0(t0)\n";
    }
    else {
        out << "li s0," << tarPos * 4 << "\n";
        out << "add s0, s0, sp\n";
        out << "sw " << tarReg << ",0(s0)\n";
    }
}

void TiggerLabelNode::generateRiscv(ostream &out, int indent) {
    out << "." << label << ":\n";
}

void TiggerFuncCallNode::generateRiscv(ostream &out, int indent) {
    out << "call " << funcName.substr(2) << "\n";
}

void TiggerReturnNode::generateRiscv(ostream &out, int indent) {
    int STK = (stackSize / 4 + 1) * 16;
    // 恢复s
    for(int i = 0; i < 12; i++) {
        outBlank(out, indent);
        TiggerLoadNode(i, "s" + std::to_string(i)).generateRiscv(out, indent);
    }
    out << "li t0," << STK - 4 << "\n";
    out << "add t0, t0, sp\n";
    out << "lw ra, 0(t0)\n";
    out << "li t0," << STK << "\n";
    out << "add sp, sp, t0\n";
    out << "ret\n";
}

void TiggerIfGotoNode::generateRiscv(ostream &out, int indent) {
    if(isEq) {
        out << "beq " << condReg << ",x0,." << label << "\n";
    } else {
        out << "bne " << condReg << ",x0,." << label << "\n";
    }
}

void TiggerGotoNode::generateRiscv(ostream &out, int indent) {
    out << "j ." << label << "\n";
}

void TiggerFuncDefNode::generateRiscv(ostream &out, int indent) {
    int STK = (stackSize / 4 + 1) * 16;
    out << ".text\n";
    out << ".align  2\n";
    out << ".global " << funcName.substr(2) << "\n";
    out << ".type " << funcName.substr(2) << ", @function\n";
    out << funcName.substr(2) << ":\n";
    out << "li t0," << -STK << "\n";
    out << "add sp, sp, t0\n";

    out << "li t0," << STK - 4 << "\n";
    out << "add t0, t0, sp\n";
    out << "sw ra, 0(t0)\n";

//     保存寄存器
//     保存s0 - s11
    for(int i = 0; i < 12; i++) {
        outBlank(out, 4);
        TiggerStoreNode("s" + std::to_string(i), i).generateRiscv(out, indent);
    }

    for(auto ptr: childList)
        ptr->generateRiscv(out, indent + 4);

    out << ".size " << funcName.substr(2) << ", .-" << funcName.substr(2) << "\n";
}