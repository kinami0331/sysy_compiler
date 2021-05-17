//
// Created by KiNAmi on 2021/5/16.
//
#include "eeyore_ast.hpp"

bool EeyoreLeftValueNode::isLocalNotArray() {
    return name[0] != 'p' && !EeyoreBaseNode::getVarInfo(name).isGlobal && !EeyoreBaseNode::getVarInfo(name).isArray;
}

bool EeyoreRightValueNode::isLocalNotArray() {
    return !_isNum && name[0] != 'p' && !EeyoreBaseNode::getVarInfo(name).isGlobal &&
           !EeyoreBaseNode::getVarInfo(name).isArray;
}

void EeyoreRootNode::generateCFG() {
    for(auto ptr:childList)
        if(ptr->nodeType == EeyoreNodeType::FUNC_DEF) {
            static_cast<EeyoreFuncDefNode *>(ptr)->generateCFG();
            static_cast<EeyoreFuncDefNode *>(ptr)->liveVarAnalysis();
        }
}

void EeyoreFuncDefNode::generateCFG() {
    // 第一轮：找到所有基本块
    // 首先，将变量声明和main函数中的全局变量初始化统一作为Entry
    auto beginIt = find_if(childList.begin(), childList.end(),
                           [](EeyoreBaseNode *t) {
                               return t->nodeType != EeyoreNodeType::VAR_DECL &&
                                      t->nodeType != EeyoreNodeType::GLOBAL_INIT;
                           });
    auto curBlock = new BasicBlock(0);
    curBlock->setLabelName("entry");
    curBlock->stmtList.insert(curBlock->stmtList.end(), childList.begin(), beginIt);
    basicBlockList.push_back(curBlock);

    unsigned int curBlockId = 1;
    curBlock = new BasicBlock(curBlockId);
    for(auto it = beginIt; it != childList.end(); it++) {
        if((*it)->nodeType == EeyoreNodeType::COMMENT || (*it)->nodeType == EeyoreNodeType::BLOCK_END ||
           (*it)->nodeType == EeyoreNodeType::BLOCK_BEGIN || (*it)->nodeType == EeyoreNodeType::GLOBAL_INIT ||
           (*it)->nodeType == EeyoreNodeType::FILL_INIT)
            continue;
        if(curBlock->stmtList.empty()) {
            curBlock->insertStmt(*it);
            if((*it)->nodeType == EeyoreNodeType::LABEL) {
                curBlock->setLabelName(static_cast<EeyoreLabelNode *>(*it)->label);
                labelToBlockId[static_cast<EeyoreLabelNode *>(*it)->label] = curBlockId;
            }
            continue;
        }
        switch((*it)->nodeType) {
            case EeyoreNodeType::LABEL: {
                // 默认label是一个新的基本块的开始
                // 一个小优化：两个连续的label可以合并
                auto labelPtr = static_cast<EeyoreLabelNode *>(*it);
                // 如果当前block的最后一个元素不是label，则新开一个block
                assert(!curBlock->stmtList.empty());
                if(curBlock->stmtList.back()->nodeType == EeyoreNodeType::LABEL) {
                    curBlock->setLabelName(curBlock->blockLabel + " & " + labelPtr->label);
                    labelToBlockId[labelPtr->label] = curBlockId;
                } else {
                    basicBlockList.push_back(curBlock);
                    curBlockId++;
                    curBlock = new BasicBlock(curBlockId);
                    curBlock->setLabelName(labelPtr->label);
                    labelToBlockId[labelPtr->label] = curBlockId;
                }
                curBlock->insertStmt(*it);
                break;
            }
            case EeyoreNodeType::COMMENT: {
                // TODO 这里可以考虑不插入注释
                // curBlock->insertStmt(*it);
                break;
            }
            case EeyoreNodeType::FUNC_CALL:
            case EeyoreNodeType::RETURN:
            case EeyoreNodeType::GOTO:
            case EeyoreNodeType::IF_GOTO: {
                // 下一句是基本块开始
                curBlock->insertStmt(*it);
                basicBlockList.push_back(curBlock);
                curBlockId++;
                curBlock = new BasicBlock(curBlockId);
                break;
            }
            default:
                curBlock->insertStmt(*it);
        }
    }
    // 结束当前的block
    if(curBlock->stmtList.empty()) {
        curBlock->setLabelName("EXIT");
        basicBlockList.push_back(curBlock);
    } else {
        basicBlockList.push_back(curBlock);
        curBlockId++;
        curBlock = new BasicBlock(curBlockId);
        curBlock->setLabelName("EXIT");
        basicBlockList.push_back(curBlock);
    }
    assert(curBlockId + 1 == basicBlockList.size());

    // 第二轮，建立基本块之间的边
    int blockNum = curBlockId + 1;
    for(int i = 0; i < blockNum - 1; i++) {
        // 看最后一句
        if(basicBlockList[i]->stmtList.empty())
            continue;
        auto lastPtr = basicBlockList[i]->stmtList.back();
        if(lastPtr->nodeType == EeyoreNodeType::IF_GOTO) {
            auto ifGotoPtr = static_cast<EeyoreIfGotoNode *>(lastPtr);
            assert(labelToBlockId.count(ifGotoPtr->label) > 0);
            basicBlockList[i]->nextNodeList.push_back(labelToBlockId[ifGotoPtr->label]);
            basicBlockList[labelToBlockId[ifGotoPtr->label]]->preNodeList.push_back(i);
            basicBlockList[i]->nextNodeList.push_back(i + 1);
            basicBlockList[i + 1]->preNodeList.push_back(i);
        } else if(basicBlockList[i]->stmtList.back()->nodeType == EeyoreNodeType::GOTO) {
            auto gotoPtr = static_cast<EeyoreGotoNode *>(lastPtr);
            assert(labelToBlockId.count(gotoPtr->label) > 0);
            basicBlockList[i]->nextNodeList.push_back(labelToBlockId[gotoPtr->label]);
            basicBlockList[labelToBlockId[gotoPtr->label]]->preNodeList.push_back(i);
        } else if(basicBlockList[i]->stmtList.back()->nodeType == EeyoreNodeType::RETURN) {
            basicBlockList[i]->nextNodeList.push_back(blockNum - 1);
            basicBlockList[blockNum - 1]->preNodeList.push_back(i);
        } else {
            basicBlockList[i]->nextNodeList.push_back(i + 1);
            basicBlockList[i + 1]->preNodeList.push_back(i);
        }
    }
}

