//
// Created by KiNAmi on 2021/5/11.
//

#ifndef SYSY_COMPILER_EEYORE_AST_HPP
#define SYSY_COMPILER_EEYORE_AST_HPP

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

class EeyoreBaseNode;

class EeyoreFuncDefNode;

class TiggerRootNode;

typedef EeyoreBaseNode *EeyoreNodePtr;

typedef std::vector<EeyoreNodePtr> EeyoreNodePtrList;

struct VarInfo {
    bool isArray;
    bool isParam;
    bool isTempVar;
    bool isInitialized;
    bool isGlobal;
    unsigned int arraySize;
    int initValue;
    int initializedCnt;
//    vector<int> initArrayValue;

    VarInfo() = default;

    VarInfo(bool _isArray, bool _isParam, bool _isTempVar, bool _isInitialized, unsigned int _arraySize, int _initValue,
            int _initCnt, bool _isGlobal) :
            isArray(_isArray), isParam(_isParam), isTempVar(_isTempVar), isInitialized(_isInitialized),
            arraySize(_arraySize), initValue(_initValue), initializedCnt(_initCnt), isGlobal(_isGlobal) {}

    string to_string() const {
        return "| isArray: " + std::to_string(isArray) + " | " +
               "isTempVar: " + std::to_string(isTempVar) + " | " +
               "isInitialized: " + std::to_string(isInitialized) + " | " +
               "arraySize: " + std::to_string(arraySize) + " | " +
               "initValue: " + std::to_string(initValue) + " | " +
               "initializedCnt: " + std::to_string(initializedCnt) + " |";
    }
};

class EeyoreBaseNode {
public:
    string name;
    EeyoreNodeType nodeType;
    static map<string, VarInfo> globalSymbolTable;
    static map<string, VarInfo> paramSymbolTable;
    EeyoreBaseNode *parentNode;

    static VarInfo &getVarInfo(string &_name) {
        if(_name[0] == 'p')
            return paramSymbolTable[_name];
        else
            return globalSymbolTable[_name];
    }

    static void registerSymbol(string &_name) {
        assert(globalSymbolTable.count(_name) == 0);
        globalSymbolTable.insert(make_pair(_name, VarInfo(false, false, false, false, 0, 0, 0, false)));
    }

    static void registerSymbol(string &_name, int init) {
        assert(globalSymbolTable.count(_name) == 0);
        globalSymbolTable.insert(make_pair(_name, VarInfo(false, false, false, true, 0, init, 0, false)));
    }

    static void registerTempSymbol(string &_name, bool _isArray = false) {
        assert(globalSymbolTable.count(_name) == 0);
        globalSymbolTable.insert(make_pair(_name, VarInfo(_isArray, false, true, false, 0, 0, 0, false)));
    }


    static void registerArraySymbol(string &_name, unsigned int _arraySize) {
        assert(globalSymbolTable.count(_name) == 0);
        globalSymbolTable.insert(make_pair(_name, VarInfo(true, false, false, false, _arraySize, 0, 0, false)));
    }

    static void registerArraySymbol(string &_name, unsigned int _arraySize, int initCnt) {
        assert(globalSymbolTable.count(_name) == 0);
        globalSymbolTable.insert(make_pair(_name, VarInfo(true, false, false, true, _arraySize, 0, initCnt, false)));
    }

    static void registerGlobalSymbol(string &_name) {
        assert(globalSymbolTable.count(_name) == 0);
        globalSymbolTable.insert(make_pair(_name, VarInfo(false, false, false, true, 0, 0, 0, true)));
    }

    static void registerGlobalArraySymbol(string &_name, unsigned int _arraySize, int initCnt) {
        assert(globalSymbolTable.count(_name) == 0);
        globalSymbolTable.insert(make_pair(_name, VarInfo(true, false, false, true, _arraySize, 0, initCnt, true)));
    }


    virtual void print() {
        cout << Util::getEeyoreNodeType(nodeType) << ": " << name << endl;
    }

    inline virtual string to_string() { return ""; }

    inline virtual string to_eeyore_string() { return ""; }

