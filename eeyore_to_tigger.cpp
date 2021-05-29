//
// Created by KiNAmi on 2021/5/17.
//
#include "eeyore_ast.hpp"
#include "tigger.hpp"
#include <stack>
#include <set>

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
    baseStackTop = 19 + 8 + usedParamRegNum; // 栈底用来保存寄存器 11个s + 8个t + 8个参数栈 若干个a
    TiggerRootNode::funcParams[eeyoreFunc->funcName] = eeyoreFunc->paramNum;
    // 这里进行了一次优化
//    eeyoreFunc->simplifyTempVar();
    int curParamCnt = 0;
    string tempParamRegs[8] = {"t1", "t2", "t3", "t4", "t5", "t6", "s10", "s9"};

    int regUsedCnt = 0;
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
        // 临时变量直接分配寄存器，应该只有3个
        if(declPtr->name[0] == 't') {
            tempCnt++;
            assert(tempCnt <= 3);
            auto newName = "s" + std::to_string(tempCnt);
            eeyoreSymbolToTigger[declPtr->name] = newName;
            symbolInfo[newName] = TiggerVarInfo();
            symbolInfo[newName].isTempVar = true;
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
            // 预初始化
            for(int j = symbolInfo[newName].pos; j < baseStackTop + localIntVarCnt; j++)
                childList.push_back(new TiggerStoreNode("x0", j));
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

    // 登记完信息之后，开始染色
    // 构建冲突图的邻接矩阵
    map<string, map<string, unsigned int >> gMap;
    set<string> nodeSet;
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
//    generateConflictGraphviz(gMap, eeyoreFunc->funcName);
    // 分配寄存器
    auto tempMap = gMap;
    stack<pair<string, vector<string>>> waitStack; // 一个变量和这个变量的邻居
    bool isDone = false;
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
        // 如果循环完了没有删除的，则随便删除一个
        nodeSet.erase(nodeSet.begin());
    }

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
                break;
            }
            if(i == validRegNum - 1)
                assert(false);
        }
    }

    for(int i = 1; i < s - 1; i++) {
        assert(eeyoreFunc->basicBlockList[i]->stmtList.size() == 1);
        auto ptr = eeyoreFunc->basicBlockList[i]->stmtList[0];

        switch(ptr->nodeType) {
            case EeyoreNodeType::ASSIGN: {
                auto assignPtr = static_cast<EeyoreAssignNode *>(ptr);
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
                    if(leftInfo.isParam || leftInfo.isTempVar)
                        leftReg = leftName;

                    // 如果左边是一个在reg中的局部变量
                    if(leftInfo.isLocal && leftInfo.inReg)
                        leftReg = getRegName(leftInfo.regNum);

                    // 如果是二元表达式
                    if(expPtr->isBinary) {
                        string op1Reg = "s0", op2Reg = "t0";

                        setRightValueReg(expPtr->firstOperand, op1Reg);
                        setRightValueReg(expPtr->secondOperand, op2Reg);

                        childList.push_back(new TiggerAssignNode(expPtr->type, leftReg, op1Reg, op2Reg));

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
                        if(leftInfo.isParam || leftInfo.isTempVar)
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

                        if(rightInfo.isParam || rightInfo.isTempVar)
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
                    } else if(leftInfo.isParam || leftInfo.isTempVar) {
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

                // 将这个参数的值保存到参数栈中
                assert(curParamCnt < 8);

                if(paramPtr->param->isNum()) {
                    childList.push_back(new TiggerAssignNode("t0", paramPtr->param->getValue()));
                    childList.push_back(new TiggerStoreNode("t0", 19 + curParamCnt));
                } else {
                    // 否则，首先读取条件
                    string paramName = eeyoreSymbolToTigger[paramPtr->param->name];
                    auto &paramInfo = symbolInfo[paramName];
                    if(paramInfo.isParam) {
                        int num = paramName[1] - '0';
                        childList.push_back(new TiggerStoreNode(paramName, 19 + curParamCnt));
                    } else if(paramInfo.isTempVar)
                        childList.push_back(new TiggerStoreNode(paramName, 19 + curParamCnt));
                    else if(paramInfo.isGlobal) {
                        if(paramInfo.isArray) {
                            childList.push_back(new TiggerLoadAddrNode(paramName, "t0"));
                            childList.push_back(new TiggerStoreNode("t0", 19 + curParamCnt));
                        } else {
                            childList.push_back(new TiggerLoadNode(paramName, "t0"));
                            childList.push_back(new TiggerStoreNode("t0", 19 + curParamCnt));
                        }
                    } else {
                        assert(paramInfo.isLocal);
                        if(paramInfo.isArray) {
                            childList.push_back(new TiggerLoadAddrNode(paramInfo.pos, "t0"));
                            childList.push_back(new TiggerStoreNode("t0", 19 + curParamCnt));
                        } else {
                            if(paramInfo.inReg) {
                                childList.push_back(
                                        new TiggerStoreNode(getRegName(paramInfo.regNum), 19 + curParamCnt));
                            } else {
                                childList.push_back(new TiggerLoadNode(paramInfo.pos, "t0"));
                                childList.push_back(new TiggerStoreNode("t0", 19 + curParamCnt));
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
                childList.push_back(new TiggerCommentNode("// save 'a' && 't' regs"));
                for(int j = 0; j < usedParamRegNum; j++) {
                    childList.push_back(new TiggerStoreNode("a" + std::to_string(j), 27 + j));
                }
                for(int j = 0; j < 7; j++) {
                    childList.push_back(new TiggerStoreNode("t" + std::to_string(j), 12 + j));
                }
                childList.push_back(new TiggerCommentNode("// save global vars"));

                // 塞入参数
                int neededParamNum = TiggerRootNode::funcParams[funcCallPtr->name];
                assert(curParamCnt >= neededParamNum);

                // 从栈中读出参数
                for(int j = curParamCnt - neededParamNum; j < curParamCnt; j++) {
                    childList.push_back(
                            new TiggerLoadNode(19 + j, "a" + std::to_string(j + neededParamNum - curParamCnt)));
                }

                // 计数减少
                curParamCnt -= neededParamNum;

                // 调用函数
                childList.push_back(new TiggerFuncCallNode(funcCallPtr->name));
                // 如果有返回值，要把a0保存到目标寄存器
                if(funcCallPtr->hasReturnValue) {
                    string leftName = eeyoreSymbolToTigger[funcCallPtr->returnSymbol];
                    auto &leftInfo = symbolInfo[leftName];
                    string leftReg = "s0";
                    if(leftInfo.isParam || leftInfo.isTempVar)
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
                    childList.push_back(new TiggerLoadNode(12 + j, "t" + std::to_string(j)));
                }
                for(int j = 0; j < usedParamRegNum; j++) {
                    childList.push_back(new TiggerLoadNode(27 + j, "a" + std::to_string(j)));
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
            assert(rValInfo.isTempVar || rValInfo.isParam);
            rightReg = rValName;
        }
    }
}


pair<string, TiggerBaseNode *> TiggerFuncDefNode::symbolToReg(const string &symbol, bool is_t0) {
    assert(eeyoreSymbolToTigger.count(symbol) > 0);
    auto tiggerName = eeyoreSymbolToTigger[symbol];
    assert(symbolInfo.count(tiggerName) > 0);
    auto &info = symbolInfo[tiggerName];

    TiggerBaseNode *rstPtr = nullptr;
    string rst = "";

    if(info.isGlobal) {
        // 如果是全局数组，将数组地址放到t0
        if(info.isArray) {
            rst = is_t0 ? "t0" : "s0";
            rstPtr = new TiggerLoadAddrNode(tiggerName, rst);

        }
            // 如果是全局变量，将变量值放到t0
        else {
            rst = is_t0 ? "t0" : "s0";
            rstPtr = new TiggerLoadNode(tiggerName, rst);

        }
    } else if(info.isParam || info.isTempVar) {
        rst = tiggerName;
    } else {
        // 如果是数组，返回这个数组的地址
        if(info.isArray) {
            rst = is_t0 ? "t0" : "s0";
            rstPtr = new TiggerLoadAddrNode(info.pos, rst);
        } else
            rst = tiggerName;
    }

    return pair<string, TiggerBaseNode *>(rst, rstPtr);
}

pair<string, vector<TiggerBaseNode *>> TiggerFuncDefNode::getSymbolReg(const string &tiggerName, bool needValue) {
    assert(symbolInfo.count(tiggerName) > 0);
    auto &info = symbolInfo[tiggerName];

    vector<TiggerBaseNode *> rstList;
    string rst;

    // 函数参数和临时变量的名字都直接分配了，直接使用即可
    if(info.isParam || info.isTempVar) {
        rst = tiggerName;
        return pair<string, vector<TiggerBaseNode *>>(rst, rstList);
    }

    // 访问时设置inUse状态
    info.inUse = true;
    // 如果这个变量在寄存器中，直接返回寄存器名
    if(info.inReg) {
        rst = getRegName(info.regNum);
        return pair<string, vector<TiggerBaseNode *>>(rst, rstList);
    }

    // 如果这个变量不在寄存器中，分类讨论
    // 首先申请一个寄存器
    auto reg = simpleAllocateReg(tiggerName);
    rstList.insert(rstList.end(), reg.second.begin(), reg.second.end());
    info.inReg = true;
    info.regNum = reg.first;
    rst = getRegName(info.regNum);

    if(info.isGlobal) {
        if(info.isArray) {
            // 如果是一个全局数组
            // 全局数组只需要把数组地址加载出来，不需要读内容
            rstList.push_back(new TiggerLoadAddrNode(tiggerName, rst));
        } else {
            // 如果是全局变量，且需要值，则将值读入
            if(needValue)
                rstList.push_back(new TiggerLoadNode(tiggerName, rst));
        }
    } else {
        // 应该是局部变量
        assert(info.isLocal);
        // 如果是数组，存入这个数组的地址
        if(info.isArray) {
            rstList.push_back(new TiggerLoadAddrNode(info.pos, rst));
        } else {
            // 否则，如果需要数据则写入
            if(needValue)
                rstList.push_back(new TiggerLoadNode(info.pos, rst));
        }
    }

    return pair<string, vector<TiggerBaseNode *>>(rst, rstList);
}

int TiggerFuncDefNode::currentPos = 0;

pair<int, vector<TiggerBaseNode *>> TiggerFuncDefNode::simpleAllocateReg(const string &tiggerName) {
    assert(symbolInfo.count(tiggerName) > 0);
    auto &info = symbolInfo[tiggerName];
    assert(info.inReg == false);
    vector<TiggerBaseNode *> rstList;
    int reg = -1;
    // 第一轮，如果找到有空的就分配
    for(int i = 0; i < validRegNum; i++)
        if(!regUse[i]) {
            reg = i;
            regUse[i] = true;
            regUseName[i] = tiggerName;
            break;
        }
    // 如果找到了直接返回
    if(reg != -1)
        return pair<int, vector<TiggerBaseNode *>>(reg, rstList);
    // 如果没找到，需要选择一个排除
    // 从currentPos开始遍历
    while(reg == -1) {
        auto userName = regUseName[currentPos];
        auto &userInfo = symbolInfo[userName];
        // 第二次机会
        if(userInfo.inUse) {
            userInfo.inUse = false;
        } else {
            // 选择这个换出
            reg = currentPos;
            // 如果这个寄存器被写过，则需要保存内容
            if(userInfo.isWrited) {
                // 一定不是数组
                assert(!userInfo.isArray);
                if(userInfo.isGlobal) {
                    // 如果是全局变量
                    // 将地址放到t0，然后t0[0] = 当前值
                    rstList.push_back(new TiggerLoadAddrNode(tiggerName, "t0"));
                    rstList.push_back(new TiggerAssignNode("t0", 0, getRegName(reg)));
                } else if(userInfo.isLocal) {
                    // 如果是局部变量
                    rstList.push_back(new TiggerStoreNode(getRegName(reg), userInfo.pos));
                }
            }
            // 设置原来的变量的状态为不在寄存器中
            userInfo.inReg = false;
            // 此时已经保存了内容，可以使用这个寄存器
            regUseName[currentPos] = tiggerName;
        }
        currentPos = (currentPos + 1) % validRegNum;
    }
    return pair<int, vector<TiggerBaseNode *>>(reg, rstList);
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
