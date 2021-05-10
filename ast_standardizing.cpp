//
// Created by KiNAmi on 2021/5/6.
//
#include "ast.hpp"

unsigned int BaseNode::definedValueCnt = 0;
unsigned int BaseNode::tempValueCnt = 0;
unsigned int BaseNode::labelCnt = 0;
vector<ContinueNode *> ContinueNode::currentContinueNodeList;
vector<BreakNode *> BreakNode::currentBreakNodeList;

/*
 * override standardizing()
 */

void BlockNode::standardizing() {
    BaseNode::standardizing();

    NodePtrList newChilds;
    for(auto ptr:childNodes) {
        auto t = ptr->extractStmtNode();
        newChilds.insert(newChilds.end(), t.begin(), t.end());
    }
    childNodes = newChilds;
    for(auto ptr:childNodes) {
        ptr->setParentPtr(this);
    }
}

/*
 * override getParentSymbolTable();
 */

void RootNode::getParentSymbolTable() {
    symbolTablePtr = make_shared<map<string, AbstractValNode *>>();
}

void BlockNode::getParentSymbolTable() {
    if(parentNodePtr->nodeType == NodeType::FUNC_DEF)
        symbolTablePtr = parentNodePtr->symbolTablePtr;
    else {
        symbolTablePtr = make_shared<map<string, AbstractValNode *>>();
        symbolTablePtr->insert(parentNodePtr->symbolTablePtr->begin(), parentNodePtr->symbolTablePtr->end());
    }
}

void FuncDefNode::getParentSymbolTable() {
    symbolTablePtr = make_shared<map<string, AbstractValNode *>>();
    symbolTablePtr->insert(parentNodePtr->symbolTablePtr->begin(), parentNodePtr->symbolTablePtr->end());
}

/*
 * override replaceSymbols()
 */

int AbstractValNode::getConstValue(vector<int> &dims) {
    assert(isConst && isArray);
    assert(dims.size() == arrayDims.size());
    int idx = getIndex(dims);
    return idx < arrayValues.size() ? arrayValues[idx] : 0;
}

int AbstractValNode::getIndex(vector<int> &dims) {
    assert(isConst && isArray);
    assert(dims.size() == arrayDims.size());
    int s = (int) dims.size();
    int idx = dims[0];
    for(int i = 1; i < s; i++)
        idx = idx * arrayDims[i] + dims[i];
    return idx;
}

int AbstractValNode::getConstValue() {
    assert(isConst && !isArray);
    return constVal;
}

/*
 * where IdentNode appears: VarDefNode, FuncDefNode, ArgumentNode, FuncCallNode, LVal
 * we only replace func ident and argument here, as LVal may be replaced by a const num
 */
void IdentNode::replaceSymbols() {
    if(parentNodePtr->nodeType == NodeType::FUNC_DEF || parentNodePtr->nodeType == NodeType::FUNC_CALL) {
        if(id != "getint" && id != "getch" && id != "getarray" && id != "putint" && id != "putch" && id != "putarray"
           && id != "starttime" && id != "stoptime")
            id = "f_" + id;
    } else if(parentNodePtr->nodeType == NodeType::ARGUMENT || parentNodePtr->nodeType == NodeType::VAR_DEF) {
        assert(symbolTablePtr->count(id) > 0);
        id = (*symbolTablePtr)[id]->newName;
    }
}

void LValNode::replaceSymbols() {
    auto identName = static_cast<IdentNode *>(childNodes[0])->id;
    assert(symbolTablePtr->count(identName) > 0);
    auto valPtr = (*symbolTablePtr)[identName];

    // judge if this LVal is a const num, return if not
    isConst = false;
    if(!valPtr->isConst)
        return;
    vector<int> dims;
    int lValChidlNum = static_cast<int>(childNodes.size());
    for(int i = 1; i < lValChidlNum; i++) {
        assert(childNodes[i]->nodeType == NodeType::EXP);
        if(static_cast<ExpNode *>(childNodes[i])->expType != ExpType::Number)
            return;
        dims.push_back(static_cast<NumberNode *>(childNodes[i]->childNodes[0])->num);
    }
    if(dims.size() < valPtr->arrayDims.size())
        return;

    isConst = true;
    static_cast<IdentNode *>(childNodes[0])->id = valPtr->newName;
    assert(parentNodePtr->nodeType == NodeType::EXP);
    auto parentExpPtr = static_cast<ExpNode *>(parentNodePtr);
    assert(static_cast<ExpNode *>(parentNodePtr)->expType == ExpType::LVal);

    parentExpPtr->childNodes.clear();
    parentExpPtr->setExpType(ExpType::Number);
    if(lValChidlNum == 1)
        parentExpPtr->childNodes.push_back(new NumberNode(valPtr->getConstValue()));
    else
        parentExpPtr->childNodes.push_back(new NumberNode(valPtr->getConstValue(dims)));
}


