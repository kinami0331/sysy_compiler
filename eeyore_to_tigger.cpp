//
// Created by KiNAmi on 2021/5/17.
//
#include "eeyore_ast.hpp"
#include "tigger.hpp"
#include <stack>
#include <set>
#include <cstdlib>
#include <ctime>
#include <cmath>

map<string, TiggerVarInfo> TiggerRootNode::tiggerGlobalSymbol;
map<string, string> TiggerRootNode::eeyoreSymbolToTigger;
map<string, int> TiggerRootNode::funcParams = {{"f_getint",          0},
                                               {"f_getch",           0},
                                               {"f_getarray",        1},
                                               {"f_putint",          1},
                                               {"f_putch",           1},
                                               {"f_putarray",        2},
                                               {"f__sysy_starttime", 1},
                                               {"f__sysy_stoptime",  1}};

TiggerRootNode *EeyoreRootNode::generateTigger() {
    auto tiggerRoot = new TiggerRootNode();
    int s = childList.size();
    unsigned int globalCnt = 0;
    for(int i = 0; i < s; i++) {
        // 全局变量改名
        if(childList[i]->nodeType == EeyoreNodeType::VAR_DECL) {
            auto varDeclPtr = static_cast<EeyoreVarDeclNode *>(childList[i]);
            auto newName = "v" + std::to_string(globalCnt);
            globalCnt++;
            TiggerRootNode::eeyoreSymbolToTigger[varDeclPtr->name] = newName;
            TiggerRootNode::tiggerGlobalSymbol[newName] = TiggerVarInfo();
            TiggerRootNode::tiggerGlobalSymbol[newName].isGlobal = true;
            if(EeyoreBaseNode::globalSymbolTable[varDeclPtr->name].isArray) {
                TiggerRootNode::tiggerGlobalSymbol[newName].isArray = true;
                TiggerRootNode::tiggerGlobalSymbol[newName].arraySize = EeyoreBaseNode::globalSymbolTable[varDeclPtr->name].arraySize;
            }
            tiggerRoot->childList.push_back(new TiggerGlobalDeclNode(newName));
        } else {
            auto funDefPtr = static_cast<EeyoreFuncDefNode *>(childList[i]);
            auto tiggerFunc = new TiggerFuncDefNode(TiggerRootNode::tiggerGlobalSymbol,
                                                    TiggerRootNode::eeyoreSymbolToTigger);
            tiggerFunc->translateEeyore(funDefPtr);
            tiggerRoot->childList.push_back(tiggerFunc);
        }
    }
    return tiggerRoot;
}

