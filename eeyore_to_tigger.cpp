//
// Created by KiNAmi on 2021/5/17.
//
#include "eeyore_ast.hpp"
#include "tigger.hpp"

map<string, TiggerVarInfo> TiggerRootNode::tiggerGlobalSymbol;
map<string, string> TiggerRootNode::eeyoreSymbolToTigger;


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
    validRegNum = validRegNum + 8 - usedParamRegNum;
    baseStackTop = 19 + usedParamRegNum; // 栈底用来保存寄存器 11个s + 8个t + 若干个a
    eeyoreFunc->simplifyTempVar();
    int curParamCnt = 0;
    int regUsedCnt = 0;
    unsigned int localIntVarCnt = 0;
    unsigned int tempCnt = 0;
    unsigned int localVarCnt = 0;

    for(int i = 0; i < eeyoreFunc->paramNum; i++) {
        string t = "p" + std::to_string(i);
        string newName = "a" + std::to_string(i);
        eeyoreSymbolToTigger[t] = newName;
        symbolInfo[newName] = TiggerVarInfo();
        symbolInfo[newName].isParam = true;

    }
    int s = eeyoreFunc->childList.size();
    for(int i = 0; i < s; i++) {
        auto ptr = eeyoreFunc->childList[i];
        switch(ptr->nodeType) {
            case EeyoreNodeType::VAR_DECL: {
                auto declPtr = static_cast<EeyoreVarDeclNode *>(ptr);
                // 临时变量直接分配寄存器，应该只有3个
                if(declPtr->name[0] == 't') {
                    tempCnt++;
                    assert(tempCnt <= 3);
                    auto newName = "s" + std::to_string(tempCnt);
                    eeyoreSymbolToTigger[declPtr->name] = newName;
                    symbolInfo[newName] = TiggerVarInfo();
                    symbolInfo[newName].isTempVar = true;
                    break;
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
                break;
            }
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

                    // 如果是二元表达式
                    if(expPtr->isBinary) {
                        // 如果左边的操作数是数字，右边的操作数应该一定不是数字，否则应该在之前已经计算完
                        if(expPtr->firstOperand->isNum()) {
                            assert(!expPtr->secondOperand->isNum());
                            // 将数字装载到s0中
                            childList.push_back(new TiggerAssignNode("s0", expPtr->firstOperand->getValue()));
                            // 右边一定是一个符号
                            assert(eeyoreSymbolToTigger.count(expPtr->secondOperand->name) > 0);
                            // 获取右边符号对应的寄存器信息
                            auto regInfo = getSymbolReg(eeyoreSymbolToTigger[expPtr->secondOperand->name], true);
                            childList.insert(childList.end(), regInfo.second.begin(), regInfo.second.end());

                            // 获得等号左边的寄存器信息
                            auto leftRegInfo = getSymbolReg(leftName, false);
                            childList.insert(childList.end(), leftRegInfo.second.begin(), leftRegInfo.second.end());
                            childList.push_back(
                                    new TiggerAssignNode(expPtr->type, leftRegInfo.first, "s0", regInfo.first));
                            // 记录左边这个被修改过
                            leftInfo.isWrited = true;

                            // 如果左边是全局或者局部变量，更新值到内存中
                            if(leftInfo.isGlobal) {
                                childList.push_back(new TiggerLoadAddrNode(leftName, "t0"));
                                childList.push_back(new TiggerAssignNode("t0", 0, leftRegInfo.first));
                            } else if(leftInfo.isLocal) {
                                childList.push_back(new TiggerStoreNode(leftRegInfo.first, leftInfo.pos));
                            }

                        } else {
                            // 如果第一个操作符不是数字
                            auto op1Info = getSymbolReg(eeyoreSymbolToTigger[expPtr->firstOperand->name], true);
                            childList.insert(childList.end(), op1Info.second.begin(), op1Info.second.end());

                            if(expPtr->secondOperand->isNum()) {
                                // 如果右边是数字，以后将数字装在t0中
                                // 获得等号左边的寄存器信息
                                auto leftRegInfo = getSymbolReg(leftName, false);
                                childList.insert(childList.end(), leftRegInfo.second.begin(), leftRegInfo.second.end());
                                childList.push_back(new TiggerAssignNode(expPtr->type, leftRegInfo.first, op1Info.first,
                                                                         expPtr->secondOperand->getValue()));
                                // 记录左边这个被修改过
                                leftInfo.isWrited = true;

                                // 如果左边是全局或者局部变量，更新值到内存中
                                if(leftInfo.isGlobal) {
                                    childList.push_back(new TiggerLoadAddrNode(leftName, "t0"));
                                    childList.push_back(new TiggerAssignNode("t0", 0, leftRegInfo.first));
                                } else if(leftInfo.isLocal) {
                                    childList.push_back(new TiggerStoreNode(leftRegInfo.first, leftInfo.pos));
                                }

                            } else {
                                // 如果右边不是数字，也解析
                                auto op2Info = getSymbolReg(eeyoreSymbolToTigger[expPtr->secondOperand->name], true);
                                childList.insert(childList.end(), op2Info.second.begin(), op2Info.second.end());
                                // 获得等号左边的寄存器信息
                                auto leftRegInfo = getSymbolReg(leftName, false);
                                childList.insert(childList.end(), leftRegInfo.second.begin(), leftRegInfo.second.end());
                                childList.push_back(new TiggerAssignNode(expPtr->type, leftRegInfo.first, op1Info.first,
                                                                         op2Info.first));
                                // 记录左边这个被修改过
                                leftInfo.isWrited = true;

                                // 如果左边是全局或者局部变量，更新值到内存中
                                if(leftInfo.isGlobal) {
                                    childList.push_back(new TiggerLoadAddrNode(leftName, "t0"));
                                    childList.push_back(new TiggerAssignNode("t0", 0, leftRegInfo.first));
                                } else if(leftInfo.isLocal) {
                                    childList.push_back(new TiggerStoreNode(leftRegInfo.first, leftInfo.pos));
                                }
                            }
                        }
                    } else {
                        // 如果是一元表达式，假设操作数不是数字（否则可以直接计算）
                        assert(!expPtr->firstOperand->isNum());
                        // 取操作数的值
                        auto op1Info = getSymbolReg(eeyoreSymbolToTigger[expPtr->firstOperand->name], true);
                        childList.insert(childList.end(), op1Info.second.begin(), op1Info.second.end());
                        // 获得等号左边的寄存器信息
                        auto leftRegInfo = getSymbolReg(leftName, false);
                        childList.insert(childList.end(), leftRegInfo.second.begin(), leftRegInfo.second.end());
                        childList.push_back(new TiggerAssignNode(expPtr->type, leftRegInfo.first, op1Info.first));
                        // 记录左边这个被修改过
                        leftInfo.isWrited = true;

                        // 如果左边是全局或者局部变量，更新值到内存中
                        if(leftInfo.isGlobal) {
                            childList.push_back(new TiggerLoadAddrNode(leftName, "t0"));
                            childList.push_back(new TiggerAssignNode("t0", 0, leftRegInfo.first));
                        } else if(leftInfo.isLocal) {
                            childList.push_back(new TiggerStoreNode(leftRegInfo.first, leftInfo.pos));
                        }
                    }


                    // 表达式部分计算完成

                }
                    // 如果右边是一个符号或一个数字
                else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::RIGHT_VALUE) {
                    // 如果右边是right_value, 即右边是symbol或者num
                    auto rVarPtr = static_cast<EeyoreRightValueNode *>(assignPtr->rightTerm);

                    // 左边可能是一个符号或者一个数组
                    if(assignPtr->leftValue->isArray) {

                        // 如果右边是数字，要先放进寄存器s0
                        string rightReg = "s0";
                        if(rVarPtr->isNum())
                            childList.push_back(new TiggerAssignNode("s0", rVarPtr->getValue()));
                        else {
                            auto rightName = eeyoreSymbolToTigger[rVarPtr->name];
                            auto rightRegInfo = getSymbolReg(rightName, true);
                            childList.insert(childList.end(), rightRegInfo.second.begin(), rightRegInfo.second.end());
                            rightReg = rightRegInfo.first;
                        }
                        // 注意此时s0寄存器占用中


                        // 如果左边是一个数组
                        string leftTiggerName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                        auto &leftInfo = symbolInfo[leftTiggerName];
                        // 获得等号左边的寄存器信息
                        auto leftRegInfo = getSymbolReg(leftTiggerName, false);
                        childList.insert(childList.end(), leftRegInfo.second.begin(), leftRegInfo.second.end());

                        auto leftRegName = leftRegInfo.first;
                        // 现在处理左边括号中的情况
                        if(!assignPtr->leftValue->rValNodePtr->isNum()) {
                            // 如果左边括号中是数字，一会可以直接处理
                            // 如果左边括号中是一个寄存器，应该将左边的地址和寄存器相加，然后再处理
                            // 将左边括号中的对应寄存器找出，放进s0中
                            auto tName = eeyoreSymbolToTigger[assignPtr->leftValue->rValNodePtr->name];
                            auto tInfo = getSymbolReg(tName, true);
                            childList.insert(childList.end(), tInfo.second.begin(), tInfo.second.end());
                            // 加地址，将地址放入"t0"
                            childList.push_back(
                                    new TiggerAssignNode(OpType::opPlus, "t0", leftRegInfo.first, tInfo.first));
                            leftRegName = "t0";
                        }

                        // 最后的赋值
                        // 如果左边括号中是数字
                        if(assignPtr->leftValue->rValNodePtr->isNum()) {
                            childList.push_back(
                                    new TiggerAssignNode(leftRegName, assignPtr->leftValue->rValNodePtr->getValue(),
                                                         rightReg));
                        } else {
                            childList.push_back(new TiggerAssignNode(leftRegName, 0, rightReg));
                        }
                    } else {
                        // 如果左边是一个符号
                        string leftTName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                        auto &leftInfo = symbolInfo[leftTName];
                        // 获得等号左边的寄存器信息
                        auto leftRegInfo = getSymbolReg(leftTName, false);
                        childList.insert(childList.end(), leftRegInfo.second.begin(), leftRegInfo.second.end());

                        if(rVarPtr->isNum()) {
                            // 直接赋值
                            childList.push_back(new TiggerAssignNode(leftRegInfo.first, rVarPtr->getValue()));
                        } else {
                            // 获取右边符号对应的寄存器信息
                            auto rightName = eeyoreSymbolToTigger[rVarPtr->name];
                            auto rightRegInfo = getSymbolReg(rightName, true);
                            childList.insert(childList.end(), rightRegInfo.second.begin(), rightRegInfo.second.end());
                            childList.push_back(new TiggerAssignNode(leftRegInfo.first, rightRegInfo.first));
                        }

                        // 如果左边是全局或者局部变量
                        if(leftInfo.isGlobal) {
                            childList.push_back(new TiggerLoadAddrNode(leftTName, "t0"));
                            childList.push_back(new TiggerAssignNode("t0", 0, leftRegInfo.first));
                        } else if(leftInfo.isLocal) {
                            childList.push_back(new TiggerStoreNode(leftRegInfo.first, leftInfo.pos));
                        }
                        // 设置左边的变量有修改
                        leftInfo.isWrited = true;
                    }

                }
                    // 左值
                else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::LEFT_VALUE) {
                    // 如果右边是right_value, 即右边是symbol或者num
                    auto lVarPtr = static_cast<EeyoreLeftValueNode *>(assignPtr->rightTerm);
                    // 左边应该一定不是array
                    assert(!assignPtr->leftValue->isArray);
                    // 获取右边符号对应的寄存器信息
                    auto rightName = eeyoreSymbolToTigger[lVarPtr->name];
                    auto rightRegInfo = getSymbolReg(rightName, true);
                    childList.insert(childList.end(), rightRegInfo.second.begin(), rightRegInfo.second.end());
                    // 左边的信息
                    auto leftName = eeyoreSymbolToTigger[assignPtr->leftValue->name];
                    auto leftRegInfo = getSymbolReg(leftName, false);
                    childList.insert(childList.end(), leftRegInfo.second.begin(), leftRegInfo.second.end());

                    if(lVarPtr->isArray) {
                        // 如果右边是个数组
                        // 如果右边的数组里是数字
                        if(lVarPtr->rValNodePtr->isNum()) {
                            childList.push_back(new TiggerAssignNode(leftRegInfo.first, rightRegInfo.first,
                                                                     lVarPtr->rValNodePtr->getValue()));
                        } else {
                            // 如果右边的数组里不是数字，则需要先算出目标地址
                            // 目标地址在t0
                            auto tName = eeyoreSymbolToTigger[lVarPtr->rValNodePtr->name];
                            auto tInfo = getSymbolReg(tName, true);
                            childList.push_back(
                                    new TiggerAssignNode(OpType::opPlus, "t0", rightRegInfo.first, tInfo.first));
                            childList.push_back(new TiggerAssignNode(leftRegInfo.first, "t0", 0));
                        }
                    } else {
                        // 两边都是一个单独的符号
                        childList.push_back(new TiggerAssignNode(leftRegInfo.first, rightRegInfo.first));
                    }
                    symbolInfo[leftName].isWrited = true;

                    // 如果左边是全局或者局部变量，更新值到内存中
                    if(symbolInfo[leftName].isGlobal) {
                        childList.push_back(new TiggerLoadAddrNode(leftName, "t0"));
                        childList.push_back(new TiggerAssignNode("t0", 0, leftRegInfo.first));
                    } else if(symbolInfo[leftName].isLocal) {
                        childList.push_back(new TiggerStoreNode(leftRegInfo.first, symbolInfo[leftName].pos));
                    }

                } else
                    assert(false);

                childList.push_back(new TiggerCommentNode(""));
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
                childList.push_back(new TiggerCommentNode(""));
                break;
            }
            case EeyoreNodeType::IF_GOTO: {
                childList.push_back(new TiggerCommentNode("// " + ptr->to_eeyore_string()));
                auto ifGotoPtr = static_cast<EeyoreIfGotoNode *>(ptr);
                if(ifGotoPtr->condRightValue->isNum()) {
                    // 如果是数字，将数字放进s0中
                    childList.push_back(new TiggerAssignNode("s0", ifGotoPtr->condRightValue->getValue()));
                    childList.push_back(new TiggerIfGotoNode(ifGotoPtr->isEq, "s0", ifGotoPtr->label));
                } else {
                    // 否则，首先读取条件
                    string condName = eeyoreSymbolToTigger[ifGotoPtr->condRightValue->name];
                    auto &condInfo = symbolInfo[condName];
                    // 获得等号左边的寄存器信息
                    auto condRegInfo = getSymbolReg(condName, true);
                    childList.insert(childList.end(), condRegInfo.second.begin(), condRegInfo.second.end());
                    // 添加if goto
                    childList.push_back(new TiggerIfGotoNode(ifGotoPtr->isEq, condRegInfo.first, ifGotoPtr->label));
                }
                childList.push_back(new TiggerCommentNode(""));
                break;
            }
            case EeyoreNodeType::PARAM: {
                auto paramPtr = static_cast<EeyoreFuncParamNode *>(ptr);
                // 如果是第一个参数，先保存一下当前的寄存器

                if(curParamCnt == 0) {
                    childList.push_back(new TiggerCommentNode("// save 'a' regs"));
                    for(int j = 0; j < usedParamRegNum; j++) {
                        // a0是19(从第0个开始)
                        childList.push_back(new TiggerStoreNode("a" + std::to_string(j), 19 + j));
                    }
                    childList.push_back(new TiggerCommentNode(""));
                }

                childList.push_back(new TiggerCommentNode("// " + ptr->to_eeyore_string()));
                string curParamReg = "a" + std::to_string(curParamCnt);
                curParamCnt++;

                if(paramPtr->param->isNum()) {
                    // 直接将数字放进对应的函数参数寄存器中
                    childList.push_back(new TiggerAssignNode(curParamReg, paramPtr->param->getValue()));
                } else {
                    // 否则，首先读取条件
                    string paramName = eeyoreSymbolToTigger[paramPtr->param->name];
                    auto &paramInfo = symbolInfo[paramName];
                    if(paramName[0] == 'a') {
                        int num = paramName[1] - '0';
                        childList.push_back(new TiggerLoadNode(19 + num, curParamReg));

                    } else {
                        // 获得等号左边的寄存器信息
                        auto paramRegInfo = getSymbolReg(paramName, true);
                        childList.insert(childList.end(), paramRegInfo.second.begin(), paramRegInfo.second.end());
                        // 添加赋值
                        childList.push_back(new TiggerAssignNode(curParamReg, paramRegInfo.first));
                    }
                }
                childList.push_back(new TiggerCommentNode(""));
                break;
            }
            case EeyoreNodeType::FUNC_CALL: {
                auto funcCallPtr = static_cast<EeyoreFuncCallNode *>(ptr);
                childList.push_back(new TiggerCommentNode("// " + ptr->to_eeyore_string()));
                childList.push_back(new TiggerCommentNode("// save 't' regs"));
                for(int j = 0; j < 7; j++) {
                    childList.push_back(new TiggerStoreNode("t" + std::to_string(j), 12 + j));
                }
                childList.push_back(new TiggerCommentNode("// save global vars"));
                // 保存存储在寄存器中的全局变量的值
                for(int j = 0; j < validRegNum; j++)
                    if(regUse[j] && symbolInfo[regUseName[j]].isGlobal && !symbolInfo[regUseName[j]].isArray) {
                        // 如果是全局变量
                        // 将地址放到t0，然后t0[0] = 当前值
                        childList.push_back(new TiggerLoadAddrNode(regUseName[j], "t0"));
                        childList.push_back(new TiggerAssignNode("t0", 0, getRegName(j)));
                        symbolInfo[regUseName[j]].inReg = false;
                        regUse[j] = false;
                    }

                // 调用函数
                childList.push_back(new TiggerFuncCallNode(funcCallPtr->name));
                // 如果有返回值，要把a0保存到目标寄存器
                if(funcCallPtr->hasReturnValue) {
                    string returnName = eeyoreSymbolToTigger[funcCallPtr->returnSymbol];
                    auto &returnInfo = symbolInfo[returnName];
                    auto returnRegInfo = getSymbolReg(returnName, false);
                    childList.insert(childList.end(), returnRegInfo.second.begin(), returnRegInfo.second.end());
                    // 添加赋值
                    childList.push_back(new TiggerAssignNode(returnRegInfo.first, "a0"));
                    returnInfo.isWrited = true;

                    // 如果左边是全局或者局部变量，更新值到内存中
                    if(returnInfo.isGlobal) {
                        childList.push_back(new TiggerLoadAddrNode(returnName, "t0"));
                        childList.push_back(new TiggerAssignNode("t0", 0, returnRegInfo.first));
                    } else if(returnInfo.isLocal) {
                        childList.push_back(new TiggerStoreNode(returnRegInfo.first, returnInfo.pos));
                    }
                }
                // 恢复a和t
                childList.push_back(new TiggerCommentNode("// load 't' and 'a' regs"));
                for(int j = 0; j < 7; j++) {
                    childList.push_back(new TiggerLoadNode(12 + j, "t" + std::to_string(j)));
                }
                for(int j = 0; j < usedParamRegNum; j++) {
                    childList.push_back(new TiggerLoadNode(19 + j, "a" + std::to_string(j)));
                }

                curParamCnt = 0; // 重新计数

                childList.push_back(new TiggerCommentNode(""));
                break;
            }
            case EeyoreNodeType::RETURN: {
                auto returnPtr = static_cast<EeyoreReturnNode *>(ptr);
                childList.push_back(new TiggerCommentNode("// " + ptr->to_eeyore_string()));
                // 恢复寄存器在generate中完成
                childList.push_back(new TiggerCommentNode("// save global vars"));
                // 保存存储在寄存器中的全局变量的值
                for(int j = 0; j < validRegNum; j++)
                    if(regUse[j] && symbolInfo[regUseName[j]].isGlobal && !symbolInfo[regUseName[j]].isArray) {
                        // 如果是全局变量
                        // 将地址放到t0，然后t0[0] = 当前值
                        childList.push_back(new TiggerLoadAddrNode(regUseName[j], "t0"));
                        childList.push_back(new TiggerAssignNode("t0", 0, getRegName(j)));
                    }
                // 处理返回值
                if(returnPtr->hasReturnValue) {
                    if(returnPtr->returnValuePtr->isNum()) {
                        childList.push_back(new TiggerAssignNode("a0", returnPtr->returnValuePtr->getValue()));
                    } else {
                        string returnName = eeyoreSymbolToTigger[returnPtr->returnValuePtr->name];
                        auto &returnInfo = symbolInfo[returnName];
                        auto returnRegInfo = getSymbolReg(returnName, true);
                        childList.insert(childList.end(), returnRegInfo.second.begin(), returnRegInfo.second.end());
                        // 添加赋值
                        childList.push_back(new TiggerAssignNode("a0", returnRegInfo.first));
                    }
                }
                childList.push_back(new TiggerReturnNode());
                childList.push_back(new TiggerCommentNode(""));
                break;
            }
            case EeyoreNodeType::COMMENT: {
                auto commentPtr = static_cast<EeyoreCommentNode *>(ptr);
                if(commentPtr->comment == "// end global var init") {
                    // 将当前的全局变量信息全部取消
                    // 保存存储在寄存器中的全局变量的值
                    for(int j = 0; j < validRegNum; j++)
                        if(regUse[j] && symbolInfo[regUseName[j]].isGlobal && !symbolInfo[regUseName[j]].isArray) {
                            // 如果是全局变量
                            // 将地址放到t0，然后t0[0] = 当前值
                            childList.push_back(new TiggerLoadAddrNode(regUseName[j], "t0"));
                            childList.push_back(new TiggerAssignNode("t0", 0, getRegName(j)));
                            symbolInfo[regUseName[j]].inReg = false;
                            regUse[j] = false;
                        }
                }
            }
            case EeyoreNodeType::BLOCK_BEGIN:
            case EeyoreNodeType::BLOCK_END:
            case EeyoreNodeType::FILL_INIT:
                break;
            default:
                assert(false);
        }
    }

    paramNum = eeyoreFunc->paramNum;
    stackSize = baseStackTop + localIntVarCnt;
    funcName = eeyoreFunc->funcName;

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
