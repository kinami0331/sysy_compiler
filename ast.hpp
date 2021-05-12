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
#include <map>
#include <algorithm>
#include "main.hpp"

using namespace std;

class AbstractValNode {
public:
    string newName;
    bool isConst;
    bool isArray;
    int constVal;
    vector<int> arrayDims;
    vector<int> arrayValues;

    virtual int getConstValue(vector<int> &dims);

    virtual int getConstValue();

    virtual int getIndex(vector<int> &dims);


};

typedef shared_ptr<map<string, AbstractValNode *>> SymbolTablePtr;

class BaseNode;

class EeyoreBaseNode;

class EeyoreRightValueNode;

class EeyoreRootNode;

typedef BaseNode *NodePtr;

typedef std::vector<NodePtr> NodePtrList;

class BaseNode {
public:

public:
    string name;
    NodeType nodeType;
    std::vector<BaseNode *> childNodes;
    SymbolTablePtr symbolTablePtr;
    NodePtr parentNodePtr{};

    static unsigned int definedValueCnt;
    static unsigned int tempValueCnt;
    static unsigned int labelCnt;

    BaseNode();

    BaseNode *pushNodePtr(BaseNode *ptr);

    BaseNode *pushNodePtrList(std::vector<BaseNode *> &list);

    virtual ~BaseNode() = default;

    static void outBlank(ostream &out, int n);

//    virtual void replaceSymbols(SymbolTablePtr parentTablePtr);

    virtual void print();

    virtual void print(int blank, bool newLine);

    virtual void generateSysy(ostream &out, int indent);

    virtual inline void replaceSymbols() {};

    virtual void standardizing();

    virtual void getParentSymbolTable();

    virtual void setParentPtr(BaseNode *ptr);

    virtual inline void updateSymbolTable(BaseNode *ptr) {};

    virtual inline void equivalentlyTransform() {};

    virtual inline NodePtrList extractStmtNode() { return NodePtrList({this}); };

    virtual vector<EeyoreBaseNode *> generateEeyoreNode();

    virtual EeyoreRootNode *generateEeyoreTree();

};


class IdentNode : public BaseNode {
public:
    std::string id;

    IdentNode();

    explicit IdentNode(string _id);

    void generateSysy(ostream &out, int ident) override;

    void replaceSymbols() override;
};

// unary operator
class Op1Node : public BaseNode {
public:
    OpType opType;

    explicit Op1Node(OpType type);

    void generateSysy(ostream &out, int ident) override;

private:
    Op1Node();
};

// binary operator
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
    static vector<BreakNode *> currentBreakNodeList;
    bool hasLabel = false;
    string label;

    BreakNode();

    void generateSysy(ostream &out, int ident) override;

    inline void setLabel(string &l) {
        label = l;
        hasLabel = true;
    }

    NodePtrList extractStmtNode() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};

class ContinueNode : public BaseNode {
public:
    ContinueNode();

    static vector<ContinueNode *> currentContinueNodeList;
    bool hasLabel = false;
    string label;

    void generateSysy(ostream &out, int ident) override;

    inline void setLabel(string &l) {
        label = l;
        hasLabel = true;
    }

    NodePtrList extractStmtNode() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};

// 对应CompUnit，子节点种类为ConstDecl、VarDecl、FuncDef
class RootNode : public BaseNode {
public:
    RootNode();

    void getParentSymbolTable() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};

// 变量声明
// 变量和常量的地位应该是等价的，我们使用一个isConst来进行区分
class VarDeclNode : public BaseNode {
    /*
     * childNodes: one or more ValDefNodes
     */
public:
    bool isConst;

    // 默认类型都是int，这里没有显式地写出
    // 包含若干个定义
    explicit VarDeclNode(bool _isConst);

    // 提供一个简便的创建临时变量声明的方法
    explicit VarDeclNode(string &ident);

    // 是一个语句，需要缩进
    void generateSysy(ostream &out, int ident) override;

    void updateSymbolTable(BaseNode *ptr) override;

    NodePtrList extractStmtNode() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;

private:
    VarDeclNode();
};

class CEInBracketsNode : public BaseNode {
public:
    CEInBracketsNode();

    void generateSysy(ostream &out, int ident) override;

    vector<int> flattenArray();

    vector<int> getDimVector();

};

class VarDefNode : public BaseNode, public AbstractValNode {
    /*
     * childNodes: an identNode, a CEInBracketsNode, an optional InitValNode (necessary for constVal)
     */
public:
    explicit VarDefNode(bool _isConst);