void EeyoreFuncDefNode::liveVarAnalysis() {
    calcDefAndUseSet();
    calcInAndOutSet();
}

void EeyoreFuncDefNode::calcDefAndUseSet() {
    int s = basicBlockList.size();
    for(int i = 1; i < s - 1; i++) {
        basicBlockList[i]->calcDefAndUseSet();
    }
}


void EeyoreFuncDefNode::BasicBlock::calcDefAndUseSet() {
    // 从前往后扫描，忽略全局变量
    set<string> defRecord; // 是否有赋值
    set<string> useRecord; // 记录一个局部或临时变量是否有使用
    for(auto ptr:stmtList) {
        if(ptr->nodeType == EeyoreNodeType::LABEL)
            continue;
        switch(ptr->nodeType) {
            case EeyoreNodeType::ASSIGN: {
                auto assignPtr = static_cast<EeyoreAssignNode *>(ptr);

                // 先检查右边
                if(assignPtr->rightTerm->nodeType == EeyoreNodeType::EXP) {
                    auto expPtr = static_cast<EeyoreExpNode *>(assignPtr->rightTerm);
                    // 检查第一个参数
                    if(expPtr->firstOperand->isLocalNotArray()) {
                        // 如果没有def过，加入use集合中
                        if(defRecord.count(expPtr->firstOperand->name) == 0)
                            useSet.insert(expPtr->firstOperand->name);
                        useRecord.insert(expPtr->firstOperand->name);
                    }
                    // 检查第二个参数
                    if(expPtr->isBinary) {
                        if(expPtr->secondOperand->isLocalNotArray()) {
                            // 如果没有def过，加入use集合中
                            if(defRecord.count(expPtr->secondOperand->name) == 0)
                                useSet.insert(expPtr->secondOperand->name);
                            useRecord.insert(expPtr->secondOperand->name);
                        }
                    }
                } else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::RIGHT_VALUE) {
                    auto rValPtr = static_cast<EeyoreRightValueNode *>(assignPtr->rightTerm);
                    if(rValPtr->isLocalNotArray()) {
                        // 如果没有def过，加入use集合中
                        if(defRecord.count(rValPtr->name) == 0)
                            useSet.insert(rValPtr->name);
                        useRecord.insert(rValPtr->name);
                    }
                } else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::LEFT_VALUE) {
                    auto lValPtr = static_cast<EeyoreLeftValueNode *>(assignPtr->rightTerm);
                    if(lValPtr->isLocalNotArray()) {
                        // 如果没有def过，加入use集合中
                        if(defRecord.count(lValPtr->name) == 0)
                            useSet.insert(lValPtr->name);
                        useRecord.insert(lValPtr->name);
                    }
                } else
                    assert(false);

                // 再检查左边
                if(assignPtr->leftValue->isLocalNotArray()) {
                    // 如果还没有use过，则加入到def集合中
                    if(useRecord.count(assignPtr->leftValue->name) == 0)
                        defSet.insert(assignPtr->leftValue->name);
                    // 记录该变量已经赋值过
                    defRecord.insert(assignPtr->leftValue->name);
                }
                break;
            }
            case EeyoreNodeType::PARAM: {
                if(static_cast<EeyoreFuncParamNode *>(ptr)->param->isLocalNotArray()) {
                    // 如果还没有def过，则加入到use集合中
                    if(defRecord.count(static_cast<EeyoreFuncParamNode *>(ptr)->param->name) == 0)
                        useSet.insert(static_cast<EeyoreFuncParamNode *>(ptr)->param->name);
                    // 记录该变量已经使用
                    useRecord.insert(static_cast<EeyoreFuncParamNode *>(ptr)->param->name);
                }
                break;
            }
            case EeyoreNodeType::FUNC_CALL: {
                if(static_cast<EeyoreFuncCallNode *>(ptr)->hasReturnValue) {
                    assert(static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol[0] != 'p');
                    if(useRecord.count(static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol) == 0)
                        defSet.insert(static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol);
                    defRecord.insert(static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol);
                }
                break;
            }
            case EeyoreNodeType::IF_GOTO: {
                if(static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->isLocalNotArray()) {
                    // 如果还没有def过，则加入到use集合中
                    if(defRecord.count(static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->name) == 0)
                        useSet.insert(static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->name);
                    // 记录该变量已经赋值过
                    useRecord.insert(static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->name);
                }
                break;
            }
            case EeyoreNodeType::GOTO: {
                break;
            }
            case EeyoreNodeType::RETURN : {
                if(static_cast<EeyoreReturnNode *>(ptr)->hasReturnValue &&
                   static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->isLocalNotArray()) {
                    // 如果还没有def过，则加入到use集合中
                    if(defRecord.count(static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->name) == 0)
                        useSet.insert(static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->name);
                    // 记录该变量已经赋值过
                    useRecord.insert(static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->name);
                }
                break;
            }
            default:
                assert(false);
        }
    }


}