    virtual void generateSysy(ostream &out, int indent);

    virtual void generateEeyore(ostream &out, int indent) {};

    static inline void outBlank(ostream &out, int n) {
        for(int i = 0; i < n; i++)
            out << ' ';
    }

    inline void setParentPtr(EeyoreBaseNode *ptr) {
        parentNode = ptr;
    }
};

// struct
class EeyoreRootNode : public EeyoreBaseNode {
public:
    vector<EeyoreBaseNode *> childList;

    EeyoreRootNode() {
        nodeType = EeyoreNodeType::ROOT;
    }

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

    void generateCFG();

    void generateGraphviz(ostream &out);

    void simplifyTempVar();

public:
    TiggerRootNode *generateTigger();

};

class EeyoreBlockBeginNode : public EeyoreBaseNode {
public:
    EeyoreBlockBeginNode() {
        nodeType = EeyoreNodeType::BLOCK_BEGIN;
    }
};

class EeyoreBlockEndNode : public EeyoreBaseNode {
public:
    EeyoreBlockEndNode() {
        nodeType = EeyoreNodeType::BLOCK_END;
    }
};

// term
class EeyoreRightValueNode : public EeyoreBaseNode {
public:
    // if is the symbol, the symbol ident is 'name'
    bool _isNum;


    explicit EeyoreRightValueNode(int _num) {
        nodeType = EeyoreNodeType::RIGHT_VALUE;
        _isNum = true;
        value = _num;
    }

    explicit EeyoreRightValueNode(string &_symbol) {
        nodeType = EeyoreNodeType::RIGHT_VALUE;
        name = _symbol;
        _isNum = false;
    }

    bool isArray();

    bool isArray2();

    bool isLocalNotArray();

    inline bool isNum() const {
        return _isNum;
    }

    inline int getValue() const {
        assert(_isNum);
        return value;
    }

    inline string to_string() override {
        return _isNum ? std::to_string(value) : name;
    }

    inline string to_eeyore_string() override {
        return _isNum ? std::to_string(value) : name;
    }

private:
    int value = 0;
};

class EeyoreLeftValueNode : public EeyoreBaseNode {
public:
    // name
    bool isArray;
    EeyoreRightValueNode *rValNodePtr = nullptr;

    explicit EeyoreLeftValueNode(string &symbol) {
        nodeType = EeyoreNodeType::LEFT_VALUE;
        isArray = false;
        name = symbol;
    }

    EeyoreLeftValueNode(string &symbol, EeyoreRightValueNode *rPtr) {
        nodeType = EeyoreNodeType::LEFT_VALUE;
        isArray = true;
        name = symbol;
        rValNodePtr = rPtr;
        rValNodePtr->setParentPtr(this);
    }

    inline string to_string() override {
        if(isArray)
            return name + "[" + rValNodePtr->to_string() + " / 4]";
        else
            return name;
    }

    inline string to_eeyore_string() override {
        if(isArray)
            return name + "[" + rValNodePtr->to_eeyore_string() + "]";
        else
            return name;
    }

    bool isArray2();

    bool isLocalNotArray();
};

class EeyoreExpNode : public EeyoreBaseNode {
public:
    bool isBinary;
    OpType type;
    EeyoreRightValueNode *firstOperand;
    EeyoreRightValueNode *secondOperand;

    EeyoreExpNode(OpType _type, EeyoreRightValueNode *_first, EeyoreRightValueNode *_second) {
        nodeType = EeyoreNodeType::EXP;
        type = _type;
        firstOperand = _first;
        secondOperand = _second;
        isBinary = true;
        firstOperand->setParentPtr(this);
        secondOperand->setParentPtr(this);
    }

    EeyoreExpNode(OpType _type, EeyoreRightValueNode *_first) {
        nodeType = EeyoreNodeType::EXP;
        type = _type;
        firstOperand = _first;
        secondOperand = nullptr;
        isBinary = false;
        firstOperand->setParentPtr(this);
    }