/*
 * override updateSymbolTable()
 */

void VarDeclNode::updateSymbolTable(BaseNode *ptr) {
    assert(parentNodePtr->nodeType == NodeType::ROOT || parentNodePtr->nodeType == NodeType::BLOCK);
    assert(ptr->nodeType == NodeType::VAR_DEF);
    auto varDefPtr = (VarDefNode *) ptr;
    varDefPtr->newName = "T" + to_string(definedValueCnt);
    definedValueCnt++;
    symbolTablePtr->insert_or_assign(((IdentNode *) varDefPtr->childNodes[0])->id, varDefPtr);
}

void FuncDefNode::updateSymbolTable(BaseNode *ptr) {
    assert(ptr->nodeType == NodeType::IDENT || ptr->nodeType == NodeType::BLOCK || ptr->nodeType == NodeType::ARGUMENT);
    if(ptr->nodeType == NodeType::BLOCK || ptr->nodeType == NodeType::IDENT)
        return;
    auto *argPtr = (ArgumentNode *) ptr;
    argPtr->newName = "p" + to_string(paramCnt);
    paramCnt++;
    symbolTablePtr->insert_or_assign(((IdentNode *) argPtr->childNodes[0])->id, argPtr);
}

/*
 * override equivalentlyTransform()
 */

void VarDefNode::equivalentlyTransform() {
    flattenArray();
}

void ArgumentNode::equivalentlyTransform() {
    if(!isArray)
        return;
    arrayDims.push_back(0);
    for(int i = 0; i < childNodes[1]->childNodes.size(); i++) {
        assert(childNodes[1]->childNodes[i]->nodeType == NodeType::EXP);
        assert(((ExpNode *) (childNodes[1]->childNodes[i]))->expType == ExpType::Number);
        auto numPtr = (NumberNode *) (childNodes[1]->childNodes[i]->childNodes[0]);
        arrayDims.push_back(numPtr->num);
    }
    childNodes[1]->childNodes.clear();
}

void ExpNode::equivalentlyTransform() {
    switch(expType) {
        case ExpType::BinaryExp:
//            childNodes[0]->evalNow();
//            childNodes[2]->evalNow();
            if(((ExpNode *) childNodes[0])->isConst() && ((ExpNode *) childNodes[2])->isConst())
                evalBinary();
            break;
        case ExpType::UnaryExp:
//            childNodes[1]->evalNow();
            if(((ExpNode *) childNodes[1])->isConst())
                evalUnary();
            break;
        default:
            break;
    }
}

