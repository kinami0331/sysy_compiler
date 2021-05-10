//
// Created by KiNAmi on 2021/5/6.
//
#include "ast.hpp"

void BaseNode::generateSysy(ostream &out, int indent) {
    for (auto &childNode : childNodes) {
        childNode->generateSysy(out, indent);
    }
}

void IdentNode::generateSysy(ostream &out, int ident) {
    out << id;
}

void Op1Node::generateSysy(ostream &out, int ident) {
    out << Util::getOpTypeName(opType);
}

void Op2Node::generateSysy(ostream &out, int ident) {
    out << Util::getOpTypeName(opType);
}


void NumberNode::generateSysy(ostream &out, int ident) {
    out << num;
}

void BreakNode::generateSysy(ostream &out, int ident) {
    outBlank(out, ident);
    out << "break;\n";
}

void ContinueNode::generateSysy(ostream &out, int ident) {
    outBlank(out, ident);
    out << "continue;\n";
}


void CEInBracketsNode::generateSysy(ostream &out, int ident) {
    int s = childNodes.size();
    for (int i = 0; i < s; i++) {
        out << "[";
        childNodes[i]->generateSysy(out, ident);
        out << "]";
    }
}

// 是一个语句，需要缩进
void VarDeclNode::generateSysy(ostream &out, int ident) {
    outBlank(out, ident);
//    if (isConst)
//        out << "const ";
    out << "int ";
    int s = childNodes.size();
    for (int i = 0; i < s; i++) {
        if (i != 0)
            out << ",";
        childNodes[i]->generateSysy(out, ident);
    }
    out << ";\n";
}

void VarDefNode::generateSysy(ostream &out, int ident) {
    int s = childNodes.size();
    // 变量名
    childNodes[0]->generateSysy(out, ident);
    // 中括号
    childNodes[1]->generateSysy(out, ident);
    if (s <= 2)
        return;
    // 赋值
    out << "=";
    childNodes[2]->generateSysy(out, ident);
}

void InitValNode::generateSysy(ostream &out, int ident) {
    if (isList) {
        out << "{";
        for (auto i = childNodes.begin(); i != childNodes.end(); i++) {
            if (i != childNodes.begin())
                out << ",";
            (*i)->generateSysy(out, ident);
        }
        out << "}";
    } else
        childNodes[0]->generateSysy(out, ident);
}

void FuncDefNode::generateSysy(ostream &out, int ident) {
    // 函数定义是一个完整语句，需要缩进
    // 虽然缩进应当是0
    outBlank(out, ident);
    int s = childNodes.size();
    // 返回值
    if (isReturnTypeInt)
        out << "int ";
    else
        out << "void ";

    // 函数名
    childNodes[0]->generateSysy(out, ident);

    // 形参
    out << "(";
    for (int i = 1; i < s - 1; i++) {
        if (i != 1)
            out << ",";
        childNodes[i]->generateSysy(out, ident);
    }
    out << ") ";
    // 函数体
    childNodes[s - 1]->generateSysy(out, ident + 4);
}

void ArgumentNode::generateSysy(ostream &out, int ident) {
    out << "int ";
    // 形参名
    childNodes[0]->generateSysy(out, ident);

    // 开始判断是否有数组
    int s = childNodes.size();
    if (s == 1)
        return;
    out << "[]";
    for (int i = 1; i < s; i++)
        childNodes[i]->generateSysy(out, ident);
}

void BlockNode::generateSysy(ostream &out, int ident) {
    assert(parentNodePtr != nullptr);
    if (parentNodePtr->nodeType == NodeType::BLOCK) {
        int s = (int) childNodes.size();
        outBlank(out, ident);
        out << "{\n";
        for (int i = 0; i < s; i++) {
            if (childNodes[i]->nodeType == NodeType::EXP) {
                outBlank(out, ident);
                childNodes[i]->generateSysy(out, ident);
                out << ";\n";
            } else
                childNodes[i]->generateSysy(out, ident + 4);
        }
        outBlank(out, ident);
        out << "}\n";
    } else {
        // 第一个花括号直接输出，不换行
        // ident指的是block内的语句的缩进
        out << "{\n";
        int s = (int) childNodes.size();
        for (int i = 0; i < s; i++) {
            if (childNodes[i]->nodeType == NodeType::EXP) {
                outBlank(out, ident);
                childNodes[i]->generateSysy(out, ident);
                out << ";\n";
            } else
                childNodes[i]->generateSysy(out, ident);
        }
        outBlank(out, ident - 4);
        out << "}\n";
    }
}

