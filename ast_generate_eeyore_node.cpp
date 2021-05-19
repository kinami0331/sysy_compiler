//
// Created by KiNAmi on 2021/5/11.
//

#include "ast.hpp"
#include "eeyore_ast.hpp"

pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>> ExpNode::extractEeyoreExp() {
    vector<EeyoreBaseNode *> eeyoreList;
    EeyoreRightValueNode *rValPtr;

    switch(expType) {
        case ExpType::Number: {
            assert(childNodes[0]->nodeType == NodeType::NUMBER);
            rValPtr = new EeyoreRightValueNode(static_cast<NumberNode *>(childNodes[0])->num);
            break;
        }
        case ExpType::LVal: {
            if(childNodes[0]->childNodes.size() == 1) {
                assert(childNodes[0]->childNodes[0]->nodeType == NodeType::IDENT);
                rValPtr = new EeyoreRightValueNode(static_cast<IdentNode *>(childNodes[0]->childNodes[0])->id);
            } else {
                assert(childNodes[0]->childNodes[0]->nodeType == NodeType::IDENT);
                string lValName = static_cast<IdentNode *>(childNodes[0]->childNodes[0])->id;
                assert(childNodes[0]->childNodes[1]->nodeType == NodeType::EXP);
                // 将左值的括号中的exp解为若干个赋值语句和一个exp
                auto t = static_cast<ExpNode *>(childNodes[0]->childNodes[1])->extractEeyoreExp();
                eeyoreList = move(t.second);
                // 将左值括号中的结果乘以4
                if(t.first->isNum()) {
                    // 创建一个新的临时变量
                    auto newTempName = "t" + to_string(tempValueCnt);
                    tempValueCnt++;
                    // 注册临时变量
                    EeyoreBaseNode::registerTempSymbol(newTempName);
                    // 临时变量声明
                    eeyoreList.push_back(new EeyoreVarDeclNode(newTempName));
                    // 返回的是一个右值
                    rValPtr = new EeyoreRightValueNode(newTempName);

                    // 新建一个赋值语句，左边是一个新的临时变量，右边是该左值
                    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(newTempName),
                                                              new EeyoreLeftValueNode(lValName,
                                                                                      new EeyoreRightValueNode(
                                                                                              t.first->getValue() *
                                                                                              4))));
                } else {
                    // 创建一个新的临时变量
                    auto indexTempVar = "t" + to_string(tempValueCnt);
                    tempValueCnt++;
                    // 注册临时变量
                    EeyoreBaseNode::registerTempSymbol(indexTempVar);
                    // 临时变量声明
                    eeyoreList.push_back(new EeyoreVarDeclNode(indexTempVar));
                    // 添加表达式
                    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(indexTempVar),
                                                              new EeyoreExpNode(OpType::opMul, t.first,
                                                                                new EeyoreRightValueNode(4))));
                    // 创建一个新的临时变量
                    auto newTempName = "t" + to_string(tempValueCnt);
                    tempValueCnt++;
                    // 注册临时变量
                    EeyoreBaseNode::registerTempSymbol(newTempName);
                    // 临时变量声明
                    eeyoreList.push_back(new EeyoreVarDeclNode(newTempName));
                    // 返回的是一个右值
                    rValPtr = new EeyoreRightValueNode(newTempName);

                    // 新建一个赋值语句，左边是一个新的临时变量，右边是该左值
                    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(newTempName),
                                                              new EeyoreLeftValueNode(lValName,
                                                                                      new EeyoreRightValueNode(
                                                                                              indexTempVar))));
                }


            }
            break;
        }
        case ExpType::FuncCall: {
            // 函数名
            string funcName = static_cast<IdentNode *>(childNodes[0]->childNodes[0])->id;
            // 创建一个新的临时变量
            auto newTempName = "t" + to_string(tempValueCnt);
            tempValueCnt++;
            // 注册临时变量
            EeyoreBaseNode::registerTempSymbol(newTempName);
            // 临时变量声明
            eeyoreList.push_back(new EeyoreVarDeclNode(newTempName));
            // 返回值
            rValPtr = new EeyoreRightValueNode(newTempName);
            // 首先创建一个新的FuncCall, 返回值复制到临时变量
            auto eeyoreFuncCall = new EeyoreFuncCallNode(funcName, newTempName);
            // 生成每一个参数
            for(int i = 1; i < childNodes[0]->childNodes.size(); i++) {
                assert(childNodes[0]->childNodes[i]->nodeType == NodeType::EXP);
                auto t = static_cast<ExpNode *>(childNodes[0]->childNodes[i])->extractEeyoreExp();
                eeyoreList.insert(eeyoreList.end(), t.second.begin(), t.second.end());
                eeyoreFuncCall->pushParam(t.first);
                // 传参
                eeyoreList.push_back(new EeyoreFuncParamNode(t.first));
            }
            eeyoreList.push_back(eeyoreFuncCall);
            break;
        }
        case ExpType::BinaryExp: {
            // 首先将两个参数分别extract
            assert(childNodes[0]->nodeType == NodeType::EXP);
            assert(childNodes[1]->nodeType == NodeType::OP_2);
            assert(childNodes[2]->nodeType == NodeType::EXP);
            auto opType = static_cast<Op2Node *>(childNodes[1])->opType;
            if(opType == OpType::opAnd)
                return extractEeyoreAndExp();
            else if(opType == OpType::opOr)
                return extractEeyoreOrExp();

            auto p1 = static_cast<ExpNode *>(childNodes[0])->extractEeyoreExp();
            eeyoreList.insert(eeyoreList.end(), p1.second.begin(), p1.second.end());
            auto p2 = static_cast<ExpNode *>(childNodes[2])->extractEeyoreExp();
            eeyoreList.insert(eeyoreList.end(), p2.second.begin(), p2.second.end());


            // 创建一个新的临时变量
            auto newTempName = "t" + to_string(tempValueCnt);
            tempValueCnt++;

            // 临时变量声明
            eeyoreList.push_back(new EeyoreVarDeclNode(newTempName));
            // 临时变量右边的exp
            EeyoreExpNode *expPtr;
            if(p1.first->isArray()) {
                assert(!p2.first->isArray());
                assert(opType == OpType::opPlus || opType == OpType::opDec);
                // 注册临时变量
                EeyoreBaseNode::registerTempSymbol(newTempName, true);
                // 将p2乘以4后再与p1操作
                if(p2.first->isNum())
                    expPtr = new EeyoreExpNode(opType, p1.first, new EeyoreRightValueNode(p2.first->getValue() * 4));
                else {
                    // 创建一个新的临时变量
                    auto tmpName = "t" + to_string(tempValueCnt);
                    tempValueCnt++;
                    // 注册临时变量
                    EeyoreBaseNode::registerTempSymbol(tmpName);
                    // 临时变量声明
                    eeyoreList.push_back(new EeyoreVarDeclNode(tmpName));
                    // 先进行一次赋值
                    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(tmpName),
                                                              new EeyoreExpNode(OpType::opMul, p2.first,
                                                                                new EeyoreRightValueNode(4))));
                    expPtr = new EeyoreExpNode(opType, p1.first, new EeyoreRightValueNode(tmpName));
                }
            } else if(p2.first->isArray()) {
                assert(!p1.first->isArray());
                assert(opType == OpType::opPlus || opType == OpType::opDec);
                // 注册临时变量
                EeyoreBaseNode::registerTempSymbol(newTempName, true);
                // 将p1乘以4后再与p2操作
                if(p1.first->isNum())
                    expPtr = new EeyoreExpNode(opType, new EeyoreRightValueNode(p1.first->getValue() * 4), p2.first);
                else {
                    // 创建一个新的临时变量
                    auto tmpName = "t" + to_string(tempValueCnt);
                    tempValueCnt++;
                    // 注册临时变量
                    EeyoreBaseNode::registerTempSymbol(tmpName);
                    // 临时变量声明
                    eeyoreList.push_back(new EeyoreVarDeclNode(tmpName));
                    // 先进行一次赋值
                    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(tmpName),
                                                              new EeyoreExpNode(OpType::opMul, p1.first,
                                                                                new EeyoreRightValueNode(4))));
                    expPtr = new EeyoreExpNode(opType, new EeyoreRightValueNode(tmpName), p2.first);
                }
            } else {
                // 注册临时变量
                EeyoreBaseNode::registerTempSymbol(newTempName);
                expPtr = new EeyoreExpNode(opType, p1.first, p2.first);
            }
            // 返回值
            rValPtr = new EeyoreRightValueNode(newTempName);
            eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(newTempName), expPtr));
            break;
        }
        case ExpType::UnaryExp: {
            assert(childNodes[0]->nodeType == NodeType::OP_1);
            assert(childNodes[1]->nodeType == NodeType::EXP);
            auto opType = static_cast<Op1Node *>(childNodes[0])->opType;
            auto p = static_cast<ExpNode *>(childNodes[1])->extractEeyoreExp();
            eeyoreList.insert(eeyoreList.end(), p.second.begin(), p.second.end());
            assert(!p.first->isArray());
            if(opType == OpType::opPlus) {
                rValPtr = p.first;
            } else {
                // 创建一个新的临时变量
                auto newTempName = "t" + to_string(tempValueCnt);
                tempValueCnt++;
                // 注册临时变量
                EeyoreBaseNode::registerTempSymbol(newTempName);
                // 临时变量声明
                eeyoreList.push_back(new EeyoreVarDeclNode(newTempName));
                // 返回值
                rValPtr = new EeyoreRightValueNode(newTempName);
                eeyoreList.push_back(
                        new EeyoreAssignNode(new EeyoreLeftValueNode(newTempName), new EeyoreExpNode(opType, p.first)));
            }
            break;
        }
    }

    return pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>>(rValPtr, move(eeyoreList));
}

pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>> ExpNode::extractEeyoreAndExp() {
    vector<EeyoreBaseNode *> eeyoreList;
    EeyoreRightValueNode *rValPtr;

    assert(childNodes[0]->nodeType == NodeType::EXP);
    assert(childNodes[1]->nodeType == NodeType::OP_2);
    assert(childNodes[2]->nodeType == NodeType::EXP);
    auto p1 = static_cast<ExpNode *>(childNodes[0])->extractEeyoreExp();
    auto p2 = static_cast<ExpNode *>(childNodes[2])->extractEeyoreExp();
    /*
     * t1 = a
     * if t1 == 0 goto l1:
     *     t2 = b    // 如果运行到这里，a肯定是1，所以a && b = b
     *     goto l2
     * l1:
     * t2 = 0       // 输出t2
     * l2:
     */
    // 创建一个临时变量
    auto tmp = "t" + to_string(tempValueCnt);
    tempValueCnt++;
    // 临时变量声明
    eeyoreList.push_back(new EeyoreVarDeclNode(tmp));
    // 注册临时变量
    EeyoreBaseNode::registerTempSymbol(tmp);

    // 申请两个label
    auto label1 = "l" + to_string(labelCnt);
    labelCnt++;
    auto label2 = "l" + to_string(labelCnt);
    labelCnt++;

    // 如果p1为0，直接返回0
    eeyoreList.push_back(new EeyoreCommentNode("// begin '&&' judgement"));
    eeyoreList.push_back(new EeyoreBlockBeginNode());
    // t1 = a
    eeyoreList.insert(eeyoreList.end(), p1.second.begin(), p1.second.end());
    eeyoreList.push_back(new EeyoreIfGotoNode(p1.first, label1));
    eeyoreList.push_back(new EeyoreCommentNode("// if the first operand is not false"));
    // 如果不为0，再进行判断
    eeyoreList.insert(eeyoreList.end(), p2.second.begin(), p2.second.end());
    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(tmp), p2.first));
    eeyoreList.push_back(new EeyoreGotoNode(label2));
    eeyoreList.push_back(new EeyoreLabelNode(label1));
    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(tmp), new EeyoreRightValueNode(0)));
    eeyoreList.push_back(new EeyoreLabelNode(label2));
    eeyoreList.push_back(new EeyoreBlockEndNode());
    eeyoreList.push_back(new EeyoreCommentNode("// end '&&' judgement"));
    // 返回的是一个右值
    rValPtr = new EeyoreRightValueNode(tmp);
    return pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>>(rValPtr, move(eeyoreList));
}

pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>> ExpNode::extractEeyoreOrExp() {
    vector<EeyoreBaseNode *> eeyoreList;
    EeyoreRightValueNode *rValPtr;

    assert(childNodes[0]->nodeType == NodeType::EXP);
    assert(childNodes[1]->nodeType == NodeType::OP_2);
    assert(childNodes[2]->nodeType == NodeType::EXP);
    auto p1 = static_cast<ExpNode *>(childNodes[0])->extractEeyoreExp();
    auto p2 = static_cast<ExpNode *>(childNodes[2])->extractEeyoreExp();

    /*
     * t1 = a
     * if t1 != 0 goto l1:
     *     t2 = b    // 如果运行到这里，a肯定是0，所以a || b = b
     *     goto l2
     * l1:
     * t2 = 1       // 输出t2
     * l2:
     */

    // 创建一个临时变量
    auto tmp = "t" + to_string(tempValueCnt);
    tempValueCnt++;
    // 临时变量声明
    eeyoreList.push_back(new EeyoreVarDeclNode(tmp));
    // 注册临时变量
    EeyoreBaseNode::registerTempSymbol(tmp);

    // 申请两个label
    auto label1 = "l" + to_string(labelCnt);
    labelCnt++;
    auto label2 = "l" + to_string(labelCnt);
    labelCnt++;

    // 如果p1为0，直接返回0
    eeyoreList.push_back(new EeyoreCommentNode("// begin '||' judgement"));
    eeyoreList.push_back(new EeyoreBlockBeginNode());
    // t1 = a
    eeyoreList.insert(eeyoreList.end(), p1.second.begin(), p1.second.end());
    eeyoreList.push_back(new EeyoreIfGotoNode(p1.first, label1, false));
    eeyoreList.push_back(new EeyoreCommentNode("// if the first operand is not false"));
    // 如果不为1，再进行判断
    eeyoreList.insert(eeyoreList.end(), p2.second.begin(), p2.second.end());
    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(tmp), p2.first));
    eeyoreList.push_back(new EeyoreGotoNode(label2));
    eeyoreList.push_back(new EeyoreLabelNode(label1));
    eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(tmp), new EeyoreRightValueNode(1)));
    eeyoreList.push_back(new EeyoreLabelNode(label2));
    eeyoreList.push_back(new EeyoreBlockEndNode());
    eeyoreList.push_back(new EeyoreCommentNode("// end '||' judgement"));
    // 返回的是一个右值
    rValPtr = new EeyoreRightValueNode(tmp);
    return pair<EeyoreRightValueNode *, vector<EeyoreBaseNode *>>(rValPtr, move(eeyoreList));
}