    string to_string() override {
        if(isBinary) {
            if(firstOperand->isArray2())
                return "(int*)((long long)(" + firstOperand->to_string() + ") " + Util::getOpTypeName(type) + " " +
                       secondOperand->to_string() + ")";
            else if(secondOperand->isArray2())
                return "(int*)(" + firstOperand->to_string() + " " + Util::getOpTypeName(type) + " (long long)(" +
                       secondOperand->to_string() + "))";
            else
                return firstOperand->to_string() + " " + Util::getOpTypeName(type) + " " + secondOperand->to_string();
        } else
            return Util::getOpTypeName(type) + firstOperand->to_string();
    }

    inline string to_eeyore_string() override {
        if(isBinary)
            return firstOperand->to_eeyore_string() + " " + Util::getOpTypeName(type) + " " +
                   secondOperand->to_eeyore_string();
        else
            return Util::getOpTypeName(type) + firstOperand->to_eeyore_string();
    }

};

// stmt
class EeyoreCommentNode : public EeyoreBaseNode {
public:
    string comment;

    EeyoreCommentNode(string &_comment) {
        nodeType = EeyoreNodeType::COMMENT;
        comment = _comment;
    }

    EeyoreCommentNode(const string &_comment) {
        nodeType = EeyoreNodeType::COMMENT;
        comment = _comment;
    }

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

};

class EeyoreAssignNode : public EeyoreBaseNode {
public:
    EeyoreLeftValueNode *leftValue;
    EeyoreBaseNode *rightTerm;

    EeyoreAssignNode(EeyoreLeftValueNode *_leftValue, EeyoreBaseNode *_rightTerm) : leftValue(_leftValue),
                                                                                    rightTerm(_rightTerm) {
        nodeType = EeyoreNodeType::ASSIGN;
        leftValue->setParentPtr(this);
        rightTerm->setParentPtr(this);
    }

    void print() override {
        cout << Util::getEeyoreNodeType(nodeType) << ": " << leftValue->to_string() << " = " << rightTerm->to_string()
             << endl;
    }

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

    string to_eeyore_string() override;
};

class EeyoreVarDeclNode : public EeyoreBaseNode {
public:
    // ident is name
    explicit EeyoreVarDeclNode(string &_name) {
        nodeType = EeyoreNodeType::VAR_DECL;
        name = _name;
    }

    void print() override {
        cout << Util::getEeyoreNodeType(nodeType) << ": " << name << " " << EeyoreBaseNode::getVarInfo(name).to_string()
             << endl;
    }

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

    string to_eeyore_string() override;
};

class EeyoreFuncDefNode : public EeyoreBaseNode {
public:
    bool hasReturnVal;
    unsigned int paramNum;
    vector<EeyoreBaseNode *> childList;
    map<string, VarInfo> paramSymbolTable;
    string funcName;

    // ident is name
    EeyoreFuncDefNode(string &_name, bool _hasReturnVal, unsigned int _paramNum) {
        nodeType = EeyoreNodeType::FUNC_DEF;
        funcName = _name;
        hasReturnVal = _hasReturnVal;
        paramNum = _paramNum;
    }

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

public:
    void simplifyTempVar();
    unsigned int maxParamNum();

public:
    // for cfg
    class BasicBlock {
    public:
        unsigned int blockId;
        string blockLabel;
        vector<int> preNodeList;
        vector<int> nextNodeList;
        vector<EeyoreBaseNode *> stmtList;
        set<string> defSet;
        set<string> useSet;
        set<string> inSet;
        set<string> outSet;

        int cycleCnt = 0;

        BasicBlock() {
            blockLabel = "(null)";
            blockId = 0;
        }

        explicit BasicBlock(int n) {
            blockLabel = "(null)";
            blockId = n;
        }

        void insertStmt(EeyoreBaseNode *ptr) {
            stmtList.push_back(ptr);
        }

        void setLabelName(const string &name) {
            blockLabel = name;
        }

        void calcDefAndUseSet();

    };

    map<string, int> labelToBlockId;

    vector<BasicBlock *> basicBlockList;

    void generateCFG();

    void liveVarAnalysis();

    void calcDefAndUseSet();

    void calcInAndOutSet();

    void generateGraphviz(ostream &out);
};