void TiggerFuncDefNode::translateEeyore(EeyoreFuncDefNode *eeyoreFunc) {
    usedParamRegNum = eeyoreFunc->maxParamNum();
    // 可用的寄存器数量
    validRegNum = validRegNum + 8 - usedParamRegNum;
    baseStackTop = 19 + 7 + 8; // 栈底用来保存寄存器 11个s + 7个t + 8个a
    TiggerRootNode::funcParams[eeyoreFunc->funcName] = eeyoreFunc->paramNum;
    int curParamCnt = 0;

    unsigned int localIntVarCnt = 0;
    unsigned int tempCnt = 0;
    unsigned int localVarCnt = 0;

    // 添加信息
    for(int i = 0; i < eeyoreFunc->paramNum; i++) {
        string t = "p" + std::to_string(i);
        string newName = "a" + std::to_string(i);
        eeyoreSymbolToTigger[t] = newName;
        symbolInfo[newName] = TiggerVarInfo();
        symbolInfo[newName].isParam = true;
    }

    int s = eeyoreFunc->basicBlockList.size();
    assert(s >= 2);
    for(auto ptr:eeyoreFunc->basicBlockList[0]->stmtList) {
        assert(ptr->nodeType = EeyoreNodeType::VAR_DECL);

        auto declPtr = static_cast<EeyoreVarDeclNode *>(ptr);

        // 把临时变量当作一般的变量处理
        if(declPtr->name[0] == 't') {
            auto newName = "y" + std::to_string(tempCnt);
            tempCnt++;
            eeyoreSymbolToTigger[declPtr->name] = newName;
            symbolInfo[newName] = TiggerVarInfo();
            symbolInfo[newName].isTempVar = true;
            symbolInfo[newName].isArray = false;
            symbolInfo[newName].isLocal = true;     // 把临时变量当成一个一般通过local试试
            symbolInfo[newName].pos = baseStackTop + localIntVarCnt;
            localIntVarCnt++;
            continue;
        }
        auto eeyoreVarInfo = EeyoreBaseNode::getVarInfo(declPtr->name);

        // 如果是一个局部数组，记录数组信息
        if(eeyoreVarInfo.isArray) {
            string newName = "z" + std::to_string(localVarCnt);
            localVarCnt++;
            eeyoreSymbolToTigger[declPtr->name] = newName;
            symbolInfo[newName] = TiggerVarInfo();
            symbolInfo[newName].isArray = true;
            symbolInfo[newName].isLocal = true;
            symbolInfo[newName].arraySize = eeyoreVarInfo.arraySize;
            symbolInfo[newName].pos = baseStackTop + localIntVarCnt;
            localIntVarCnt += eeyoreVarInfo.arraySize;
            if(eeyoreVarInfo.isInitialized) {
                // 预初始化
                for(int j = symbolInfo[newName].pos; j < baseStackTop + localIntVarCnt; j++)
                    childList.push_back(new TiggerStoreNode("x0", j));
            }
        }
            // 如果是一个局部变量，预分配一个符号寄存器
        else {
            // 分配符号寄存器（非实际寄存器）
            string newName = "z" + std::to_string(localVarCnt);
            localVarCnt++;
            eeyoreSymbolToTigger[declPtr->name] = newName;
            symbolInfo[newName] = TiggerVarInfo();
            symbolInfo[newName].isArray = false;
            symbolInfo[newName].isLocal = true;
            symbolInfo[newName].pos = baseStackTop + localIntVarCnt;
            localIntVarCnt++;
        }
    }

    // 统计循环
    for(int i = 1; i < s - 1; i++) {
        assert(eeyoreFunc->basicBlockList[i]->stmtList.size() == 1);
        auto ptr = eeyoreFunc->basicBlockList[i]->stmtList[0];

        double curCost = 1 + pow(99, eeyoreFunc->basicBlockList[i]->cycleCnt);

        switch(ptr->nodeType) {
            case EeyoreNodeType::ASSIGN: {
                auto assignPtr = static_cast<EeyoreAssignNode *>(ptr);

                // 检查右边
                if(assignPtr->rightTerm->nodeType == EeyoreNodeType::EXP) {
                    auto expPtr = static_cast<EeyoreExpNode *>(assignPtr->rightTerm);
                    assert(symbolInfo.count(eeyoreSymbolToTigger[assignPtr->leftValue->name]) > 0);

                    string leftName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                    auto &leftInfo = symbolInfo[leftName];
                    leftInfo.spillCost += curCost;
                    // 如果是二元表达式
                    if(expPtr->isBinary) {
                        if(!expPtr->firstOperand->isNum())
                            symbolInfo[eeyoreSymbolToTigger[expPtr->firstOperand->name]].spillCost += curCost;
                        if(!expPtr->secondOperand->isNum())
                            symbolInfo[eeyoreSymbolToTigger[expPtr->secondOperand->name]].spillCost += curCost;
                    } else {
                        // 如果是一元表达式，假设操作数不是数字（否则可以直接计算）
                        assert(!expPtr->firstOperand->isNum());
                        if(!expPtr->firstOperand->isNum())
                            symbolInfo[eeyoreSymbolToTigger[expPtr->firstOperand->name]].spillCost += curCost;
                    }
                }
                    //
                else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::RIGHT_VALUE) {
                    // 如果右边是right_value, 即右边是symbol或者num
                    auto rVarPtr = static_cast<EeyoreRightValueNode *>(assignPtr->rightTerm);

                    string leftName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                    auto &leftInfo = symbolInfo[leftName];
                    leftInfo.spillCost += curCost;

                    if(!rVarPtr->isNum())
                        symbolInfo[eeyoreSymbolToTigger[rVarPtr->name]].spillCost += curCost;

                }
                    // 左值
                else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::LEFT_VALUE) {
                    auto lVarPtr = static_cast<EeyoreLeftValueNode *>(assignPtr->rightTerm);
                    // 左边应该一定不是array
                    assert(!assignPtr->leftValue->isArray);

                    string rightName = eeyoreSymbolToTigger[lVarPtr->name];
                    auto &rightInfo = symbolInfo[rightName];
                    rightInfo.spillCost += curCost;
                    // 获取左边的信息
                    string leftName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                    auto &leftInfo = symbolInfo[leftName];
                    leftInfo.spillCost += curCost;

                } else
                    assert(false);
                break;
            }
            case EeyoreNodeType::IF_GOTO: {
                auto ifGotoPtr = static_cast<EeyoreIfGotoNode *>(ptr);
                if(!ifGotoPtr->condRightValue->isNum())
                    symbolInfo[eeyoreSymbolToTigger[ifGotoPtr->condRightValue->name]].spillCost += curCost;
                break;
            }
            case EeyoreNodeType::PARAM: {
                auto paramPtr = static_cast<EeyoreFuncParamNode *>(ptr);

                // 将这个参数的值保存到参数栈中
                assert(curParamCnt < 8);

                if(!paramPtr->param->isNum()) {
                    // 否则，首先读取条件
                    string paramName = eeyoreSymbolToTigger[paramPtr->param->name];
                    auto &paramInfo = symbolInfo[paramName];
                    paramInfo.spillCost += curCost;
                }
                break;
            }
            default:
                break;
        }
    }

    // 登记完信息之后，开始染色
    // 构建冲突图的邻接矩阵
    map<string, map<string, unsigned int >> gMap;
    set<string> nodeSet;
    for(auto name:symbolInfo) {
        if(name.second.isLocal || name.second.isTempVar) {
            gMap[name.first] = map<string, unsigned int>();
            nodeSet.insert(name.first);
        }

    }

    for(int i = 1; i < s - 1; i++) {
        auto inSet = eeyoreFunc->basicBlockList[i]->inSet;
        auto outSet = eeyoreFunc->basicBlockList[i]->outSet;
        for(auto t1:inSet)
            for(auto t2:inSet) {
                assert(eeyoreSymbolToTigger.count(t1) > 0 && eeyoreSymbolToTigger.count(t2) > 0);
                auto n1 = eeyoreSymbolToTigger[t1];
                auto n2 = eeyoreSymbolToTigger[t2];
                if(n1 == n2)
                    continue;
                gMap[n1][n2] = 1;
                nodeSet.insert(n1);
                nodeSet.insert(n2);
            }
        for(auto t1:outSet)
            for(auto t2:outSet) {
                assert(eeyoreSymbolToTigger.count(t1) > 0 && eeyoreSymbolToTigger.count(t2) > 0);
                auto n1 = eeyoreSymbolToTigger[t1];
                auto n2 = eeyoreSymbolToTigger[t2];
                if(n1 == n2)
                    continue;
                gMap[n1][n2] = 1;
                nodeSet.insert(n1);
                nodeSet.insert(n2);
            }
    }


    srand((unsigned int) time(NULL));

    stack<pair<string, vector<string>>> waitStack; // 一个变量和这个变量的邻居
    while(!nodeSet.empty()) {
        bool deleted = false;
        // 先将度小于validRegNum的寄存器从图中删除
        for(auto &t1:nodeSet) {
            int cnt = 0;
            vector<string> t;
            for(auto &t2:nodeSet) {
                if(gMap[t1][t2]) {
                    cnt += 1;
                    t.push_back(t2);
                }
            }
            // 如果计数小于合法的寄存器数量，则入栈
            if(cnt < validRegNum) {
                waitStack.push(pair<string, vector<string>>(t1, t));
                // 删除t1这个节点
                nodeSet.erase(t1);
                deleted = true;
                break;
            }
        }
        // 如果有删除了，则continue
        if(deleted)
            continue;
        // 找whileCnt最小的
        string tar = *(nodeSet.begin());
        double minCost = 3.402823466e+38;
        for(auto t:nodeSet) {
            if(symbolInfo[t].spillCost < minCost) {
                minCost = symbolInfo[t].spillCost;
                tar = t;
            }
        }
        nodeSet.erase(tar);
    }

    for(int i = 0; i < usedParamRegNum; i++)
        inUseReg.insert("a" + std::to_string(i));
    // 进行染色
    while(!waitStack.empty()) {
        auto curTop = waitStack.top();
        waitStack.pop();
        set<int> usedReg;
        // 统计用过的reg
        for(string n: curTop.second) {
            auto info = symbolInfo[n];
            if(info.inReg)
                usedReg.insert(info.regNum);
        }
        for(int i = 0; i < validRegNum; i++) {
            if(usedReg.count(i) == 0) {
                symbolInfo[curTop.first].inReg = true;
                symbolInfo[curTop.first].regNum = i;
                inUseReg.insert(getRegName(i));
                break;
            }
            if(i == validRegNum - 1)
                assert(false);
        }
    }

    // 语句翻译
    for(int i = 1; i < s - 1; i++) {
        assert(eeyoreFunc->basicBlockList[i]->stmtList.size() == 1);
        auto ptr = eeyoreFunc->basicBlockList[i]->stmtList[0];


        switch(ptr->nodeType) {
            case EeyoreNodeType::ASSIGN: {
                auto assignPtr = static_cast<EeyoreAssignNode *>(ptr);
                auto tInfo = symbolInfo[eeyoreSymbolToTigger[assignPtr->leftValue->name]];
                if(!assignPtr->leftValue->isArray &&
                   !tInfo.isGlobal && !tInfo.isParam &&
                   eeyoreFunc->basicBlockList[i]->outSet.count(assignPtr->leftValue->name) == 0)
                    break;

                childList.push_back(new TiggerCommentNode("// " + assignPtr->to_eeyore_string()));

                // 检查右边
                if(assignPtr->rightTerm->nodeType == EeyoreNodeType::EXP) {
                    auto expPtr = static_cast<EeyoreExpNode *>(assignPtr->rightTerm);
                    // exp左边的可能是一个全局非数组变量或者一个局部非数组变量或者一个临时变量
                    // 如果是全局非数组变量，则需要读出变量地址
                    assert(symbolInfo.count(eeyoreSymbolToTigger[assignPtr->leftValue->name]) > 0);
                    string leftName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                    auto &leftInfo = symbolInfo[leftName];
                    string leftReg = "s0";
                    assert(!leftInfo.isArray);
                    if(leftInfo.isParam)
                        leftReg = leftName;
                    // 如果左边是一个在reg中的局部变量
                    if(leftInfo.isLocal && leftInfo.inReg)
                        leftReg = getRegName(leftInfo.regNum);

                    // 如果是二元表达式
                    if(expPtr->isBinary) {
                        string op1Reg = "s0", op2Reg = "t0";

                        setRightValueReg(expPtr->firstOperand, op1Reg);

                        // 特判右边
                        if(expPtr->secondOperand->isNum()) {
                            int value = expPtr->secondOperand->getValue();
                            if(expPtr->type == OpType::opPlus && value >= -2048 && value <= 2047)
                                childList.push_back(new TiggerAssignNode(OpType::opPlus, leftReg, op1Reg, value));
                            else if(expPtr->type == OpType::opDec && value >= -2047 && value <= 2048)
                                childList.push_back(new TiggerAssignNode(OpType::opPlus, leftReg, op1Reg, -value));
                            else if(expPtr->type == OpType::opMul && value == 4)
                                childList.push_back(new TiggerAssignNode(OpType::opMul, leftReg, op1Reg, value));
                            else if(expPtr->type == OpType::opMul && value == 2)
                                childList.push_back(new TiggerAssignNode(OpType::opMul, leftReg, op1Reg, value));
                            else if(expPtr->type == OpType::opDiv && value == 2)
                                childList.push_back(new TiggerAssignNode(OpType::opDiv, leftReg, op1Reg, value));
                            else if(expPtr->type == OpType::opMod && value == 2)
                                childList.push_back(new TiggerAssignNode(OpType::opMod, leftReg, op1Reg, value));
                            else if(expPtr->type == OpType::opE &&
                                    eeyoreFunc->basicBlockList[i + 1]->stmtList[0]->nodeType ==
                                    EeyoreNodeType::IF_GOTO &&
                                    value == 0 &&
                                    !static_cast<EeyoreIfGotoNode *> (eeyoreFunc->basicBlockList[i +
                                                                                                 1]->stmtList[0])->condRightValue->isNum() &&
                                    eeyoreSymbolToTigger[static_cast<EeyoreIfGotoNode *> (eeyoreFunc->basicBlockList[i +
                                                                                                                     1]->stmtList[0])->condRightValue->name] ==
                                    leftName) {

                                auto ifGotoPtr = static_cast<EeyoreIfGotoNode *>
                                (eeyoreFunc->basicBlockList[i + 1]->stmtList[0]);
                                if(ifGotoPtr->isEq) {
                                    childList.push_back(
                                            new TiggerExpIfGotoNode(ifGotoPtr->label, OpType::opNE, op1Reg,
                                                                    "x0"));
                                } else
                                    childList.push_back(
                                            new TiggerExpIfGotoNode(ifGotoPtr->label, OpType::opE, op1Reg,
                                                                    "x0"));
                                i++;
                                break;
                            } else {
                                setRightValueReg(expPtr->secondOperand, op2Reg);
                                childList.push_back(new TiggerAssignNode(expPtr->type, leftReg, op1Reg, op2Reg));
                            }
                        } else {
                            setRightValueReg(expPtr->secondOperand, op2Reg);
                            // 如果下一句是ifgoto，且当前语句的操作是比较运算
                            if(eeyoreFunc->basicBlockList[i + 1]->stmtList[0]->nodeType == EeyoreNodeType::IF_GOTO &&
                               (expPtr->type == OpType::opE || expPtr->type == OpType::opNE ||
                                expPtr->type == OpType::opG || expPtr->type == OpType::opGE ||
                                expPtr->type == OpType::opL || expPtr->type == OpType::opLE)) {
                                auto ifGotoPtr = static_cast<EeyoreIfGotoNode *>
                                (eeyoreFunc->basicBlockList[i + 1]->stmtList[0]);
                                // 不是数字，且用于比较的东西是leftname
                                if(!ifGotoPtr->condRightValue->isNum() &&
                                   eeyoreSymbolToTigger[ifGotoPtr->condRightValue->name] == leftName) {
                                    if(ifGotoPtr->isEq) {
                                        switch(expPtr->type) {
                                            case OpType::opE:
                                                childList.push_back(
                                                        new TiggerExpIfGotoNode(ifGotoPtr->label, OpType::opNE, op1Reg,
                                                                                op2Reg));
                                                break;
                                            case OpType::opNE:
                                                childList.push_back(
                                                        new TiggerExpIfGotoNode(ifGotoPtr->label, OpType::opE, op1Reg,
                                                                                op2Reg));
                                                break;
                                            case OpType::opGE:
                                                childList.push_back(
                                                        new TiggerExpIfGotoNode(ifGotoPtr->label, OpType::opL, op1Reg,
                                                                                op2Reg));
                                                break;
                                            case OpType::opLE:
                                                childList.push_back(
                                                        new TiggerExpIfGotoNode(ifGotoPtr->label, OpType::opG, op1Reg,
                                                                                op2Reg));
                                                break;
                                            case OpType::opG:
                                                childList.push_back(
                                                        new TiggerExpIfGotoNode(ifGotoPtr->label, OpType::opLE, op1Reg,
                                                                                op2Reg));
                                                break;
                                            case OpType::opL:
                                                childList.push_back(
                                                        new TiggerExpIfGotoNode(ifGotoPtr->label, OpType::opGE, op1Reg,
                                                                                op2Reg));
                                                break;
                                            default:
                                                assert(false);
                                        }
                                    } else {
                                        childList.push_back(
                                                new TiggerExpIfGotoNode(ifGotoPtr->label, expPtr->type, op1Reg,
                                                                        op2Reg));
                                    }
                                    i++;
                                    break;
                                }
                            }
                            childList.push_back(new TiggerAssignNode(expPtr->type, leftReg, op1Reg, op2Reg));
                        }


                    } else {
                        // 如果是一元表达式，假设操作数不是数字（否则可以直接计算）
                        assert(!expPtr->firstOperand->isNum());
                        string op1Reg = "s0";
                        setRightValueReg(expPtr->firstOperand, op1Reg);
                        childList.push_back(new TiggerAssignNode(expPtr->type, leftReg, op1Reg));
                    }

                    // 如果左边是全局或者局部变量
                    if(leftInfo.isGlobal) {
                        childList.push_back(new TiggerLoadAddrNode(leftName, "t0"));
                        childList.push_back(new TiggerAssignNode("t0", 0, leftReg));
                    } else if(leftInfo.isLocal && !leftInfo.inReg) {
                        childList.push_back(new TiggerStoreNode(leftReg, leftInfo.pos));
                    }

                    // 表达式部分计算完成
                }
                    //
                else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::RIGHT_VALUE) {
                    // 如果右边是right_value, 即右边是symbol或者num
                    auto rVarPtr = static_cast<EeyoreRightValueNode *>(assignPtr->rightTerm);

                    // s5[s6] = s7

                    string leftReg = "s0";
                    // 左边可能是一个符号或者一个数组
                    // 如果是数组，计算出对应元素的位置为s0, 然后放到0(s0)处
                    // 如果是符号，按照是否在寄存器中分类讨论
                    string leftName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                    auto &leftInfo = symbolInfo[leftName];
                    if(assignPtr->leftValue->isArray) {
                        // 如果左边是数组
                        // 首先获取数组参数
                        string tReg = "t0";
                        setRightValueReg(assignPtr->leftValue->rValNodePtr, tReg);
                        if(leftInfo.isGlobal) {
                            childList.push_back(new TiggerLoadAddrNode(leftName, "s0"));
                            childList.push_back(new TiggerAssignNode(OpType::opPlus, "s0", "s0", tReg));
                        } else if(leftInfo.isLocal) {
                            childList.push_back(new TiggerLoadAddrNode(leftInfo.pos, "s0"));
                            childList.push_back(new TiggerAssignNode(OpType::opPlus, "s0", "s0", tReg));
                        } else if(leftInfo.isParam) {
                            childList.push_back(new TiggerAssignNode(OpType::opPlus, "s0", leftName, tReg));
                        }
                    } else {
                        // 如果左边是符号
                        string leftName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                        auto &leftInfo = symbolInfo[leftName];

                        leftReg = "sx";
                        if(leftInfo.isParam)
                            leftReg = leftName;
                        if(leftInfo.isLocal && leftInfo.inReg)
                            leftReg = getRegName(leftInfo.regNum);
                    }

                    // 右边是t0
                    string rightReg = "t0";
                    setRightValueReg(rVarPtr, rightReg);

                    // 赋值
                    if(assignPtr->leftValue->isArray) {
                        childList.push_back(new TiggerAssignNode("s0", 0, rightReg));
                    } else {
                        // 如果左边在寄存器中
                        if(leftReg != "sx") {
                            childList.push_back(new TiggerAssignNode(leftReg, rightReg));
                        } else {
                            // 如果不在寄存器中，直接保存右值
                            if(leftInfo.isGlobal) {
                                childList.push_back(new TiggerLoadAddrNode(leftName, "s0"));
                                childList.push_back(new TiggerAssignNode("s0", 0, rightReg));
                            } else if(leftInfo.isLocal) {
                                childList.push_back(new TiggerStoreNode(rightReg, leftInfo.pos));
                            }
                        }
                    }
                }
                    // 左值
                else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::LEFT_VALUE) {
                    auto lVarPtr = static_cast<EeyoreLeftValueNode *>(assignPtr->rightTerm);
                    // 左边应该一定不是array
                    assert(!assignPtr->leftValue->isArray);


                    // 分析右边
                    string rightReg = "t0";
                    // 右边可能是一个符号或者一个数组
                    // 如果是数组，计算出对应元素的位置为t0, 然后放到0(t0)处
                    // 如果是符号，按照是否在寄存器中分类讨论
                    if(lVarPtr->isArray) {
                        // 如果右边是数组
                        // 首先获取数组参数
                        string tReg = "s0";
                        setRightValueReg(lVarPtr->rValNodePtr, tReg);

                        string rightName = eeyoreSymbolToTigger[lVarPtr->name];
                        auto &rightInfo = symbolInfo[rightName];
                        if(rightInfo.isGlobal) {
                            childList.push_back(new TiggerLoadAddrNode(rightName, "t0"));
                            childList.push_back(new TiggerAssignNode(OpType::opPlus, "t0", "t0", tReg));
                        } else if(rightInfo.isLocal) {
                            childList.push_back(new TiggerLoadAddrNode(rightInfo.pos, "t0"));
                            childList.push_back(new TiggerAssignNode(OpType::opPlus, "t0", "t0", tReg));
                        } else if(rightInfo.isParam) {
                            childList.push_back(new TiggerAssignNode(OpType::opPlus, "t0", rightName, tReg));
                        }
                    } else {
                        // 如果右边是符号
                        string rightName = eeyoreSymbolToTigger[lVarPtr->name];
                        auto &rightInfo = symbolInfo[rightName];

                        if(rightInfo.isParam)
                            rightReg = rightName;
                        else if(rightInfo.isLocal && rightInfo.inReg)
                            rightReg = getRegName(rightInfo.regNum);
                        else if(rightInfo.isLocal && !rightInfo.inReg)
                            childList.push_back(new TiggerLoadNode(rightInfo.pos, "t0"));
                        else if(rightInfo.isGlobal)
                            childList.push_back(new TiggerLoadNode(rightName, "t0"));
                    }

                    // 获取左边的信息
                    string leftName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                    auto &leftInfo = symbolInfo[leftName];
                    if(leftInfo.isGlobal) {
                        childList.push_back(new TiggerLoadAddrNode(leftName, "s0"));
                        if(lVarPtr->isArray) {
                            childList.push_back(new TiggerAssignNode("t0", "t0", 0));
                            childList.push_back(new TiggerAssignNode("s0", 0, "t0"));
                        } else {
                            childList.push_back(new TiggerAssignNode("s0", 0, rightReg));
                        }
                    } else if(leftInfo.isParam) {
                        if(lVarPtr->isArray) {
                            childList.push_back(new TiggerAssignNode(leftName, "t0", 0));
                        } else {
                            childList.push_back(new TiggerAssignNode(leftName, rightReg));
                        }
                    } else if(leftInfo.isLocal && leftInfo.inReg) {
                        if(lVarPtr->isArray) {
                            childList.push_back(new TiggerAssignNode(getRegName(leftInfo.regNum), "t0", 0));
                        } else {
                            childList.push_back(new TiggerAssignNode(getRegName(leftInfo.regNum), rightReg));
                        }
                    } else if(leftInfo.isLocal && !leftInfo.inReg) {
                        childList.push_back(new TiggerLoadAddrNode(leftInfo.pos, "s0"));
                        if(lVarPtr->isArray) {
                            childList.push_back(new TiggerAssignNode("t0", "t0", 0));
                            childList.push_back(new TiggerAssignNode("s0", 0, "t0"));
                        } else {
                            childList.push_back(new TiggerAssignNode("s0", 0, rightReg));
                        }
                    } else
                        assert(false);
                } else
                    assert(false);
                break;
            }
            case EeyoreNodeType::LABEL: {
                auto labelPtr = static_cast<EeyoreLabelNode *>(ptr);
                childList.push_back(new TiggerLabelNode(labelPtr->label));
                break;
            }
            case EeyoreNodeType::GOTO: {
                childList.push_back(new TiggerCommentNode("// " + ptr->to_eeyore_string()));
                auto gotoPtr = static_cast<EeyoreGotoNode *>(ptr);
                childList.push_back(new TiggerGotoNode(gotoPtr->label));
                break;
            }
            case EeyoreNodeType::IF_GOTO: {
                childList.push_back(new TiggerCommentNode("// " + ptr->to_eeyore_string()));
                auto ifGotoPtr = static_cast<EeyoreIfGotoNode *>(ptr);

                string tReg = "s0";
                setRightValueReg(ifGotoPtr->condRightValue, tReg);
                childList.push_back(new TiggerIfGotoNode(ifGotoPtr->isEq, tReg, ifGotoPtr->label));
                break;
            }
            case EeyoreNodeType::PARAM: {
                auto paramPtr = static_cast<EeyoreFuncParamNode *>(ptr);

                if(curParamCnt == 0) {
                    childList.push_back(new TiggerCommentNode("// save 'a' && 't' regs"));
                    for(int j = 0; j < 8; j++) {
                        if(inUseReg.count("a" + std::to_string(j)) > 0)
                            childList.push_back(new TiggerStoreNode("a" + std::to_string(j), 19 + j));
                    }
                    for(int j = 0; j < 7; j++) {
                        if(inUseReg.count("t" + std::to_string(j)) > 0)
                            childList.push_back(new TiggerStoreNode("t" + std::to_string(j), 12 + j));
                    }
                }

                // 将这个参数的值保存到参数栈中
                assert(curParamCnt < 8);

                if(paramPtr->param->isNum()) {
                    childList.push_back(
                            new TiggerAssignNode("a" + std::to_string(curParamCnt), paramPtr->param->getValue()));
                } else {
                    // 否则，首先读取条件
                    string paramName = eeyoreSymbolToTigger[paramPtr->param->name];
                    auto &paramInfo = symbolInfo[paramName];
                    if(paramInfo.isParam) {
                        // 注意此时的参数顺序
                        int num = paramName[1] - '0';
                        // 如果这个函数参数已经被重新写过
                        if(num < curParamCnt) {
                            childList.push_back(new TiggerLoadNode(19 + num, "a" + std::to_string(curParamCnt)));
                        } else if(num > curParamCnt)
                            childList.push_back(
                                    new TiggerAssignNode("a" + std::to_string(curParamCnt), "a" + std::to_string(num)));
                    } else if(paramInfo.isGlobal) {
                        if(paramInfo.isArray) {
                            childList.push_back(new TiggerLoadAddrNode(paramName, "a" + std::to_string(curParamCnt)));
                        } else {
                            childList.push_back(new TiggerLoadNode(paramName, "a" + std::to_string(curParamCnt)));
                        }
                    } else {
                        assert(paramInfo.isLocal);
                        if(paramInfo.isArray) {
                            childList.push_back(
                                    new TiggerLoadAddrNode(paramInfo.pos, "a" + std::to_string(curParamCnt)));
                        } else {
                            if(paramInfo.inReg) {
                                childList.push_back(new TiggerAssignNode(
                                        "a" + std::to_string(curParamCnt), getRegName(paramInfo.regNum)));
                            } else {
                                childList.push_back(
                                        new TiggerLoadNode(paramInfo.pos, "a" + std::to_string(curParamCnt)));
                            }
                        }
                    }
                }
                curParamCnt++;
                break;
            }
            case EeyoreNodeType::FUNC_CALL: {
                auto funcCallPtr = static_cast<EeyoreFuncCallNode *>(ptr);
                childList.push_back(new TiggerCommentNode("// " + ptr->to_eeyore_string()));

                if(curParamCnt == 0) {
                    childList.push_back(new TiggerCommentNode("// save 'a' && 't' regs"));
                    for(int j = 0; j < 8; j++) {
                        if(inUseReg.count("a" + std::to_string(j)) > 0)
                            childList.push_back(new TiggerStoreNode("a" + std::to_string(j), 19 + j));
                    }
                    for(int j = 0; j < 7; j++) {
                        if(inUseReg.count("t" + std::to_string(j)) > 0)
                            childList.push_back(new TiggerStoreNode("t" + std::to_string(j), 12 + j));
                    }
                }

                int neededParamNum = TiggerRootNode::funcParams[funcCallPtr->name];
                assert(curParamCnt == neededParamNum);
                curParamCnt = 0;

                // 调用函数
                childList.push_back(new TiggerFuncCallNode(funcCallPtr->name));
                // 如果有返回值，要把a0保存到目标寄存器
                if(funcCallPtr->hasReturnValue) {
                    string leftName = eeyoreSymbolToTigger[funcCallPtr->returnSymbol];
                    auto &leftInfo = symbolInfo[leftName];
                    string leftReg = "s0";
                    if(leftInfo.isParam)
                        leftReg = leftName;
                    else if(leftInfo.isLocal && leftInfo.inReg)
                        leftReg = getRegName(leftInfo.regNum);

                    // 如果左边是全局或者局部变量
                    if(leftInfo.isGlobal) {
                        childList.push_back(new TiggerLoadAddrNode(leftName, "t0"));
                        childList.push_back(new TiggerAssignNode("t0", 0, "a0"));
                    } else if(leftInfo.isLocal && !leftInfo.inReg) {
                        childList.push_back(new TiggerStoreNode("a0", leftInfo.pos));
                    } else {
                        childList.push_back(new TiggerAssignNode(leftReg, "a0"));
                    }

                }
                // 恢复a和t
                childList.push_back(new TiggerCommentNode("// load 't' and 'a' regs"));
                for(int j = 0; j < 7; j++) {
                    if(inUseReg.count("t" + std::to_string(j)) > 0)
                        childList.push_back(new TiggerLoadNode(12 + j, "t" + std::to_string(j)));
                }
                for(int j = 0; j < 8; j++) {
                    if(inUseReg.count("a" + std::to_string(j)) > 0)
                        childList.push_back(new TiggerLoadNode(19 + j, "a" + std::to_string(j)));
                }

                break;
            }
            case EeyoreNodeType::RETURN: {
                auto returnPtr = static_cast<EeyoreReturnNode *>(ptr);
                childList.push_back(new TiggerCommentNode("// " + ptr->to_eeyore_string()));
                // 恢复寄存器在generate中完成
                childList.push_back(new TiggerCommentNode("// save global vars"));

                // 处理返回值
                if(returnPtr->hasReturnValue) {
                    string tReg = "s0";
                    setRightValueReg(returnPtr->returnValuePtr, tReg);
                    childList.push_back(new TiggerAssignNode("a0", tReg));
                }
                // 恢复
                for(int i = 0; i < 12; i++) {
                    if(inUseReg.count("s" + std::to_string(i)) > 0)
                        childList.push_back(new TiggerLoadNode(i, "s" + std::to_string(i)));
                }
                childList.push_back(new TiggerReturnNode());
                break;
            }

                // 不存在的东西
            case EeyoreNodeType::COMMENT:
            case EeyoreNodeType::FILL_INIT:
            case EeyoreNodeType::GLOBAL_INIT:
            case EeyoreNodeType::BLOCK_END:
            case EeyoreNodeType::BLOCK_BEGIN:
            case EeyoreNodeType::VAR_DECL:
                assert(false);
        }

    }

    paramNum = eeyoreFunc->paramNum;
    stackSize = baseStackTop + localIntVarCnt;
    funcName = eeyoreFunc->funcName;

    for(auto ptr:childList)
        if(ptr->nodeType == TiggerNodeType::RETURN)
            static_cast<TiggerReturnNode *>(ptr)->stackSize = stackSize;

}