vector<EeyoreBaseNode *> VarDeclNode::generateEeyoreNode() {
//    assert(parentNodePtr->nodeType == NodeType::BLOCK);
    vector<EeyoreBaseNode *> eeyoreList;
    for(auto ptr:childNodes) {
        ptr->setParentPtr(this);
        assert(ptr->nodeType == NodeType::VAR_DEF);
        auto t = std::move(ptr->generateEeyoreNode());
        eeyoreList.insert(eeyoreList.end(), t.begin(), t.end());
    }
    return eeyoreList;
}

vector<EeyoreBaseNode *> VarDefNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;

    // 名字是newName
    // 新建一个VarDecl
    string newName = static_cast<IdentNode *>(childNodes[0])->id;
    bool isArray = childNodes[1]->childNodes.size() > 0;

    eeyoreList.push_back(new EeyoreVarDeclNode(newName));
    auto s = childNodes.size();

    if(isArray) {
        assert(childNodes[1]->childNodes[0]->nodeType == NodeType::EXP);
        assert(static_cast<ExpNode *>(childNodes[1]->childNodes[0])->expType == ExpType::Number);
        int dimNum = static_cast<NumberNode *>(childNodes[1]->childNodes[0]->childNodes[0])->num;
        // 三种情况：有初始化、全局、无初始化
        if(s == 3) {
            // 遍历初始化列表
            int initSize = childNodes[2]->childNodes.size();
            for(int i = 0; i < initSize; i++) {
                assert(childNodes[2]->childNodes[i]->childNodes[0]->nodeType == NodeType::EXP);
                auto expPtr = static_cast<ExpNode *>(childNodes[2]->childNodes[i]->childNodes[0]);
                // 解析exp
                auto t = expPtr->extractEeyoreExp();
                eeyoreList.insert(eeyoreList.end(), t.second.begin(), t.second.end());
                eeyoreList.push_back(
                        new EeyoreAssignNode(new EeyoreLeftValueNode(newName, new EeyoreRightValueNode(i * 4)),
                                             t.first));
            }
            // 注册
            if(parentNodePtr->parentNodePtr->nodeType == NodeType::ROOT)
                EeyoreBaseNode::registerGlobalArraySymbol(newName, dimNum, initSize);
            else
                EeyoreBaseNode::registerArraySymbol(newName, dimNum, initSize);
            // 完成初始化
            eeyoreList.push_back(new EeyoreFillInitNode(newName));
        } else if(parentNodePtr->parentNodePtr->nodeType == NodeType::ROOT) {
            // 否则，如果父节点是root，也要初始化
            EeyoreBaseNode::registerGlobalArraySymbol(newName, dimNum, 0);
            eeyoreList.push_back(new EeyoreFillInitNode(newName));
        } else {
            // 否则，不用初始化
            EeyoreBaseNode::registerArraySymbol(newName, dimNum);
        }
    } else {
        // 三种情况
        if(s == 3) {
            assert(childNodes[2]->childNodes[0]->nodeType == NodeType::EXP);
            auto expPtr = static_cast<ExpNode *>(childNodes[2]->childNodes[0]);
            // 解析exp
            auto t = expPtr->extractEeyoreExp();
            eeyoreList.insert(eeyoreList.end(), t.second.begin(), t.second.end());
            eeyoreList.push_back(
                    new EeyoreAssignNode(new EeyoreLeftValueNode(newName), t.first));
            // 注册
            if(parentNodePtr->parentNodePtr->nodeType == NodeType::ROOT)
                EeyoreBaseNode::registerGlobalSymbol(newName);
            else
                EeyoreBaseNode::registerSymbol(newName, 0);

        } else if(parentNodePtr->parentNodePtr->nodeType == NodeType::ROOT) {
            // 全局变量，直接初始化为0
            EeyoreBaseNode::registerGlobalSymbol(newName);
            eeyoreList.push_back(
                    new EeyoreAssignNode(new EeyoreLeftValueNode(newName), new EeyoreRightValueNode(0)));
        } else {
            EeyoreBaseNode::registerSymbol(newName);
        }
    }


    return eeyoreList;
}