    void generateSysy(ostream &out, int ident) override;

    void flattenArray();

    void equivalentlyTransform() override;
//    void standardizing() override;

    NodePtrList extractStmtNode() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;

private:
    VarDefNode();

};

// 变量初始化的值
class InitValNode : public BaseNode {
public:
    // 是否为{}列表
    bool isList;

    explicit InitValNode(bool _isList);

    void generateSysy(ostream &out, int ident) override;

    void standardizingInitList(vector<int> &dims);

    void flattenArray(vector<int> &dims);

    vector<int> getIntList();

    vector<int> getIntList(vector<int> &dims);

    vector<NodePtr> getExpList();

    vector<NodePtr> getExpList(vector<int> &dims);

    bool hasOnlyOneExp();

    NodePtr getTheSingleVal();

//    void standardizing() override;

private:
    InitValNode();
};

// 函数定义
class FuncDefNode : public BaseNode {
    /*
     * childNodes: an IdentNode, 0 or some ArgumentNode, a blockNode
     */
public:
    // 返回值是否为int
    bool isReturnTypeInt;
    int paramCnt = 0;

    FuncDefNode();

    explicit FuncDefNode(bool isInt);

    void generateSysy(ostream &out, int ident) override;

    void getParentSymbolTable() override;

    void updateSymbolTable(BaseNode *ptr) override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};

// 函数形参
class ArgumentNode : public BaseNode, public AbstractValNode {
public:
    // 类型肯定是int，舍去
//    bool isArray{};

    ArgumentNode(bool _isArray);

    void generateSysy(ostream &out, int ident) override;

    void equivalentlyTransform() override;

private:
    ArgumentNode();
};

// block
class BlockNode : public BaseNode {
public:
    vector<EeyoreBaseNode *> eeyoreNodeList;

    BlockNode();

    void generateSysy(ostream &out, int ident) override;

    void getParentSymbolTable() override;

    void standardizing() override;

    NodePtrList extractStmtNode() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;

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

    NodePtrList extractStmtNode() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};

// 赋值语句
class AssignNode : public BaseNode {
public:
    AssignNode();

    AssignNode(string &LValIdent, NodePtr expNode);

    void generateSysy(ostream &out, int ident) override;

    NodePtrList extractStmtNode() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};

// IF语句
class IfNode : public BaseNode {
public:
    IfNode();

    void generateSysy(ostream &out, int ident) override;

    NodePtrList extractStmtNode() override;
};

// while语句
class WhileNode : public BaseNode {
public:
    WhileNode();

    void generateSysy(ostream &out, int ident) override;

    NodePtrList extractStmtNode() override;
};

// 返回语句
class ReturnNode : public BaseNode {
public:
    ReturnNode();

    void generateSysy(ostream &out, int ident) override;

    NodePtrList extractStmtNode() override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
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

    ExpNode(ExpType type, int n);

    explicit ExpNode(string &lValIdent);

    void generateSysy(ostream &out, int ident) override;

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

    void equivalentlyTransform() override;

    pair<NodePtr, NodePtrList> extractExp();

    pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>> extractEeyoreExp();

    pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>> extractEeyoreAndExp();

    pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>> extractEeyoreOrExp();


};

// 函数调用
class FuncCallNode : public BaseNode {
public:
    FuncCallNode();

    void generateSysy(ostream &out, int ident) override;

    NodePtrList extractStmtNode() override;
};

// 左值
class LValNode : public BaseNode {
public:
    bool isConst = false;

    LValNode();

    // 要么是一个标识符，要么标识符后面跟着若干个方括号+exp
    void generateSysy(ostream &out, int ident) override;

    void equivalentlyTransform() override;

    void replaceSymbols() override;
};


class IfGotoNode : public BaseNode {
public:
    string endIfLabel; // if fail
    string endElseLabel;

    explicit IfGotoNode(string &_label);

    IfGotoNode(string &_label, string &_elseLabel);


    void generateSysy(ostream &out, int ident) override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};

class WhileGotoNode : public BaseNode {
public:
    string beginLabel;
    string endLabel;

    WhileGotoNode(string &l1, string &l2);

    void generateSysy(ostream &out, int ident) override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};

class GotoNode : public BaseNode {
public:
    string label;

    explicit GotoNode(string &l) {
        label = l;
    }

    void generateSysy(ostream &out, int indent) override;

    vector<EeyoreBaseNode *> generateEeyoreNode() override;
};


#endif //SYSY_COMPILER_AST_HPP