void LValNode::equivalentlyTransform() {
    if(isConst)
        return;
    assert(childNodes[0]->nodeType == NodeType::IDENT);
    auto idPtr = static_cast<IdentNode *>(childNodes[0]);
    assert(symbolTablePtr->count(idPtr->id) > 0);
    auto valPtr = (*symbolTablePtr)[idPtr->id];
    idPtr->id = valPtr->newName;

    if(valPtr->arrayDims.size() + 1 == childNodes.size()) {
        if(childNodes.size() > 2) {
            assert(childNodes.size() - 1 == valPtr->arrayDims.size());
            assert(childNodes[1]->nodeType == NodeType::EXP);
            NodePtr expNode = childNodes[1];
            for(int i = 2; i < childNodes.size(); i++) {
                NodePtr t = new ExpNode(ExpType::BinaryExp);
                t->pushNodePtr(expNode);
                t->pushNodePtr(new Op2Node(OpType::opMul));
                t->pushNodePtr(new ExpNode(ExpType::Number, valPtr->arrayDims[i - 1]));
                t->equivalentlyTransform();
                expNode = t;
                t = new ExpNode(ExpType::BinaryExp);
                t->pushNodePtr(expNode);
                t->pushNodePtr(new Op2Node(OpType::opPlus));
                t->pushNodePtr(childNodes[i]);
                t->equivalentlyTransform();
                expNode = t;

            }
            NodePtrList newNodes;
            newNodes.push_back(childNodes[0]);
            newNodes.push_back(expNode);
            childNodes = newNodes;
        }
    } else {
        int arrayDimNum = static_cast<int>(valPtr->arrayDims.size());
        int valDimNum = static_cast<int>(childNodes.size()) - 1;
        assert(arrayDimNum > valDimNum);
        if(valDimNum == 0)
            return;
        int valDimSize = 1;
        for(int i = valDimNum; i < arrayDimNum; i++)
            valDimSize *= valPtr->arrayDims[i];
        NodePtr expNode = childNodes[1];
        for(int i = 2; i < childNodes.size(); i++) {
            NodePtr t = new ExpNode(ExpType::BinaryExp);
            t->pushNodePtr(expNode);
            t->pushNodePtr(new Op2Node(OpType::opMul));
            t->pushNodePtr(new ExpNode(ExpType::Number, valPtr->arrayDims[i - 1]));
            t->equivalentlyTransform();
            expNode = t;
            t = new ExpNode(ExpType::BinaryExp);
            t->pushNodePtr(expNode);
            t->pushNodePtr(new Op2Node(OpType::opPlus));
            t->pushNodePtr(childNodes[i]);
            t->equivalentlyTransform();
            expNode = t;
        }
        NodePtr t;
        t = new ExpNode(ExpType::BinaryExp);
        t->pushNodePtr(expNode)
                ->pushNodePtr(new Op2Node(OpType::opMul))
                ->pushNodePtr(new ExpNode(ExpType::Number, valDimSize));
        t->equivalentlyTransform();
        expNode = t;
        NodePtr newLValNode = (new LValNode())->pushNodePtr(childNodes[0]);
        assert(parentNodePtr->nodeType == NodeType::EXP);
        static_cast<ExpNode *>(parentNodePtr)->setExpType(ExpType::BinaryExp);
        parentNodePtr->childNodes.clear();
        parentNodePtr->pushNodePtr((new ExpNode(ExpType::LVal))->pushNodePtr(newLValNode))
                ->pushNodePtr(new Op2Node(OpType::opPlus))
                ->pushNodePtr(expNode);
        parentNodePtr->print();
    }
}


/*
 * override extractStmtNode()
 */