vector<EeyoreBaseNode *> FuncDefNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;
    // 第一个是ident，最后一个是block
    int s = childNodes.size();
    // 函数定义
    auto funcDef = new EeyoreFuncDefNode(static_cast<IdentNode *>(childNodes[0])->id, isReturnTypeInt, s - 2);

    // 清空函数符号表
    EeyoreBaseNode::paramSymbolTable.clear();
    // 添加参数
    for(int i = 1; i < s - 1; i++) {
        bool isArray = childNodes[i]->childNodes.size() > 1;
        auto paramName = static_cast<IdentNode *>(childNodes[i]->childNodes[0])->id;
        EeyoreBaseNode::paramSymbolTable.insert(make_pair(paramName,
                                                          VarInfo(isArray, true, false, false, 0, 0, 0, false)));
    }
    funcDef->paramSymbolTable = EeyoreBaseNode::paramSymbolTable;
    // 添加函数体
    funcDef->childList = move(childNodes[s - 1]->generateEeyoreNode());
    // 判断最后一句是不是return
    if(funcDef->childList.size() == 0 || funcDef->childList.back()->nodeType != EeyoreNodeType::RETURN) {
        if(isReturnTypeInt)
            funcDef->childList.push_back(new EeyoreReturnNode(new EeyoreRightValueNode(0)));
        else
            funcDef->childList.push_back(new EeyoreReturnNode());
    }
    // 添加函数定义
    eeyoreList.push_back(funcDef);
    // 设置parent
    for(auto childPtr:funcDef->childList)
        childPtr->setParentPtr(funcDef);
    return eeyoreList;
}

