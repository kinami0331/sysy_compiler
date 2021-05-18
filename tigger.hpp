//
// Created by KiNAmi on 2021/5/17.
//

#ifndef SYSY_COMPILER_TIGGER_HPP
#define SYSY_COMPILER_TIGGER_HPP

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

class EeyoreFuncDefNode;

struct TiggerVarInfo {
    bool isArray;
    unsigned int pos;
    bool isParam;
    bool isTempVar;
    bool isGlobal;
    bool isLocal;
    unsigned int arraySize;
    bool inReg;
    bool inUse;
    bool isWrited;
    int regNum;

    TiggerVarInfo() {
        isArray = false;
        pos = 0;
        isParam = false;
        isTempVar = false;
        isGlobal = false;
        isLocal = false;
        arraySize = 0;
        inReg = false;
        regNum = 0;
        isWrited = false;
        inUse = false;
    };

};


class TiggerBaseNode {
public:
    TiggerNodeType nodeType;

    virtual string toTiggerString() { return ""; };

    virtual void generateTigger(ostream &out, int indent);

    virtual void generateRiscv(ostream &out, int indent) {};

    static inline void outBlank(ostream &out, int n) {
        for(int i = 0; i < n; i++)
            out << ' ';
    }

    TiggerBaseNode() {};
};

class TiggerRootNode : public TiggerBaseNode {
public:
    static map<string, TiggerVarInfo> tiggerGlobalSymbol;
    static map<string, string> eeyoreSymbolToTigger;
    vector<TiggerBaseNode *> childList;

    TiggerRootNode() {
        nodeType = TiggerNodeType::ROOT;
    }

    void generateTigger(ostream &out, int indent) override;

    void generateRiscv(ostream &out, int indent) override;

};

class TiggerGlobalDeclNode : public TiggerBaseNode {
public:
    string name;

    TiggerGlobalDeclNode(string _name) {
        nodeType = TiggerNodeType::GLOBAL_DECL;
        name = _name;
    }

    string toTiggerString() override;

    void generateRiscv(ostream &out, int indent) override;

};

class TiggerCommentNode : public TiggerBaseNode {
public:
    string comment;

    TiggerCommentNode(const string &t) {
        nodeType = TiggerNodeType::COMMENT;
        comment = t;
    }

    string toTiggerString() override;

    void generateTigger(ostream &out, int indent) override;

    void generateRiscv(ostream &out, int indent) override {};
};

class TiggerAssignNode : public TiggerBaseNode {
public:
    TiggerAssignType assignType;
    OpType opType;
    bool isLeftArray;
    string leftReg;
    string rightReg1;
    string rightReg2;
    int rightNum;

    TiggerAssignNode(OpType op, const string &left, const string &op1, const string &op2) {
        nodeType = TiggerNodeType::ASSIGN;
        assignType = TiggerAssignType::RR_BINARY;
        opType = op;
        leftReg = left;
        rightReg1 = op1;
        rightReg2 = op2;
    }

    TiggerAssignNode(OpType op, const string &left, const string &op1, int op2) {
        nodeType = TiggerNodeType::ASSIGN;
        assignType = TiggerAssignType::RN_BINARY;
        opType = op;
        leftReg = left;
        rightReg1 = op1;
        rightNum = op2;
    }

    TiggerAssignNode(OpType op, const string &left, const string &op1) {
        nodeType = TiggerNodeType::ASSIGN;
        assignType = TiggerAssignType::R_UNARY;
        opType = op;
        rightReg1 = op1;
        leftReg = left;
    }

    TiggerAssignNode(const string &left, const string &op1) {
        nodeType = TiggerNodeType::ASSIGN;
        assignType = TiggerAssignType::R;
        rightReg1 = op1;
        leftReg = left;
    }

    explicit TiggerAssignNode(const string &left, int op) {
        nodeType = TiggerNodeType::ASSIGN;
        assignType = TiggerAssignType::N;
        rightNum = op;
        leftReg = left;
    }

    TiggerAssignNode(const string &left, int num, const string &op1) {
        nodeType = TiggerNodeType::ASSIGN;
        assignType = TiggerAssignType::LEFT_ARRAY;
        isLeftArray = true;
        leftReg = left;
        rightNum = num;
        rightReg1 = op1;
    }

    TiggerAssignNode(const string &left, const string &op1, int num) {
        nodeType = TiggerNodeType::ASSIGN;
        assignType = TiggerAssignType::RIGHT_ARRAY;
        leftReg = left;
        rightNum = num;
        rightReg1 = op1;
    }

    string toTiggerString() override;

    void generateRiscv(ostream &out, int indent) override;

};

class TiggerLoadNode : public TiggerBaseNode {
public:
    bool isGlobal;
    string globalVarName;
    int stackPos;
    string tarReg;

    explicit TiggerLoadNode(const string &name, const string &tar) {
        nodeType = TiggerNodeType::LOAD;
        isGlobal = true;
        globalVarName = name;
        tarReg = tar;
    }

    explicit TiggerLoadNode(int pos, const string &tar) {
        nodeType = TiggerNodeType::LOAD;
        isGlobal = false;
        stackPos = pos;
        tarReg = tar;
    }