pair<NodePtr, NodePtrList> ExpNode::extractExp() {
    NodePtrList stmts;
    NodePtr rst = this;
    switch(expType) {
        case ExpType::Number:
            rst = this;
            break;
        case ExpType::LVal: {
            if(childNodes[0]->childNodes.size() == 1)
                rst = this;
            else {
                assert(childNodes[0]->childNodes[1]->nodeType == NodeType::EXP);
                // 将左值的括号中的exp解为若干个赋值语句和一个exp
                auto t = static_cast<ExpNode *>(childNodes[0]->childNodes[1])->extractExp();
                // 左值的括号中的内容修改
                assert(t.first->nodeType == NodeType::EXP);
                childNodes[0]->childNodes[1] = t.first;
                // 赋值语句添加到最终返回的stmt中
                stmts = move(t.second);
                // 创建一个新的临时变量
                auto newTempName = "t" + to_string(tempValueCnt);
                tempValueCnt++;
                // 声明临时变量
                stmts.push_back(new VarDeclNode(newTempName));
                // 为临时变量赋值
                stmts.push_back(new AssignNode(newTempName, this));
                // 返回的操作数应该是一个exp包裹着新生成的临时变量的左值node
                rst = new ExpNode(newTempName);
            }
            break;
        }
        case ExpType::FuncCall: {
            // 首先创建一个新的FuncCall
            auto newFuncCallPtr = new FuncCallNode();
            newFuncCallPtr->pushNodePtr(childNodes[0]->childNodes[0]);
            // 生成每一个参数
            for(int i = 1; i < childNodes[0]->childNodes.size(); i++) {
                assert(childNodes[0]->childNodes[i]->nodeType == NodeType::EXP);
                auto tExpExtract = static_cast<ExpNode *>(childNodes[0]->childNodes[i])->extractExp();
                stmts.insert(stmts.end(), tExpExtract.second.begin(), tExpExtract.second.end());
                assert(tExpExtract.first->nodeType == NodeType::EXP);
                newFuncCallPtr->pushNodePtr(tExpExtract.first);
            }
            // 创建一个新的临时变量
            auto newTempName = "t" + to_string(tempValueCnt);
            tempValueCnt++;
            // 声明临时变量
            stmts.push_back(new VarDeclNode(newTempName));
            // 为临时变量赋值
            stmts.push_back(new AssignNode(newTempName,
                                           (new ExpNode(ExpType::FuncCall))->pushNodePtr(newFuncCallPtr)));
            // 返回的操作数应该是一个exp包裹着新生成的临时变量的左值node
            rst = new ExpNode(newTempName);
            break;
        }
        case ExpType::BinaryExp: {
            // 首先将两个参数分别extract
            assert(childNodes[0]->nodeType == NodeType::EXP);
            assert(childNodes[2]->nodeType == NodeType::EXP);
            auto p1 = static_cast<ExpNode *>(childNodes[0])->extractExp();
            stmts.insert(stmts.end(), p1.second.begin(), p1.second.end());
            auto p2 = static_cast<ExpNode *>(childNodes[2])->extractExp();
            stmts.insert(stmts.end(), p2.second.begin(), p2.second.end());
            // 创建一个新的临时变量
            auto newTempName = "t" + to_string(tempValueCnt);
            tempValueCnt++;
            // 声明临时变量
            stmts.push_back(new VarDeclNode(newTempName));
            // 为临时变量赋值
            stmts.push_back(new AssignNode(newTempName,
                                           (new ExpNode(ExpType::BinaryExp))
                                                   ->pushNodePtr(p1.first)
                                                   ->pushNodePtr(childNodes[1])
                                                   ->pushNodePtr(p2.first)));
            // 返回的操作数应该是一个exp包裹着新生成的临时变量的左值node
            rst = new ExpNode(newTempName);
            break;
        }
        case ExpType::UnaryExp: {
            assert(childNodes[1]->nodeType == NodeType::EXP);
            auto p = static_cast<ExpNode *>(childNodes[1])->extractExp();
            stmts.insert(stmts.end(), p.second.begin(), p.second.end());

            // 创建一个新的临时变量
            auto newTempName = "t" + to_string(tempValueCnt);
            tempValueCnt++;
            // 声明临时变量
            stmts.push_back(new VarDeclNode(newTempName));
            // 为临时变量赋值
            stmts.push_back(new AssignNode(newTempName,
                                           (new ExpNode(ExpType::UnaryExp))
                                                   ->pushNodePtr(childNodes[0])
                                                   ->pushNodePtr(p.first)));
            // 返回的操作数应该是一个exp包裹着新生成的临时变量的左值node
            rst = new ExpNode(newTempName);
            break;
        }
    }

    return pair<NodePtr, NodePtrList>({rst, stmts});
}

NodePtrList BlockNode::extractStmtNode() {
    return childNodes;
}

NodePtrList VarDeclNode::extractStmtNode() {
    assert(parentNodePtr->nodeType == NodeType::BLOCK);
    NodePtrList newChildList;
    for(auto ptr:childNodes) {
        assert(ptr->nodeType == NodeType::VAR_DEF);
        auto t = std::move(ptr->extractStmtNode());
        newChildList.insert(newChildList.end(), t.begin(), t.end());
    }
    return newChildList;
}