vector<EeyoreBaseNode *> AssignNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;

    // 处理右边的exp
    auto rVal = static_cast<ExpNode *>(childNodes[1])->extractEeyoreExp();
    eeyoreList.insert(eeyoreList.end(), rVal.second.begin(), rVal.second.end());

    // 处理左值
    auto lValPtr = static_cast<LValNode *>(childNodes[0]);
    if(lValPtr->childNodes.size() == 1) {
        if(!eeyoreList.empty() &&
           eeyoreList.back()->nodeType == EeyoreNodeType::ASSIGN &&
           static_cast<EeyoreAssignNode *>(eeyoreList.back())->leftValue->name == rVal.first->name &&
           static_cast<EeyoreAssignNode *>(eeyoreList.back())->rightTerm->nodeType == EeyoreNodeType::EXP) {
            static_cast<EeyoreAssignNode *>(eeyoreList.back())->leftValue = new EeyoreLeftValueNode(
                    static_cast<IdentNode *>(childNodes[0]->childNodes[0])->id);
        } else {
            eeyoreList.push_back(new EeyoreAssignNode(
                    new EeyoreLeftValueNode(static_cast<IdentNode *>(childNodes[0]->childNodes[0])->id),
                    rVal.first));
        }

    } else {
        auto lValExp = static_cast<ExpNode *>(childNodes[0]->childNodes[1])->extractEeyoreExp();
        eeyoreList.insert(eeyoreList.end(), lValExp.second.begin(), lValExp.second.end());

        // 将左值括号中的结果乘以4
        if(lValExp.first->isNum()) {
            eeyoreList.push_back(new EeyoreAssignNode(
                    new EeyoreLeftValueNode(static_cast<IdentNode *>(childNodes[0]->childNodes[0])->id,
                                            new EeyoreRightValueNode(lValExp.first->getValue() * 4)),
                    rVal.first));
        } else {
            // 创建一个新的临时变量
            auto indexTempVar = "t" + to_string(tempValueCnt);
            tempValueCnt++;
            // 注册临时变量
            EeyoreBaseNode::registerTempSymbol(indexTempVar);
            // 临时变量声明
            eeyoreList.push_back(new EeyoreVarDeclNode(indexTempVar));
            // 添加表达式
            eeyoreList.push_back(new EeyoreAssignNode(new EeyoreLeftValueNode(indexTempVar),
                                                      new EeyoreExpNode(OpType::opMul, lValExp.first,
                                                                        new EeyoreRightValueNode(4))));

            eeyoreList.push_back(new EeyoreAssignNode(
                    new EeyoreLeftValueNode(static_cast<IdentNode *>(childNodes[0]->childNodes[0])->id,
                                            new EeyoreRightValueNode(indexTempVar)),
                    rVal.first));
        }
    }

    return eeyoreList;
}

