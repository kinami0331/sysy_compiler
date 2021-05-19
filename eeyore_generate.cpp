//
// Created by KiNAmi on 2021/5/13.
//
#include "eeyore_ast.hpp"

void EeyoreRootNode::generateEeyore(ostream &out, int indent) {
    for(auto ptr:childList)
        ptr->generateEeyore(out, indent);
}

void EeyoreFuncDefNode::generateEeyore(ostream &out, int indent) {
    out << funcName << " [" << paramNum << "]\n";

    int curIndent = indent + 4;
    for(auto ptr:childList) {
        if(ptr->nodeType == EeyoreNodeType::BLOCK_BEGIN)
            curIndent += 4;
        else if(ptr->nodeType == EeyoreNodeType::BLOCK_END)
            curIndent -= 4;
        assert(curIndent >= 0);
        ptr->generateEeyore(out, curIndent);
    }
    out << "end " << funcName << "\n";
}


void EeyoreVarDeclNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    assert(EeyoreBaseNode::globalSymbolTable.count(name) > 0);
    auto varInfo = EeyoreBaseNode::globalSymbolTable[name];
    assert(!varInfo.isParam);
    if(varInfo.isArray && !varInfo.isTempVar)
        out << "var " << varInfo.arraySize * 4 << " " << name << "\n";
    else
        out << "var " << name << "\n";
}

string EeyoreVarDeclNode::to_eeyore_string() {
    assert(EeyoreBaseNode::globalSymbolTable.count(name) > 0);
    auto varInfo = EeyoreBaseNode::globalSymbolTable[name];
    assert(!varInfo.isParam);
    if(varInfo.isArray && !varInfo.isTempVar)
        return "var " + std::to_string(varInfo.arraySize * 4) + " " + name;
    else
        return "var " + name;
}

void EeyoreFuncCallNode::generateEeyore(ostream &out, int indent) {
    // 传参
    for(int i = 0; i < paramList.size(); i++) {
        outBlank(out, indent);
        out << "param " << paramList[i]->to_eeyore_string() << "\n";
    }
    outBlank(out, indent);
    if(hasReturnValue)
        out << returnSymbol << " = ";
    out << "call " << name << "\n";
}

string EeyoreFuncCallNode::to_eeyore_string() {
    string rst = "";
    if(hasReturnValue)
        rst += returnSymbol + " = ";
    rst += "call " + name;
    return rst;
}

void EeyoreAssignNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << to_eeyore_string() << "\n";
}

string EeyoreAssignNode::to_eeyore_string() {
    return leftValue->to_eeyore_string() + " = " + rightTerm->to_eeyore_string();
}

void EeyoreLabelNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << to_eeyore_string() << "\n";
}

string EeyoreLabelNode::to_eeyore_string() {
    return label + ":";
}

void EeyoreReturnNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << to_eeyore_string() << "\n";
    out << "\n";
}

string EeyoreReturnNode::to_eeyore_string() {
    if(hasReturnValue)
        return "return " + returnValuePtr->to_eeyore_string();
    else
        return "return";
}

void EeyoreGotoNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << to_eeyore_string() << "\n";
}

string EeyoreGotoNode::to_eeyore_string() {
    return "goto " + label;
}

void EeyoreIfGotoNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << to_eeyore_string() << "\n";
}

string EeyoreIfGotoNode::to_eeyore_string() {
    if(isEq)
        return "if " + condRightValue->to_eeyore_string() + " == 0 goto " + label;
    else
        return "if " + condRightValue->to_eeyore_string() + " != 0 goto " + label;
}

void EeyoreCommentNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << comment << "\n";
}

void EeyoreFillInitNode::generateEeyore(ostream &out, int indent) {
    // TODO 这里似乎不用填充
    return;
    auto varInfo = EeyoreBaseNode::getVarInfo(varName);
    assert(!varInfo.isTempVar && !varInfo.isParam && varInfo.isArray && varInfo.isInitialized);
    for(int i = varInfo.initializedCnt; i < varInfo.arraySize; i++) {
        outBlank(out, indent);
        out << varName << "[" << std::to_string(i * 4) << "] = 0\n";
    }
}

void EeyoreFuncParamNode::generateEeyore(ostream &out, int indent) {
//    outBlank(out, indent);
//    out << to_eeyore_string() << "\n";
}

string EeyoreFuncParamNode::to_eeyore_string() {
    return "param " + param->to_eeyore_string();
}