NodePtrList VarDefNode::extractStmtNode() {
    NodePtrList newChildList;

    // generate new VarDefNode
    auto defPtr = new VarDefNode(false);
    defPtr->pushNodePtr(new IdentNode(newName));
    defPtr->pushNodePtr(childNodes[1]);

    if(childNodes.size() == 2) {
        newChildList.push_back((new VarDeclNode(false))->pushNodePtr(defPtr));
        return newChildList;
    }

    if(isArray)
        defPtr->pushNodePtr((new InitValNode(true))
                                    ->pushNodePtr((new InitValNode(false))
                                                          ->pushNodePtr(new ExpNode(ExpType::Number, 0))));
    else
        defPtr->pushNodePtr((new InitValNode(false))
                                    ->pushNodePtr(new ExpNode(ExpType::Number, 0)));

    // push new VarDeclNode
    newChildList.push_back((new VarDeclNode(false))->pushNodePtr(defPtr));

    // generate AssignNode
    if(childNodes.size() == 2)
        return newChildList;

    if(!isArray) {
        auto expPtr = static_cast<ExpNode *>(childNodes[2]->childNodes[0]);
        if(!(expPtr->expType == ExpType::Number && static_cast<NumberNode *>(expPtr->childNodes[0])->num == 0)) {
            auto assignPtr = (new AssignNode())
                    ->pushNodePtr((new LValNode())
                                          ->pushNodePtr(new IdentNode(newName)))
                    ->pushNodePtr(childNodes[2]->childNodes[0]);
            auto t = move(assignPtr->extractStmtNode());
            newChildList.insert(newChildList.end(), t.begin(), t.end());
        }
    } else {
        for(int i = 0; i < childNodes[2]->childNodes.size(); i++) {
            assert(childNodes[2]->childNodes[i]->childNodes[0]->nodeType == NodeType::EXP);
            auto expPtr = static_cast<ExpNode *>(childNodes[2]->childNodes[i]->childNodes[0]);
            if(expPtr->expType == ExpType::Number && static_cast<NumberNode *>(expPtr->childNodes[0])->num == 0)
                continue;
            auto assignPtr = (new AssignNode())
                    ->pushNodePtr((new LValNode())
                                          ->pushNodePtr(new IdentNode(newName))
                                          ->pushNodePtr(new ExpNode(ExpType::Number, i)))
                    ->pushNodePtr(childNodes[2]->childNodes[i]);
            auto t = move(assignPtr->extractStmtNode());
            newChildList.insert(newChildList.end(), t.begin(), t.end());
        }
    }

    return newChildList;
}

NodePtrList AssignNode::extractStmtNode() {
    auto t = static_cast<ExpNode *>(childNodes[1])->extractExp();
    NodePtrList stmtList = move(t.second);
    stmtList.push_back((new AssignNode())
                               ->pushNodePtr(childNodes[0])
                               ->pushNodePtr(t.first));
    return stmtList;
}

NodePtrList ReturnNode::extractStmtNode() {
    assert(parentNodePtr->nodeType == NodeType::BLOCK);
    if(childNodes.empty())
        return vector<NodePtr>({this});
    auto t = static_cast<ExpNode *>(childNodes[0])->extractExp();
    auto newChildList = move(t.second);
    newChildList.push_back((new ReturnNode())->pushNodePtr(t.first));
    return newChildList;
}

NodePtrList ExpStmtNode::extractStmtNode() {
    assert(parentNodePtr->nodeType == NodeType::BLOCK);
    if(static_cast<ExpNode *>(childNodes[0])->expType == ExpType::FuncCall)
        return vector<NodePtr>({this});

    auto t = static_cast<ExpNode *>(childNodes[0])->extractExp();
    auto newChildList = move(t.second);
    bool hasFuncCall = false;
    // 优化掉没有副作用的纯表达式语句
    for(auto ptr : newChildList)
        if(ptr->nodeType == NodeType::ASSIGN &&
           static_cast<ExpNode *>(ptr->childNodes[1])->expType == ExpType::FuncCall) {
            hasFuncCall = true;
            break;
        }
    if(hasFuncCall) {
        newChildList.push_back((new ExpStmtNode())->pushNodePtr(t.first));
        return newChildList;
    }
    return vector<NodePtr>({});
}

NodePtrList IfNode::extractStmtNode() {
    assert(parentNodePtr->nodeType == NodeType::BLOCK);
    assert(childNodes[0]->childNodes[0]->nodeType == NodeType::EXP);
    assert(childNodes[1]->nodeType == NodeType::BLOCK);
    auto t = static_cast<ExpNode *>(childNodes[0]->childNodes[0])->extractExp();
    auto newChildList = move(t.second);

    if(childNodes.size() == 2) {
        auto newLabel = "l" + to_string(labelCnt);
        labelCnt++;

        NodePtrList tVector;
        for(auto ptr: childNodes[1]->childNodes) {
            if(ptr->nodeType == NodeType::VAL_DECL)
                newChildList.push_back(ptr);
            else
                tVector.push_back(ptr);
        }
        newChildList.push_back((new IfGotoNode(newLabel))->pushNodePtr(t.first)
                                       ->pushNodePtr((new BlockNode())->pushNodePtrList(tVector)));

    } else {
        auto newIfLabel = "l" + to_string(labelCnt);
        labelCnt++;
        auto newElseLabel = "l" + to_string(labelCnt);
        labelCnt++;

        NodePtrList tVector1;
        for(auto ptr: childNodes[1]->childNodes) {
            if(ptr->nodeType == NodeType::VAL_DECL)
                newChildList.push_back(ptr);
            else
                tVector1.push_back(ptr);
        }

        NodePtrList tVector2;
        for(auto ptr: childNodes[2]->childNodes) {
            if(ptr->nodeType == NodeType::VAL_DECL)
                newChildList.push_back(ptr);
            else
                tVector2.push_back(ptr);
        }

        newChildList.push_back((new IfGotoNode(newIfLabel, newElseLabel))->pushNodePtr(t.first)
                                       ->pushNodePtr((new BlockNode())->pushNodePtrList(tVector1))
                                       ->pushNodePtr((new BlockNode())->pushNodePtrList(tVector2)));
    }
    return newChildList;
}