class EeyoreLabelNode : public EeyoreBaseNode {
public:
    string label;

    explicit EeyoreLabelNode(string &_label) {
        nodeType = EeyoreNodeType::LABEL;
        label = _label;
    }
    // label: name

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

    string to_eeyore_string() override;
};

class EeyoreReturnNode : public EeyoreBaseNode {
public:
    bool hasReturnValue;
    EeyoreRightValueNode *returnValuePtr;


    explicit EeyoreReturnNode(EeyoreRightValueNode *_returnValue) {
        nodeType = EeyoreNodeType::RETURN;
        hasReturnValue = true;
        returnValuePtr = _returnValue;
        returnValuePtr->setParentPtr(this);
    }

    EeyoreReturnNode() {
        nodeType = EeyoreNodeType::RETURN;
        hasReturnValue = false;
        returnValuePtr = nullptr;
    }

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

    string to_eeyore_string() override;

private:


};

class EeyoreGotoNode : public EeyoreBaseNode {
public:
    string label;

    explicit EeyoreGotoNode(string &_label) {
        nodeType = EeyoreNodeType::GOTO;
        label = _label;
    }
    // goto: name

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

    string to_eeyore_string() override;

};

class EeyoreIfGotoNode : public EeyoreBaseNode {
public:
    EeyoreRightValueNode *condRightValue;
    string label;
    bool isEq;

    EeyoreIfGotoNode(EeyoreRightValueNode *_condRightValue, string &_label, bool _isEq = true) {
        nodeType = EeyoreNodeType::IF_GOTO;
        condRightValue = _condRightValue;
        label = _label;
        isEq = _isEq;
        condRightValue->setParentPtr(this);
    }

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

    string to_eeyore_string() override;
};

class EeyoreFuncParamNode : public EeyoreBaseNode {
public:
    EeyoreRightValueNode *param;

    explicit EeyoreFuncParamNode(EeyoreRightValueNode *p) {
        param = p;
        nodeType = EeyoreNodeType::PARAM;
    }

    void generateEeyore(ostream &out, int indent) override;

    string to_eeyore_string() override;

};

class EeyoreFuncCallNode : public EeyoreBaseNode {
public:
    bool hasReturnValue;
    string returnSymbol;
    vector<EeyoreRightValueNode *> paramList;

    EeyoreFuncCallNode(string &_name) {
        nodeType = EeyoreNodeType::FUNC_CALL;
        hasReturnValue = false;
        name = _name;
    }

    EeyoreFuncCallNode(string &_name, string &_returnSymbol) {
        nodeType = EeyoreNodeType::FUNC_CALL;
        hasReturnValue = true;
        returnSymbol = _returnSymbol;
        name = _name;
    }

    void pushParam(EeyoreRightValueNode *ptr) {
        paramList.push_back(ptr);
        ptr->setParentPtr(this);
    }

    void print() override {
        cout << Util::getEeyoreNodeType(nodeType) << ": ";
        if(hasReturnValue)
            cout << returnSymbol << " = " << name << endl;
        else
            cout << name << endl;
    }

    void generateSysy(ostream &out, int indent) override;

    void generateEeyore(ostream &out, int indent) override;

    string to_eeyore_string() override;
};

class EeyoreFillInitNode : public EeyoreBaseNode {
public:
    string varName;

    EeyoreFillInitNode(string &_name) {
        nodeType = EeyoreNodeType::FILL_INIT;
        varName = _name;
    }

    void generateEeyore(ostream &out, int indent) override;
};

class EeyoreGlobalInitNode : public EeyoreBaseNode {
public:
    vector<EeyoreBaseNode *> childList;

    EeyoreGlobalInitNode() {
        nodeType = EeyoreNodeType::GLOBAL_INIT;
    };

    void generateEeyore(ostream &out, int indent) override {
        for(auto ptr:childList)
            ptr->generateEeyore(out, indent);
    }

    void generateSysy(ostream &out, int indent) override {
        for(auto ptr:childList)
            ptr->generateSysy(out, indent);
    }
};


#endif //SYSY_COMPILER_EEYORE_AST_HPP