void EeyoreFuncDefNode::calcInAndOutSet() {
    bool hasChange = true;
    int s = basicBlockList.size();
    while(hasChange) {
        hasChange = false;
        for(int i = s - 2; i >= 1; i--) {
            // OUT 等于所有后继的IN的并
            for(auto next:basicBlockList[i]->nextNodeList) {
                auto nextInSet = basicBlockList[next]->inSet;
                basicBlockList[i]->outSet.merge(nextInSet);
            }
            // 保存现在的in
            auto oriInSet = basicBlockList[i]->inSet;
            // 加入use
            for(auto t:basicBlockList[i]->useSet)
                basicBlockList[i]->inSet.insert(t);
            // 加入不在def中的out
            for(auto t:basicBlockList[i]->outSet)
                if(basicBlockList[i]->defSet.count(t) == 0)
                    basicBlockList[i]->inSet.insert(t);
            // 判断in是否改变
            if(oriInSet != basicBlockList[i]->inSet)
                hasChange = true;
        }
    }
}

void EeyoreRootNode::simplifyTempVar() {
    for(auto ptr:childList)
        if(ptr->nodeType == EeyoreNodeType::FUNC_DEF)
            static_cast<EeyoreFuncDefNode *>(ptr)->simplifyTempVar();
}

void EeyoreFuncDefNode::simplifyTempVar() {
    map<string, bool> necessaryTempVar;
    map<string, string> varReplace;
    // 第一轮，简化
    for(auto ptr:childList) {
        switch(ptr->nodeType) {
            case EeyoreNodeType::ASSIGN: {
                auto assignPtr = static_cast<EeyoreAssignNode *>(ptr);
                // 先检查右边
                if(assignPtr->rightTerm->nodeType == EeyoreNodeType::EXP) {
                    auto expPtr = static_cast<EeyoreExpNode *>(assignPtr->rightTerm);
                    // 检查第一个参数
                    if(!expPtr->firstOperand->isNum() && expPtr->firstOperand->name[0] == 't') {
                        assert(varReplace.count(expPtr->firstOperand->name) > 0);
                        auto newName = varReplace[expPtr->firstOperand->name];
                        assert(necessaryTempVar.count(newName) > 0);
                        expPtr->firstOperand->name = newName;
                        necessaryTempVar[newName] = false;
                    }
                    // 检查第二个参数
                    if(expPtr->isBinary) {
                        if(!expPtr->secondOperand->isNum() && expPtr->secondOperand->name[0] == 't') {
                            assert(varReplace.count(expPtr->secondOperand->name) > 0);
                            auto newName = varReplace[expPtr->secondOperand->name];
                            assert(necessaryTempVar.count(newName) > 0);
                            expPtr->secondOperand->name = newName;
                            necessaryTempVar[newName] = false;
                        }
                    }
                } else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::RIGHT_VALUE) {
                    auto rValPtr = static_cast<EeyoreRightValueNode *>(assignPtr->rightTerm);
                    if(!rValPtr->isNum() && rValPtr->name[0] == 't') {
                        assert(varReplace.count(rValPtr->name) > 0);
                        auto newName = varReplace[rValPtr->name];
                        assert(necessaryTempVar.count(newName) > 0);
                        rValPtr->name = newName;
                        necessaryTempVar[newName] = false;
                    }
                } else if(assignPtr->rightTerm->nodeType == EeyoreNodeType::LEFT_VALUE) {
                    auto lValPtr = static_cast<EeyoreLeftValueNode *>(assignPtr->rightTerm);
                    if(lValPtr->name[0] == 't') {
                        assert(varReplace.count(lValPtr->name) > 0);
                        auto newName = varReplace[lValPtr->name];
                        assert(necessaryTempVar.count(newName) > 0);
                        lValPtr->name = newName;
                        necessaryTempVar[newName] = false;
                    } else if(lValPtr->isArray && lValPtr->rValNodePtr->name[0] == 't') {
                        assert(varReplace.count(lValPtr->rValNodePtr->name) > 0);
                        auto newName = varReplace[lValPtr->rValNodePtr->name];
                        assert(necessaryTempVar.count(newName) > 0);
                        lValPtr->rValNodePtr->name = newName;
                        necessaryTempVar[newName] = false;
                    }
                } else
                    assert(false);

                // 如果左边是一个临时变量
                if(assignPtr->leftValue->name[0] == 't') {
                    assert(assignPtr->leftValue->isArray == false);
                    // 如果左边这个变量出现过，可能是出现在短路运算中，需要特判
                    if(varReplace.count(assignPtr->leftValue->name) > 0) {
                        assert(assignPtr->rightTerm->nodeType == EeyoreNodeType::RIGHT_VALUE &&
                               (static_cast<EeyoreRightValueNode *>(assignPtr->rightTerm)->getValue() == 1 ||
                                static_cast<EeyoreRightValueNode *>(assignPtr->rightTerm)->getValue() == 0));
                        assignPtr->leftValue->name = varReplace[assignPtr->leftValue->name];
                        break;
                    }
                    // 查看能否找到一个变量替换
                    string newName = "";
                    for(auto record: necessaryTempVar) {
                        if(!record.second) {
                            newName = record.first;
                            necessaryTempVar[newName] = true;
                            break;
                        }
                    }
                    // 如果找到了
                    if(newName != "") {
                        varReplace[assignPtr->leftValue->name] = newName;
                        assignPtr->leftValue->name = newName;
                    } else {
                        newName = assignPtr->leftValue->name;
                        necessaryTempVar[newName] = true;
                        varReplace[newName] = newName;
                    }
                } else if(assignPtr->leftValue->isArray) {
                    // 如果左边是个数组，需要看括号中的值
                    if(!assignPtr->leftValue->rValNodePtr->isNum() &&
                       assignPtr->leftValue->rValNodePtr->name[0] == 't') {
                        assert(varReplace.count(assignPtr->leftValue->rValNodePtr->name) > 0);
                        auto newName = varReplace[assignPtr->leftValue->rValNodePtr->name];
                        assert(necessaryTempVar.count(newName) > 0);
                        assignPtr->leftValue->rValNodePtr->name = newName;
                        necessaryTempVar[newName] = false;
                    }
                }
                break;
            }
            case EeyoreNodeType::FUNC_CALL: {
                for(auto t:static_cast<EeyoreFuncCallNode *>(ptr)->paramList) {
                    if(!t->isNum() && t->name[0] == 't') {
                        assert(varReplace.count(t->name) > 0);
                        auto newName = varReplace[t->name];
                        assert(necessaryTempVar.count(newName) > 0);
                        t->name = newName;
                        necessaryTempVar[newName] = false;
                    }
                }
                if(static_cast<EeyoreFuncCallNode *>(ptr)->hasReturnValue) {
                    assert(static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol[0] != 'p');
                    if(static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol[0] == 't') {
                        // 查看能否找到一个变量替换
                        string newName = "";
                        for(auto record: necessaryTempVar) {
                            if(!record.second) {
                                newName = record.first;
                                necessaryTempVar[newName] = true;
                                break;
                            }
                        }
                        // 如果找到了
                        if(newName != "") {
                            varReplace[static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol] = newName;
                            static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol = newName;
                        } else {
                            newName = static_cast<EeyoreFuncCallNode *>(ptr)->returnSymbol;
                            necessaryTempVar[newName] = true;
                            varReplace[newName] = newName;
                        }
                    }
                }
                break;
            }
            case EeyoreNodeType::IF_GOTO: {
                if(!static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->isNum() &&
                   static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->name[0] == 't') {
                    assert(varReplace.count(static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->name) > 0);
                    auto newName = varReplace[static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->name];
                    assert(necessaryTempVar.count(newName) > 0);
                    static_cast<EeyoreIfGotoNode *>(ptr)->condRightValue->name = newName;
                    necessaryTempVar[newName] = false;
                }
                break;
            }
            case EeyoreNodeType::RETURN: {
                if(static_cast<EeyoreReturnNode *>(ptr)->hasReturnValue &&
                   !static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->isNum() &&
                   static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->name[0] == 't') {
                    assert(varReplace.count(static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->name) > 0);
                    auto newName = varReplace[static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->name];
                    assert(necessaryTempVar.count(newName) > 0);
                    static_cast<EeyoreReturnNode *>(ptr)->returnValuePtr->name = newName;
                    necessaryTempVar[newName] = false;
                }
                break;
            }
            default:
                break;
        }
    }

    vector<EeyoreBaseNode *> newChildList;
    // 第二轮，去除没用到的局部变量声明
    for(auto ptr:childList) {
        if(ptr->nodeType == EeyoreNodeType::VAR_DECL &&
           static_cast<EeyoreVarDeclNode *>(ptr)->name[0] == 't' &&
           necessaryTempVar.count(static_cast<EeyoreVarDeclNode *>(ptr)->name) == 0)
            continue;
        newChildList.push_back(ptr);
    }
//    assert(necessaryTempVar.size() <= 3);
    childList = move(newChildList);
}