vector<EeyoreBaseNode *> ExpStmtNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;
    // 函数调用
    if(static_cast<ExpNode *>(childNodes[0])->expType == ExpType::FuncCall) {
        // 得到函数调用节点
        auto syFuncCall = static_cast<FuncCallNode *>(childNodes[0]->childNodes[0]);
        // 函数名
        string funcName = static_cast<IdentNode *>(syFuncCall->childNodes[0])->id;
        // 首先创建一个新的FuncCall, 没有返回值（否则会被解析成赋值）
        auto funcCall = new EeyoreFuncCallNode(funcName);
        // 生成每一个参数
        for(int i = 1; i < syFuncCall->childNodes.size(); i++) {
            assert(syFuncCall->childNodes[i]->nodeType == NodeType::EXP);
            auto t = static_cast<ExpNode *>(syFuncCall->childNodes[i])->extractEeyoreExp();
            eeyoreList.insert(eeyoreList.end(), t.second.begin(), t.second.end());
            funcCall->pushParam(t.first);
            eeyoreList.push_back(new EeyoreFuncParamNode(t.first));
        }
        eeyoreList.push_back(funcCall);
    } else {
        auto t = static_cast<ExpNode *>(childNodes[0])->extractEeyoreExp();
        eeyoreList.insert(eeyoreList.end(), t.second.begin(), t.second.end());
    }
    return eeyoreList;
}

vector<EeyoreBaseNode *> IfGotoNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;
    eeyoreList.push_back(new EeyoreCommentNode("// begin if (else)"));
    int s = childNodes.size();
    assert(childNodes[0]->nodeType == NodeType::EXP);
    auto expPtr = static_cast<ExpNode *>(childNodes[0]);
    auto cond = expPtr->extractEeyoreExp();
    // 条件前的计算
    eeyoreList.insert(eeyoreList.begin(), cond.second.begin(), cond.second.end());
    // 添加条件
    eeyoreList.push_back(new EeyoreIfGotoNode(cond.first, endIfLabel));
    // 开始if body
    eeyoreList.push_back(new EeyoreBlockBeginNode());
    auto ifBody = childNodes[1]->generateEeyoreNode();
    eeyoreList.insert(eeyoreList.end(), ifBody.begin(), ifBody.end());
    eeyoreList.push_back(new EeyoreBlockEndNode());
    // 判断是否有else
    if(s == 3) {
        eeyoreList.push_back(new EeyoreGotoNode(endElseLabel));
    }
    // if判断结束
    eeyoreList.push_back(new EeyoreCommentNode("// if failed!"));
    eeyoreList.push_back(new EeyoreLabelNode(endIfLabel));
    // else 的 body
    if(s == 3) {
        auto elseBody = childNodes[2]->generateEeyoreNode();
        eeyoreList.push_back(new EeyoreBlockBeginNode());
        eeyoreList.insert(eeyoreList.end(), elseBody.begin(), elseBody.end());
        eeyoreList.push_back(new EeyoreBlockEndNode());
        eeyoreList.push_back(new EeyoreLabelNode(endElseLabel));
    }
    eeyoreList.push_back(new EeyoreCommentNode("// end if (else)"));

    return eeyoreList;
}

vector<EeyoreBaseNode *> WhileGotoNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;
    eeyoreList.push_back(new EeyoreCommentNode("// begin while"));
    eeyoreList.push_back(new EeyoreLabelNode(beginLabel));
    assert(childNodes[0]->nodeType == NodeType::EXP);
    auto expPtr = static_cast<ExpNode *>(childNodes[0]);
    auto cond = expPtr->extractEeyoreExp();
    eeyoreList.insert(eeyoreList.end(), cond.second.begin(), cond.second.end());
    // 添加条件
    // 判断条件是否恒真或恒假
    if(cond.first->isNum()) {
        int n = cond.first->getValue();
        // 恒真
        if(n == 0)
            eeyoreList.push_back(new EeyoreGotoNode(endLabel));
        // 否则，不插入
    } else
        eeyoreList.push_back(new EeyoreIfGotoNode(cond.first, endLabel));
    // block
    eeyoreList.push_back(new EeyoreBlockBeginNode());
    auto whileBody = childNodes[1]->generateEeyoreNode();
    eeyoreList.insert(eeyoreList.end(), whileBody.begin(), whileBody.end());
    // end block
    eeyoreList.push_back(new EeyoreBlockEndNode());
    eeyoreList.push_back(new EeyoreLabelNode(endLabel));
    eeyoreList.push_back(new EeyoreCommentNode("// end while"));
    return eeyoreList;
}