    string toTiggerString() override;

    void generateRiscv(ostream &out, int indent) override;
};

class TiggerLoadAddrNode : public TiggerBaseNode {
public:
    bool isGlobal;
    string globalVarName;
    int stackPos;
    string tarReg;

    explicit TiggerLoadAddrNode(const string &name, const string &tar) {
        nodeType = TiggerNodeType::LOAD_ADDR;
        isGlobal = true;
        globalVarName = name;
        tarReg = tar;
    }

    explicit TiggerLoadAddrNode(int pos, const string &tar) {
        nodeType = TiggerNodeType::LOAD_ADDR;
        isGlobal = false;
        stackPos = pos;
        tarReg = tar;
    }

    string toTiggerString() override;

    void generateRiscv(ostream &out, int indent) override;
};

class TiggerStoreNode : public TiggerBaseNode {
public:
    string tarReg;
    int tarPos;

    TiggerStoreNode(const string &reg, int pos) {
        nodeType = TiggerNodeType::STORE;
        tarReg = reg;
        tarPos = pos;
    }

    string toTiggerString() override;

    void generateRiscv(ostream &out, int indent) override;

};

class TiggerLabelNode : public TiggerBaseNode {
public:
    string label;

    explicit TiggerLabelNode(const string &l) {
        nodeType = TiggerNodeType::LABEL;
        label = l;
    }

    string toTiggerString() override {
        return label + ":";
    }

    void generateRiscv(ostream &out, int indent) override;
};

class TiggerFuncCallNode : public TiggerBaseNode {
public:
    string funcName;

    explicit TiggerFuncCallNode(const string &name) {
        nodeType = TiggerNodeType::FUNC_CALL;
        funcName = name;
    }

    string toTiggerString() override {
        return "call " + funcName;
    }

    void generateRiscv(ostream &out, int indent) override;
};

class TiggerReturnNode : public TiggerBaseNode {
public:
    int stackSize;

    TiggerReturnNode() {
        nodeType = TiggerNodeType::RETURN;
    }

    string toTiggerString() override {
        return "return";
    }

    void generateTigger(ostream &out, int indent) override;

    void generateRiscv(ostream &out, int indent) override;
};

class TiggerIfGotoNode : public TiggerBaseNode {
public:
    bool isEq;
    string condReg;
    string label;

    TiggerIfGotoNode(bool eq, const string &reg, const string &l) {
        nodeType = TiggerNodeType::IF_GOTO;
        isEq = eq;
        condReg = reg;
        label = l;
    }

    string toTiggerString() override {
        if(isEq)
            return "if " + condReg + " == x0 goto " + label;
        else
            return "if " + condReg + " != x0 goto " + label;
    }

    void generateRiscv(ostream &out, int indent) override;
};

class TiggerGotoNode : public TiggerBaseNode {
public:
    string label;

    explicit TiggerGotoNode(const string &l) {
        nodeType = TiggerNodeType::GOTO;
        label = l;
    }

    string toTiggerString() override {
        return "goto " + label;
    }

    void generateRiscv(ostream &out, int indent) override;
};


class TiggerFuncDefNode : public TiggerBaseNode {
public:
    map<string, TiggerVarInfo> symbolInfo;
    map<string, string> eeyoreSymbolToTigger;
    vector<TiggerBaseNode *> childList;
    unsigned int paramNum;
    unsigned int stackSize;
    string funcName;
    // 最多有22个寄存器可用
    // s0 为后面保留，例如临时读入一个数字
    // s1 s2 s3永远留给临时变量
    // t0 保留，用于计算地址
    string regs[22] = {"s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t1", "t2", "t3", "t4", "t5", "t6", "a7",
                       "a6", "a5", "a4", "a3", "a2", "a1", "a0"};
    string regUseName[22];
    bool regUse[22] = {false};
    // 默认有14个寄存器可用
    unsigned int validRegNum = 14;
    unsigned int usedParamRegNum = 8;
    unsigned int baseStackTop = 0;

    TiggerFuncDefNode(map<string, TiggerVarInfo> &_symbolInfo, map<string, string> &_eeyoreSymbolToTigger) {
        nodeType = TiggerNodeType::FUNC_DEF;
        symbolInfo = _symbolInfo;
        eeyoreSymbolToTigger = _eeyoreSymbolToTigger;
    }

    void translateEeyore(EeyoreFuncDefNode *eeyoreFunc);

    pair<string, TiggerBaseNode *> symbolToReg(const string &symbol, bool is_t0 = true);

    pair<string, vector<TiggerBaseNode *>> getSymbolReg(const string &symbol, bool needValue);

    static int currentPos;

    pair<int, vector<TiggerBaseNode *>> simpleAllocateReg(const string &tiggerName);

    string testGetReg(int cnt) {
        if(cnt < validRegNum)
            return regs[cnt];
        return "r" + std::to_string(cnt);
    }

    string getRegName(int n) {
        if(n < validRegNum)
            return regs[n];
        assert(false);
    }

    void generateTigger(ostream &out, int indent) override;

    void generateRiscv(ostream &out, int indent) override;
};

#endif //SYSY_COMPILER_TIGGER_HPP