void TiggerFuncDefNode::setRightValueReg(EeyoreRightValueNode *rVal, string &rightReg) {
    string defaultReg = rightReg;
    if(rVal->isNum()) {
        childList.push_back(new TiggerAssignNode(defaultReg, rVal->getValue()));
    } else {
        auto rValName = eeyoreSymbolToTigger[rVal->name];
        auto rValInfo = symbolInfo[rValName];
        if(rValInfo.isGlobal) {
            if(rValInfo.isArray)
                childList.push_back(new TiggerLoadAddrNode(rValName, defaultReg));
            else
                childList.push_back(new TiggerLoadNode(rValName, defaultReg));
        } else if(rValInfo.isLocal) {
            if(rValInfo.isArray)
                childList.push_back(new TiggerLoadAddrNode(rValInfo.pos, defaultReg));
            else {
                if(rValInfo.inReg) {
                    rightReg = getRegName(rValInfo.regNum);
                } else {
                    childList.push_back(new TiggerLoadNode(rValInfo.pos, defaultReg));
                }
            }
        } else {
            assert(rValInfo.isParam);
            rightReg = rValName;
        }
    }
}


unsigned int EeyoreFuncDefNode::maxParamNum() {
    unsigned int rst = paramNum;
    for(auto ptr:childList)
        if(ptr->nodeType == EeyoreNodeType::FUNC_CALL)
            rst = rst > static_cast<EeyoreFuncCallNode *>(ptr)->paramList.size()
                  ? rst
                  : static_cast<EeyoreFuncCallNode *>(ptr)->paramList.size();
    assert(rst <= 8);
    return rst;
}