void NullStmtNode::generateSysy(ostream &out, int ident) {
    // outBlank(out, ident);
    // out << ";\n";
    // do nothing
}

void ExpStmtNode::generateSysy(ostream &out, int ident) {
    outBlank(out, ident);
    childNodes[0]->generateSysy(out, ident);
    out << ";\n";
}

void AssignNode::generateSysy(ostream &out, int ident) {
    outBlank(out, ident);
    childNodes[0]->generateSysy(out, ident);
    out << "=";
    childNodes[1]->generateSysy(out, ident);
    out << ";\n";
}

void IfNode::generateSysy(ostream &out, int ident) {
    outBlank(out, ident);
    out << "if(";
    // 这里是一个语句
    childNodes[0]->generateSysy(out, ident);
    out << ")";

    // 这里是一个stmt，做下判断
    switch (childNodes[1]->nodeType) {
        case NodeType::BLOCK:
            out << " ";
            childNodes[1]->generateSysy(out, ident + 4);
            break;
        case NodeType::EXP:
            out << "\n";
            outBlank(out, ident + 4);
            childNodes[1]->generateSysy(out, ident + 4);
            out << ";\n";
            break;
        default:
            out << "\n";
            childNodes[1]->generateSysy(out, ident + 4);
    }
    // 输出stmt内容

    // 是否有else
    if (childNodes.size() == 3) {
        outBlank(out, ident);
        out << "else";

        switch (childNodes[2]->nodeType) {
            case NodeType::BLOCK:
                out << " ";
                childNodes[2]->generateSysy(out, ident + 4);
                break;
            case NodeType::EXP:
                out << "\n";
                outBlank(out, ident + 4);
                childNodes[2]->generateSysy(out, ident + 4);
                out << ";\n";
                break;
            default:
                out << "\n";
                childNodes[2]->generateSysy(out, ident + 4);
        }
    }
}

void WhileNode::generateSysy(ostream &out, int ident) {
    outBlank(out, ident);
    out << "while(";
    childNodes[0]->generateSysy(out, ident);
    out << ") ";
    childNodes[1]->generateSysy(out, ident + 4);
}

void ReturnNode::generateSysy(ostream &out, int ident) {
    outBlank(out, ident);
    out << "return";
    if (childNodes.size() == 1) {
        out << " ";
        childNodes[0]->generateSysy(out, ident);
    }
    out << ";\n";
}

void ExpNode::generateSysy(ostream &out, int ident) {
    switch (expType) {
        case ExpType::LVal:
        case ExpType::FuncCall:
        case ExpType::Number:
            childNodes[0]->generateSysy(out, ident);
            break;
        case ExpType::BinaryExp:
        case ExpType::UnaryExp:
            out << "(";
            BaseNode::generateSysy(out, ident);
            out << ")";
            break;
    }
}

void FuncCallNode::generateSysy(ostream &out, int ident) {
    childNodes[0]->generateSysy(out, ident);
    out << "(";
    int s = childNodes.size();
    for (int i = 1; i < s; i++) {
        if (i != 1)
            out << ",";
        childNodes[i]->generateSysy(out, ident);
    }
    out << ")";
}

void LValNode::generateSysy(ostream &out, int ident) {
    childNodes[0]->generateSysy(out, ident);
    int s = childNodes.size();
    if (s == 1)
        return;
    for (int i = 1; i < s; i++) {
        out << "[";
        childNodes[i]->generateSysy(out, ident);
        out << "]";
    }

}