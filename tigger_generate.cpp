//
// Created by KiNAmi on 2021/5/18.
//

#include "tigger.hpp"

string TiggerAssignNode::toTiggerString() {
    switch(assignType) {
        case TiggerAssignType::RR_BINARY:
            return leftReg + " = " + rightReg1 + " " + Util::getOpTypeName(opType) + " " + rightReg2;
        case TiggerAssignType::RN_BINARY:
            return leftReg + " = " + rightReg1 + " " + Util::getOpTypeName(opType) + " " + std::to_string(rightNum);
        case TiggerAssignType::R_UNARY:
            return leftReg + " = " + Util::getOpTypeName(opType) + " " + rightReg1;
        case TiggerAssignType::R:
            return leftReg + " = " + rightReg1;
        case TiggerAssignType::N:
            return leftReg + " = " + std::to_string(rightNum);
        case TiggerAssignType::LEFT_ARRAY:
            return leftReg + "[" + std::to_string(rightNum) + "] = " + rightReg1;
        case TiggerAssignType::RIGHT_ARRAY:
            return leftReg + " = " + rightReg1 + "[" + std::to_string(rightNum) + "]";
    }
    return "";
}

string TiggerGlobalDeclNode::toTiggerString() {
    auto varInfo = TiggerRootNode::tiggerGlobalSymbol[name];
    if(varInfo.isArray)
        return name + " = malloc " + std::to_string(varInfo.arraySize * 4);
    else
        return name + " = 0";
}

string TiggerStoreNode::toTiggerString() {
    return "store " + tarReg + " " + std::to_string(tarPos);
}

string TiggerLoadNode::toTiggerString() {
    if(isGlobal)
        return "load " + globalVarName + " " + tarReg;
    else
        return "load " + std::to_string(stackPos) + " " + tarReg;
}

string TiggerCommentNode::toTiggerString() {
    return comment;
}

void TiggerCommentNode::generateTigger(ostream &out, int indent) {
    outBlank(out, indent);
    out << toTiggerString() << "\n";
}

string TiggerLoadAddrNode::toTiggerString() {
    if(isGlobal)
        return "loadaddr " + globalVarName + " " + tarReg;
    else
        return "loadaddr " + std::to_string(stackPos) + " " + tarReg;
}

void TiggerRootNode::generateTigger(ostream &out, int indent) {
    for(auto ptr: childList)
        ptr->generateTigger(out, indent);
}

void TiggerFuncDefNode::generateTigger(ostream &out, int indent) {
    out << funcName << " [" << paramNum << "] [" << stackSize << "]\n";
//     保存寄存器
//     保存s0 - s11
    for(int i = 0; i < 12; i++) {

        if(inUseReg.count("s" + std::to_string(i)) > 0) {
            outBlank(out, 4);
            out << TiggerStoreNode("s" + std::to_string(i), i).toTiggerString() << "\n";
        }
    }
    out << flush;

    for(auto ptr: childList)
        ptr->generateTigger(out, indent + 4);

    out << "end " << funcName << "\n";
}

void TiggerBaseNode::generateTigger(ostream &out, int indent) {
    outBlank(out, indent);
    out << toTiggerString() << "\n";
}

void TiggerReturnNode::generateTigger(ostream &out, int indent) {
    // 恢复s
    outBlank(out, indent);
    out << "return\n";
}

string TiggerExpIfGotoNode::toTiggerString() {
    string rst = "if " + op1 + " " + Util::getOpTypeName(opType) + " " + op2 + " goto " + label;
    return rst;
}
