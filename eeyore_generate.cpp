//
// Created by KiNAmi on 2021/5/13.
//
#include "eeyore_ast.hpp"

void EeyoreRootNode::generateEeyore(ostream &out, int indent) {
    for(auto ptr:childList)
        ptr->generateEeyore(out, indent);
}

void EeyoreFuncDefNode::generateEeyore(ostream &out, int indent) {
    out << name << " [" << paramNum << "]\n";

    int curIndent = indent + 4;
    for(auto ptr:childList) {
        if(ptr->nodeType == EeyoreNodeType::BLOCK_BEGIN)
            curIndent += 4;
        else if(ptr->nodeType == EeyoreNodeType::BLOCK_END)
            curIndent -= 4;
        assert(curIndent >= 0);
        ptr->generateEeyore(out, curIndent);
    }
    out << "end " << name << "\n";
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

void EeyoreFuncCallNode::generateEeyore(ostream &out, int indent) {
    // 传参
    for(int i = 0; i < paramList.size(); i++) {
        outBlank(out, indent);
        out << "param " << paramList[i]->to_string() << "\n";
    }
    outBlank(out, indent);
    if(hasReturnValue)
        out << returnSymbol << " = ";
    out << "call " << name << "\n";
}

void EeyoreAssignNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << leftValue->to_eeyore_string() << " = " << rightTerm->to_eeyore_string() << "\n";
}

void EeyoreLabelNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << label << ":\n";
}

void EeyoreReturnNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << "return";
    if(hasReturnValue)
        out << " " << returnValuePtr->to_eeyore_string();
    out << "\n";
}

void EeyoreGotoNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << "goto " << label << "\n";
}

void EeyoreIfGotoNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    if(isEq)
        out << "if " << condRightValue->to_eeyore_string() << " == 0" << " goto " << label << "\n";
    else
        out << "if " << condRightValue->to_eeyore_string() << " != 0" << " goto " << label << "\n";
}

void EeyoreCommentNode::generateEeyore(ostream &out, int indent) {
    outBlank(out, indent);
    out << comment << "\n";
}

void EeyoreFillInitNode::generateEeyore(ostream &out, int indent) {
    auto varInfo = EeyoreBaseNode::getVarInfo(varName);
    assert(!varInfo.isTempVar && !varInfo.isParam && varInfo.isArray && varInfo.isInitialized);
    for(int i = varInfo.initializedCnt; i < varInfo.arraySize; i++) {
        outBlank(out, indent);
        out << varName << "[" << std::to_string(i * 4) << "] = 0\n";
    }
}