vector<EeyoreBaseNode *> GotoNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;
    eeyoreList.push_back(new EeyoreGotoNode(label));
    return eeyoreList;
}

vector<EeyoreBaseNode *> ReturnNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;
    if(childNodes.size() == 0)
        eeyoreList.push_back(new EeyoreReturnNode());
    else {
        assert(childNodes[0]->nodeType == NodeType::EXP);
        auto returnValue = static_cast<ExpNode *>(childNodes[0])->extractEeyoreExp();
        eeyoreList.insert(eeyoreList.end(), returnValue.second.begin(), returnValue.second.end());
        eeyoreList.push_back(new EeyoreReturnNode(returnValue.first));
    }
    return eeyoreList;
}

vector<EeyoreBaseNode *> BlockNode::generateEeyoreNode() {
    if(parentNodePtr->nodeType != NodeType::FUNC_DEF)
        return BaseNode::generateEeyoreNode();

    vector<EeyoreBaseNode *> eeyoreList;
    vector<EeyoreBaseNode *> tempList;
    for(auto ptr:childNodes) {
        auto t = ptr->generateEeyoreNode();
        for(auto node:t) {
            if(node->nodeType == EeyoreNodeType::VAR_DECL)
                eeyoreList.push_back(node);
            else
                tempList.push_back(node);
        }
    }
    eeyoreList.insert(eeyoreList.end(), tempList.begin(), tempList.end());

    return eeyoreList;
}

vector<EeyoreBaseNode *> RootNode::generateEeyoreNode() {

    vector<EeyoreBaseNode *> eeyoreList;

    EeyoreFuncDefNode *mainPtr;
    auto globalInitNode = new EeyoreGlobalInitNode();
    vector<EeyoreBaseNode *> assignList;
    vector<EeyoreBaseNode *> funcDefList;

    assignList.push_back(new EeyoreCommentNode("// begin global var init"));
    auto rootNode = new EeyoreRootNode();

    for(auto ptr:childNodes) {
        auto t = ptr->generateEeyoreNode();
        for(auto node:t) {
            node->setParentPtr(rootNode);
            if(node->nodeType == EeyoreNodeType::FUNC_DEF &&
               static_cast<EeyoreFuncDefNode *>(node)->funcName == "f_main")
                mainPtr = static_cast<EeyoreFuncDefNode *>(node);
            else if(node->nodeType == EeyoreNodeType::ASSIGN || node->nodeType == EeyoreNodeType::FILL_INIT)
                assignList.push_back(node);
            else if(node->nodeType == EeyoreNodeType::FUNC_DEF)
                funcDefList.push_back(node);
            else
                rootNode->childList.push_back(node);
        }
    }
    assignList.push_back(new EeyoreCommentNode("// end global var init"));
    // 在main开头插入初始化
    globalInitNode->childList = move(assignList);
    // 找到第一个非decl的node
    auto noneDeclIt = find_if(mainPtr->childList.begin(), mainPtr->childList.end(),
                              [](EeyoreBaseNode *t) {
                                  return t->nodeType != EeyoreNodeType::VAR_DECL;
                              });

    mainPtr->childList.insert(noneDeclIt, globalInitNode->childList.begin(), globalInitNode->childList.end());
    rootNode->childList.insert(rootNode->childList.end(), funcDefList.begin(), funcDefList.end());
    rootNode->childList.push_back(mainPtr);
    eeyoreList.push_back(rootNode);

    return eeyoreList;
}

vector<EeyoreBaseNode *> ContinueNode::generateEeyoreNode() {
    assert(hasLabel);
    vector<EeyoreBaseNode *> eeyoreList;
    eeyoreList.push_back(new EeyoreGotoNode(label));
    return eeyoreList;
}

vector<EeyoreBaseNode *> BreakNode::generateEeyoreNode() {
    assert(hasLabel);
    vector<EeyoreBaseNode *> eeyoreList;
    eeyoreList.push_back(new EeyoreGotoNode(label));
    return eeyoreList;
}