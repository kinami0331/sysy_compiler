//
// Created by KiNAmi on 2021/5/1.
//


#ifndef SYSY_COMPILER_AST_HPP
#define SYSY_COMPILER_AST_HPP

#include <iostream>
#include <optional>
#include <memory>
#include <utility>
#include <vector>
#include <string>
#include <utility>
#include <iomanip>
#include <cstdarg>
#include <string>
#include <cstdio>
#include <cstring>
#include <cassert>

using namespace std;

enum class NodeType {
    IDENT, ROOT, CONST_DECL, CONST_DEF, CE_IN_BRACKET, CONST_INIT_VAL, VAL_DECL, VAR_DEF,
    INIT_VAL, FUNC_DEF, ARGUMENT, BLOCK, ASSIGN, IF, WHILE, BREAK, CONTINUE, RETURN, OP_1,
    OP_2, EXP, UNARY_EXP, NUMBER, L_VAL, COND, BASE, AST, LEAF, OR_COND, AND_COND, FUNC_CALL,
    STMT, CONST_EXP, NULL_STMT, EXP_STMT
};

enum class OpType {
    opPlus, opDec, opMul, opDiv, opMod, opNot, opAnd, opOr, opG, opL, opGE, opLE, opE, opNE,
    NONE
};

enum class ExpType {
    Number, LVal, FuncCall, BinaryExp, UnaryExp
};

class Util {
public:
    static string getNodeTypeName(NodeType type) {
        switch (type) {
            case NodeType::EXP_STMT:
                return "exp_stmt";
            case NodeType::IDENT:
                return "ident";
            case NodeType::ROOT:
                return "root";
            case NodeType::CONST_DECL:
                return "const_decl";
            case NodeType::CONST_DEF:
                return "const_def";
            case NodeType::CE_IN_BRACKET:
                return "ce_in_bracket";
            case NodeType::CONST_INIT_VAL:
                return "const_init_val";
            case NodeType::VAL_DECL:
                return "val_decl";
            case NodeType::VAR_DEF:
                return "var_def";
            case NodeType::INIT_VAL:
                return "init_val";
            case NodeType::FUNC_DEF:
                return "func_def";
            case NodeType::ARGUMENT:
                return "argument";
            case NodeType::BLOCK:
                return "block";
            case NodeType::ASSIGN:
                return "assign";
            case NodeType::IF:
                return "if";
            case NodeType::WHILE:
                return "while";
            case NodeType::BREAK:
                return "break";
            case NodeType::CONTINUE:
                return "continue";
            case NodeType::RETURN:
                return "return";
            case NodeType::OP_1:
                return "op_1";
            case NodeType::OP_2:
                return "op_2";
            case NodeType::EXP:
                return "exp";
            case NodeType::UNARY_EXP:
                return "unary_exp";
            case NodeType::NUMBER:
                return "number";
            case NodeType::L_VAL:
                return "l_val";
            case NodeType::COND:
                return "cond";
            case NodeType::BASE:
                return "base";
            case NodeType::AST:
                return "ast";
            case NodeType::LEAF:
                return "leaf";
            case NodeType::OR_COND:
                return "or_cond";
            case NodeType::AND_COND:
                return "and_cond";
            case NodeType::FUNC_CALL:
                return "func_call";
            case NodeType::STMT:
                return "stmt";
            case NodeType::CONST_EXP:
                return "const_exp";
            case NodeType::NULL_STMT:
                return "null_stmt";
        }
        return "";
    };

    static string getExpTypeName(ExpType type) {
        switch (type) {
            case ExpType::Number:
                return "Num";
            case ExpType::LVal:
                return "LVal";
            case ExpType::UnaryExp:
                return "Unary";
            case ExpType::FuncCall:
                return "Func";
            case ExpType::BinaryExp:
                return "Binary";
        }
        return "";
    }

    static string getOpTypeName(OpType type) {
        switch (type) {
            case OpType::opPlus:
                return "+";
            case OpType::opDec:
                return "-";
            case OpType::opMul:
                return "*";
            case OpType::opDiv:
                return "/";
            case OpType::opMod:
                return "%";
            case OpType::opNot:
                return "!";
            case OpType::opAnd:
                return "&&";
            case OpType::opOr:
                return "||";
            case OpType::opG:
                return ">";
            case OpType::opL:
                return "<";
            case OpType::opGE:
                return ">=";
            case OpType::opLE:
                return "<=";
            case OpType::opE:
                return "==";
            case OpType::opNE:
                return "!=";
            case OpType::NONE:
                return "none";
        }
        return "";
    }
};

class BaseNode {
public:
    string name;
    NodeType nodeType;
    std::vector<BaseNode *> childNodes;

    BaseNode();

    BaseNode *pushNodePtr(BaseNode *ptr);

    BaseNode *pushNodePtrList(std::vector<BaseNode *> &list);

    virtual ~BaseNode() = default;

    virtual void evalNow() {};

    int getChildsNum();

    static void outBlank(ostream &out, int n);

    virtual void print();

    virtual void print(int blank, bool newLine);

    virtual void generateSysy(ostream &out, int indent);

};

typedef BaseNode *NodePtr;

typedef std::vector<NodePtr> NodePtrList;