NodePtrList WhileNode::extractStmtNode() {
//    return BaseNode::extractStmtNode();
    assert(parentNodePtr->nodeType == NodeType::BLOCK);
    assert(childNodes[0]->childNodes[0]->nodeType == NodeType::EXP);
    assert(childNodes[1]->nodeType == NodeType::BLOCK);
    auto t = static_cast<ExpNode *>(childNodes[0]->childNodes[0])->extractExp();


    auto newBeginLabel = "l" + to_string(labelCnt);
    labelCnt++;
    auto newEndLabel = "l" + to_string(labelCnt);
    labelCnt++;

    NodePtrList newChildList;
    auto whileGotoPtr = new WhileGotoNode(newBeginLabel, newEndLabel);

    // 处理
    for(auto ptr: t.second) {
        if(ptr->nodeType == NodeType::VAL_DECL)
            newChildList.push_back(ptr);
        else
            whileGotoPtr->pushNodePtr(ptr);
    }
    // 和if的处理方法相同
    NodePtrList tVector;
    for(auto ptr: childNodes[1]->childNodes) {
        if(ptr->nodeType == NodeType::VAL_DECL)
            newChildList.push_back(ptr);
        else
            tVector.push_back(ptr);
    }
    tVector.push_back(new GotoNode(newBeginLabel));
    whileGotoPtr->pushNodePtr((new IfGotoNode(newEndLabel))->pushNodePtr(t.first)
                                      ->pushNodePtr((new BlockNode())->pushNodePtrList(tVector)));
    newChildList.push_back(whileGotoPtr);

    // 设置continue
    if(!ContinueNode::currentContinueNodeList.empty()) {
        vector<ContinueNode *> newList;
        for(auto continuePtr: ContinueNode::currentContinueNodeList) {
            NodePtr ptr = continuePtr;
            assert(ptr->parentNodePtr != nullptr);
            bool isChild = false;
            while(ptr->nodeType != NodeType::ROOT) {
                assert(ptr->parentNodePtr != nullptr);
                ptr = ptr->parentNodePtr;
                if(ptr == this) {
                    isChild = true;
                    break;
                }
            }
            if(isChild)
                continuePtr->setLabel(newBeginLabel);
            else
                newList.push_back(continuePtr);
        }
        ContinueNode::currentContinueNodeList = move(newList);
    }

    // 设置break
    if(!BreakNode::currentBreakNodeList.empty()) {
        vector<BreakNode *> newList;
        for(auto breakPtr: BreakNode::currentBreakNodeList) {
            NodePtr ptr = breakPtr;
            assert(ptr->parentNodePtr != nullptr);
            bool isChild = false;
            while(ptr->nodeType != NodeType::ROOT) {
                assert(ptr->parentNodePtr != nullptr);
                ptr = ptr->parentNodePtr;
                if(ptr == this) {
                    isChild = true;
                    break;
                }
            }
            if(isChild)
                breakPtr->setLabel(newEndLabel);
            else
                newList.push_back(breakPtr);
        }
        BreakNode::currentBreakNodeList = move(newList);
    }

    return newChildList;
}

NodePtrList ContinueNode::extractStmtNode() {
    currentContinueNodeList.push_back(this);
    return BaseNode::extractStmtNode();
}

NodePtrList BreakNode::extractStmtNode() {
    currentBreakNodeList.push_back(this);
    return BaseNode::extractStmtNode();
}

