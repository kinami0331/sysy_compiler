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
    unsigned int arraySize;
    bool inReg;

    VarInfo() {
        isArray = false;
        pos = 0;
        isParam = false;
        isTempVar = false;
        isGlobal = false;
        arraySize = 0;
        inReg = false;
    };

    VarInfo(bool _isArray, bool _isParam, bool _isTempVar, bool _isGlobal, unsigned int _pos,
            unsigned int _arraySize) : isArray(_isArray), pos(_pos), isParam(_isParam), isTempVar(_isTempVar),
                                       isGlobal(_isGlobal), arraySize(_arraySize) {}

};


class TiggerBaseNode {
public:
    TiggerNodeType nodeType;
};

class TiggerRootNode : public TiggerBaseNode {
public:
    map<string, TiggerVarInfo> tiggerGlobalSymbol;
    map<string, string> eeyoreSymbolToTigger;
    vector<TiggerBaseNode *> childList;

    TiggerRootNode() {
        nodeType = TiggerNodeType::ROOT;
    }

};

class TiggerGlobalDeclNode : public TiggerBaseNode {
public:
    string name;

    TiggerGlobalDeclNode(string _name) {
        nodeType = TiggerNodeType::GLOBAL_DECL;
        name = _name;
    }
};

class TiggerCommentNode : public TiggerBaseNode {
public:
    string comment;

    TiggerCommentNode(const string &t) {
        nodeType = TiggerNodeType::COMMENT;
        comment = t;
    }
};


class TiggerFuncDefNode : public TiggerBaseNode {
public:
    map<string, TiggerVarInfo> symbolInfo;
    map<string, string> eeyoreSymbolToTigger;

    TiggerFuncDefNode(map<string, TiggerVarInfo> &_symbolInfo, map<string, string> &_eeyoreSymbolToTigger) {
        nodeType = TiggerNodeType::FUNC_DEF;
        symbolInfo = _symbolInfo;
        eeyoreSymbolToTigger = _eeyoreSymbolToTigger;
    }

    void translateEeyore(EeyoreFuncDefNode *eeyoreFunc);
};

#endif //SYSY_COMPILER_TIGGER_HPP