// 终结符
class IdentNode : public BaseNode {
public:
    std::string id;

    IdentNode();

    explicit IdentNode(string _id);

    void generateSysy(ostream &out, int ident) override;
};

// 操作符
class Op1Node : public BaseNode {
public:
    OpType opType;

    explicit Op1Node(OpType type);

    void generateSysy(ostream &out, int ident) override;

private:
    Op1Node();
};

class Op2Node : public BaseNode {
public:
    OpType opType;

    explicit Op2Node(OpType type);

    void generateSysy(ostream &out, int ident) override;

private:
    Op2Node();
};

// 数字
class NumberNode : public BaseNode {
public:
    NumberNode();

    explicit NumberNode(int _num);

    int num;

    void generateSysy(ostream &out, int ident) override;
};

class BreakNode : public BaseNode {
public:
    BreakNode();

    void generateSysy(ostream &out, int ident) override;
};

class ContinueNode : public BaseNode {
public:
    ContinueNode();

    void generateSysy(ostream &out, int ident) override;
};

// 对应CompUnit，子节点种类为ConstDecl、VarDecl、FuncDef
class RootNode : public BaseNode {
public:
    RootNode();

    // 默认generateSyss
};

// 变量声明
// 变量和常量的地位应该是等价的，我们使用一个isConst来进行区分
class ValDeclNode : public BaseNode {
public:
    bool isConst;

    // 默认类型都是int，这里没有显式地写出
    // 包含若干个定义
    explicit ValDeclNode(bool _isConst);

    // 是一个语句，需要缩进
    void generateSysy(ostream &out, int ident) override;

private:
    ValDeclNode();
};

// 方括号以及其中的常量表达式
class CEInBracketsNode : public BaseNode {
public:
    CEInBracketsNode();

    void generateSysy(ostream &out, int ident) override;

    vector<int> adjustArray();

    vector<int> getDimVector();
};

// 变量声明中的声明结构
class ValDefNode : public BaseNode {
public:
    ValDefNode();

    void generateSysy(ostream &out, int ident) override;

    void adjustArray();
};

// 变量初始化的值
class InitValNode : public BaseNode {
public:
    // 是否为{}列表
    bool isList;

    explicit InitValNode(bool _isList);

    void generateSysy(ostream &out, int ident) override;

//    void adjust(vector<int> &dims);

    void standardizing(vector<int> &dims);

    bool hasOnlyOneExp();

    NodePtr getTheSingleVal();

private:
    InitValNode();
};

// 函数定义
class FuncDefNode : public BaseNode {
public:
    // 返回值是否为int
    bool isReturnTypeInt;

    FuncDefNode();

    explicit FuncDefNode(bool isInt);

    void generateSysy(ostream &out, int ident) override;
};

// 函数形参
class ArgumentNode : public BaseNode {
public:
    // 类型肯定是int，舍去
    bool isArray;

    // 若干个[const]
    ArgumentNode();

    void generateSysy(ostream &out, int ident) override;
};

// block
class BlockNode : public BaseNode {
public:
    BlockNode();

    void generateSysy(ostream &out, int ident) override;
};

// 空语句：只有;
class NullStmtNode : public BaseNode {
public:
    NullStmtNode();

    void generateSysy(ostream &out, int ident) override;
};

// 表达式语句
class ExpStmtNode : public BaseNode {
public:
    ExpStmtNode();

    void generateSysy(ostream &out, int ident) override;
};

// 赋值语句
class AssignNode : public BaseNode {
public:
    AssignNode();

    void generateSysy(ostream &out, int ident) override;
};

// IF语句
class IfNode : public BaseNode {
public:
    IfNode();

    void generateSysy(ostream &out, int ident) override;
};

// while语句
class WhileNode : public BaseNode {
public:
    WhileNode();

    void generateSysy(ostream &out, int ident) override;
};

// 返回语句
class ReturnNode : public BaseNode {
public:
    ReturnNode();

    void generateSysy(ostream &out, int ident) override;
};

// 条件表达式，只包含一个exp节点
class CondNode : public BaseNode {
public:
    CondNode();
};

// 常量表达式，只包含一个exp节点
class ConstExpNode : public BaseNode {
public:
    ConstExpNode();

};

class ExpNode : public BaseNode {
public:
    ExpType expType;

    explicit ExpNode(ExpType type);

    void generateSysy(ostream &out, int ident) override;

    void evalNow() override;

    void setExpType(ExpType type) {
        expType = type;
        name = Util::getNodeTypeName(nodeType) + "(" +
               Util::getExpTypeName(expType) + ")";
    }

    bool isConst() const {
        return expType == ExpType::Number;
        // if (expType == ExpType::Number)
        //     return true;
        // return false;
    }

    void evalBinary();

    void evalUnary();


};

// 函数调用
class FuncCallNode : public BaseNode {
public:
    FuncCallNode();

    void generateSysy(ostream &out, int ident) override;
};

// 左值
class LValNode : public BaseNode {
public:
    LValNode();

    // 要么是一个标识符，要么标识符后面跟着若干个方括号+exp
    void generateSysy(ostream &out, int ident) override;
};


#endif //SYSY_COMPILER_AST_HPP
