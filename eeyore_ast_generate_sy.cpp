//
// Created by KiNAmi on 2021/5/11.
//
#include "eeyore_ast.hpp"

map<string, VarInfo> EeyoreBaseNode::globalSymbolTable;
map<string, VarInfo> EeyoreBaseNode::paramSymbolTable;

bool EeyoreRightValueNode::isArray() {
    if(_isNum)
        return false;
    return EeyoreBaseNode::getVarInfo(name).isArray;
}

bool EeyoreRightValueNode::isArray2() {
    if(_isNum)
        return false;
    else if(name[0] == 'p') {
        if(parentNode->parentNode->nodeType == EeyoreNodeType::FUNC_DEF) {
            auto funcPtr = static_cast<EeyoreFuncDefNode *>(parentNode->parentNode);
            return funcPtr->paramSymbolTable[name].isArray;
        } else if(parentNode->parentNode->parentNode->nodeType == EeyoreNodeType::FUNC_DEF) {
            auto funcPtr = static_cast<EeyoreFuncDefNode *>(parentNode->parentNode->parentNode);
            return funcPtr->paramSymbolTable[name].isArray;
        }
        assert(false);
    }
    return EeyoreBaseNode::getVarInfo(name).isArray;
}

bool EeyoreLeftValueNode::isArray2() {
    if(isArray)
        return true;
    else if(name[0] == 'p') {
        if(parentNode->parentNode->nodeType == EeyoreNodeType::FUNC_DEF) {
            auto funcPtr = static_cast<EeyoreFuncDefNode *>(parentNode->parentNode);
            return funcPtr->paramSymbolTable[name].isArray;
        } else if(parentNode->parentNode->parentNode->nodeType == EeyoreNodeType::FUNC_DEF) {
            auto funcPtr = static_cast<EeyoreFuncDefNode *>(parentNode->parentNode->parentNode);
            return funcPtr->paramSymbolTable[name].isArray;
        }
        assert(false);
    }
    return EeyoreBaseNode::getVarInfo(name).isArray;
}

void EeyoreBaseNode::generateSysy(ostream &out, int indent) {}

void EeyoreRootNode::generateSysy(ostream &out, int indent) {
    for(auto ptr:childList)
        ptr->generateSysy(out, indent);
}

void EeyoreFuncDefNode::generateSysy(ostream &out, int indent) {
    out << (hasReturnVal ? "int " : "void ") << funcName << '(';
    for(int i = 0; i < paramNum; i++) {
        if(i > 0)
            out << ", ";
        string paramName = "p" + std::to_string(i);
        assert(paramSymbolTable.count(paramName) > 0);
        if(paramSymbolTable[paramName].isArray)
            out << "int " << paramName << "[]";
        else
            out << "int " << paramName;
    }
    out << ") {\n";
    int curIndent = indent + 4;
    for(auto ptr:childList) {
        if(ptr->nodeType == EeyoreNodeType::BLOCK_BEGIN)
            curIndent += 4;
        else if(ptr->nodeType == EeyoreNodeType::BLOCK_END)
            curIndent -= 4;
        assert(curIndent >= 0);
        ptr->generateSysy(out, curIndent);
    }
    out << "}\n";
}

void EeyoreVarDeclNode::generateSysy(ostream &out, int indent) {
    outBlank(out, indent);
    assert(EeyoreBaseNode::globalSymbolTable.count(name) > 0);
    auto varInfo = EeyoreBaseNode::globalSymbolTable[name];
    assert(!varInfo.isParam);
    if(varInfo.isArray) {
        if(varInfo.isTempVar)
            out << "int* " << name << ";\n";
        else {
            out << "int " << name << "[" << varInfo.arraySize << "]";
            if(varInfo.isInitialized)
                out << " = {0}";
            out << ";\n";
        }
    } else {
        out << "int " << name;
        if(varInfo.isInitialized)
            out << " = 0";
        out << ";\n";
    }


}

void EeyoreFuncCallNode::generateSysy(ostream &out, int indent) {
    outBlank(out, indent);
    if(hasReturnValue)
        out << returnSymbol << " = ";
    out << name << "(";
    for(int i = 0; i < paramList.size(); i++) {
        if(i > 0)
            out << ", ";
        out << paramList[i]->to_string();
    }
    out << ");\n";
}

void EeyoreAssignNode::generateSysy(ostream &out, int indent) {
    outBlank(out, indent);
    out << leftValue->to_string() << " = " << rightTerm->to_string() << ";\n";
}

void EeyoreLabelNode::generateSysy(ostream &out, int indent) {
    outBlank(out, indent);
    out << label << ": ;\n";
}

void EeyoreReturnNode::generateSysy(ostream &out, int indent) {
    outBlank(out, indent);
    out << "return";
    if(hasReturnValue)
        out << " " << returnValuePtr->to_string();
    out << ";\n";
}

void EeyoreGotoNode::generateSysy(ostream &out, int indent) {
    outBlank(out, indent);
    out << "goto " << label << ";\n";
}

void EeyoreIfGotoNode::generateSysy(ostream &out, int indent) {
    outBlank(out, indent);
    if(isEq)
        out << "if(" << condRightValue->to_string() << " == 0" << ") goto " << label << ";\n";
    else
        out << "if(" << condRightValue->to_string() << " != 0" << ") goto " << label << ";\n";
}

void EeyoreCommentNode::generateSysy(ostream &out, int indent) {
    outBlank(out, indent);
    out << comment << "\n";
}

