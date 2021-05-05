#include "ast.hpp"

BaseNode::BaseNode() {
    nodeType = NodeType::BASE;
}

IdentNode::IdentNode() {
    nodeType = NodeType::IDENT;
    name = Util::getNodeTypeName(nodeType);
    id = "";
}

IdentNode::IdentNode(string _id) {
    nodeType = NodeType::IDENT;
    id = std::move(_id);
    name = Util::getNodeTypeName(nodeType) + "(" + id + ")";
}

Op1Node::Op1Node(OpType type) {
    nodeType = NodeType::OP_1;
    opType = type;
    name = Util::getNodeTypeName(nodeType) + "(" + Util::getOpTypeName(type) + ")";
}

Op1Node::Op1Node() {
    nodeType = NodeType::OP_1;
    opType = OpType::NONE;
}

Op2Node::Op2Node() {
    nodeType = NodeType::OP_2;
    opType = OpType::NONE;
}

Op2Node::Op2Node(OpType type) {
    nodeType = NodeType::OP_2;
    opType = type;
    name = Util::getNodeTypeName(nodeType) + "(" + Util::getOpTypeName(type) + ")";
}

NumberNode::NumberNode() {
    nodeType = NodeType::NUMBER;
    num = 0;
};

NumberNode::NumberNode(int _num) {
    nodeType = NodeType::NUMBER;
    num = _num;
    name = Util::getNodeTypeName(nodeType) + "(" + to_string(num) + ")";
}

BreakNode::BreakNode() {
    nodeType = NodeType::BREAK;
    name = Util::getNodeTypeName(nodeType);
}

ContinueNode::ContinueNode() {
    nodeType = NodeType::CONTINUE;
    name = Util::getNodeTypeName(nodeType);
}

RootNode::RootNode() {
    nodeType = NodeType::ROOT;
    name = Util::getNodeTypeName(nodeType);
}


CEInBracketsNode::CEInBracketsNode() {
    nodeType = NodeType::CE_IN_BRACKET;
    name = Util::getNodeTypeName(nodeType);
}

ValDeclNode::ValDeclNode() {
    nodeType = NodeType::VAL_DECL;
    name = Util::getNodeTypeName(nodeType);
}

ValDeclNode::ValDeclNode(bool _isConst) {
    nodeType = NodeType::VAL_DECL;
    isConst = _isConst;
    if (isConst)
        name = "ValDecl(Const)";
    else
        name = "ValDecl(NotConst)";
}

ValDefNode::ValDefNode() {
    nodeType = NodeType::VAR_DEF;
    name = Util::getNodeTypeName(nodeType);
}

InitValNode::InitValNode() {
    nodeType = NodeType::INIT_VAL;
    isList = false;
    name = Util::getNodeTypeName(nodeType);
}

InitValNode::InitValNode(bool _isList) {
    nodeType = NodeType::INIT_VAL;
    isList = _isList;
    name = Util::getNodeTypeName(nodeType);
}

FuncDefNode::FuncDefNode() {
    nodeType = NodeType::FUNC_DEF;
    isReturnTypeInt = false;
    name = Util::getNodeTypeName(nodeType);
}

FuncDefNode::FuncDefNode(bool isInt) {
    nodeType = NodeType::FUNC_DEF;
    isReturnTypeInt = isInt;
    name = Util::getNodeTypeName(nodeType);
    if (isInt)
        name = name + "(int)";
    else
        name = name + "(void)";
}

// 若干个[const]
ArgumentNode::ArgumentNode() {
    nodeType = NodeType::ARGUMENT;
    name = Util::getNodeTypeName(nodeType);
}

BlockNode::BlockNode() {
    nodeType = NodeType::BLOCK;
    name = Util::getNodeTypeName(nodeType);
}

NullStmtNode::NullStmtNode() {
    nodeType = NodeType::NULL_STMT;
    name = Util::getNodeTypeName(nodeType);
}

ExpStmtNode::ExpStmtNode() {
    nodeType = NodeType::EXP_STMT;
    name = Util::getNodeTypeName(nodeType);
}

AssignNode::AssignNode() {
    nodeType = NodeType::ASSIGN;
    name = Util::getNodeTypeName(nodeType);
}

IfNode::IfNode() {
    nodeType = NodeType::IF;
    name = Util::getNodeTypeName(nodeType);
}

WhileNode::WhileNode() {
    nodeType = NodeType::WHILE;
    name = Util::getNodeTypeName(nodeType);
}

ReturnNode::ReturnNode() {
    nodeType = NodeType::RETURN;
    name = Util::getNodeTypeName(nodeType);
}

CondNode::CondNode() {
    nodeType = NodeType::COND;
    name = Util::getNodeTypeName(nodeType);
}

ConstExpNode::ConstExpNode() {
    nodeType = NodeType::CONST_EXP;
    name = Util::getNodeTypeName(nodeType);
}

ExpNode::ExpNode(ExpType type) {
    nodeType = NodeType::EXP;
    expType = type;
    name = Util::getNodeTypeName(nodeType) + "(" +
           Util::getExpTypeName(expType) + ")";
}

FuncCallNode::FuncCallNode() {
    nodeType = NodeType::FUNC_CALL;
    name = Util::getNodeTypeName(nodeType);
}

LValNode::LValNode() {
    nodeType = NodeType::L_VAL;
    name = Util::getNodeTypeName(nodeType);
}