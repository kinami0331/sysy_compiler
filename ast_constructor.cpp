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

VarDeclNode::VarDeclNode() {
    nodeType = NodeType::VAL_DECL;
    isConst = false;
    name = Util::getNodeTypeName(nodeType);
}

VarDeclNode::VarDeclNode(bool _isConst) {
    nodeType = NodeType::VAL_DECL;
    isConst = _isConst;
    if(isConst)
        name = "val_decl(C)";
    else
        name = "val_decl(NC)";
}

VarDeclNode::VarDeclNode(string &ident) {
    nodeType = NodeType::VAL_DECL;
    isConst = false;
    name = "val_decl(t)";
    childNodes.push_back((new VarDefNode(false))
                                 ->pushNodePtr(new IdentNode(ident))
                                 ->pushNodePtr(new CEInBracketsNode()));

}

VarDefNode::VarDefNode() {
    nodeType = NodeType::VAR_DEF;
    name = Util::getNodeTypeName(nodeType);
    isConst = false;
}

VarDefNode::VarDefNode(bool _isConst) {
    nodeType = NodeType::VAR_DEF;
    name = Util::getNodeTypeName(nodeType);
    isConst = _isConst;
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
    if(isInt)
        name = name + "(int)";
    else
        name = name + "(void)";
}

ArgumentNode::ArgumentNode() {
    nodeType = NodeType::ARGUMENT;
    name = Util::getNodeTypeName(nodeType);
}

ArgumentNode::ArgumentNode(bool _isArray) {
    nodeType = NodeType::ARGUMENT;
    name = Util::getNodeTypeName(nodeType);
    isArray = _isArray;
    isConst = false;
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
    name = Util::getNodeTypeName(nodeType);
}

AssignNode::AssignNode(string &LValIdent, NodePtr expNode) {
    nodeType = NodeType::ASSIGN;
    name = Util::getNodeTypeName(nodeType);
    childNodes.push_back((new LValNode())->pushNodePtr(new IdentNode(LValIdent)));
    childNodes.push_back(expNode);
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

ExpNode::ExpNode(ExpType type, int n) {
    nodeType = NodeType::EXP;
    assert(type == ExpType::Number);
    expType = type;
    childNodes.push_back(new NumberNode(n));
    name = "exp(Num)";
}

ExpNode::ExpNode(string &lValIdent) {
    nodeType = NodeType::EXP;
    expType = ExpType::LVal;
    childNodes.push_back((new LValNode)->pushNodePtr((new IdentNode(lValIdent))));
    name = "exp(LVal)";
}

FuncCallNode::FuncCallNode() {
    nodeType = NodeType::FUNC_CALL;
    name = Util::getNodeTypeName(nodeType);
}

LValNode::LValNode() {
    nodeType = NodeType::L_VAL;
    name = Util::getNodeTypeName(nodeType);
}


IfGotoNode::IfGotoNode(string &_label) {
    nodeType = NodeType::IF_GOTO;
    name = Util::getNodeTypeName(nodeType);
    ifLabel = _label;
}

IfGotoNode::IfGotoNode(string &_label, string &_elseLabel) {
    nodeType = NodeType::IF_GOTO;
    name = Util::getNodeTypeName(nodeType);
    ifLabel = _label;
    elseLabel = _elseLabel;
}

WhileGotoNode::WhileGotoNode(string &l1, string &l2) {
    nodeType = NodeType::WHILE_GOTO;
    name = Util::getNodeTypeName(nodeType);
    beginLabel = l1;
    endLabel = l2;
}

