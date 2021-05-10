//
// Created by KiNAmi on 2021/5/6.
//
#include "ast.hpp"

unsigned int BaseNode::definedValueCnt = 0;
unsigned int BaseNode::tempValueCnt = 0;

/*
 * override standardizing()
 */

//void BlockNode::standardizing() {
//    getParentSymbolTable();
//    replaceSymbols();
//    for (auto it = childNodes.begin(); it != childNodes.end();) {
//        updateSymbolTable((*it));
//        (*it)->setParentPtr(this);
//        (*it)->standardizing();
//        extractStmtNode(it);    // trick: update it here!
//    }
//    equivalentlyTransform();
//}

/*
 * override getParentSymbolTable();
 */

void RootNode::getParentSymbolTable() {
    symbolTablePtr = make_shared<map<string, AbstractValNode *>>();
}

void BlockNode::getParentSymbolTable() {
    if (parentNodePtr->nodeType == NodeType::FUNC_DEF)
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
    for (int i = 1; i < s; i++)
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
    if (parentNodePtr->nodeType == NodeType::FUNC_DEF || parentNodePtr->nodeType == NodeType::FUNC_CALL) {
        if (id != "getint" && id != "getch" && id != "getarray" && id != "putint" && id != "putch" && id != "putarray"
            && id != "starttime" && id != "stoptime")
            id = "f_" + id;
    } else if (parentNodePtr->nodeType == NodeType::ARGUMENT || parentNodePtr->nodeType == NodeType::VAR_DEF) {
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
    if (!valPtr->isConst)
        return;
    vector<int> dims;
    int lValChidlNum = static_cast<int>(childNodes.size());
    for (int i = 1; i < lValChidlNum; i++) {
        assert(childNodes[i]->nodeType == NodeType::EXP);
        if (static_cast<ExpNode *>(childNodes[i])->expType != ExpType::Number)
            return;
        dims.push_back(static_cast<NumberNode *>(childNodes[i]->childNodes[0])->num);
    }
    if (dims.size() < valPtr->arrayDims.size())
        return;

    isConst = true;
    static_cast<IdentNode *>(childNodes[0])->id = valPtr->newName;
    assert(parentNodePtr->nodeType == NodeType::EXP);
    auto parentExpPtr = static_cast<ExpNode *>(parentNodePtr);
    assert(static_cast<ExpNode *>(parentNodePtr)->expType == ExpType::LVal);

    parentExpPtr->childNodes.clear();
    parentExpPtr->setExpType(ExpType::Number);
    if (lValChidlNum == 1)
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
    if (ptr->nodeType == NodeType::BLOCK || ptr->nodeType == NodeType::IDENT)
        return;
    auto *argPtr = (ArgumentNode *) ptr;
    argPtr->newName = "p" + to_string(paramCnt);
    paramCnt++;
    symbolTablePtr->insert_or_assign(((IdentNode *) argPtr->childNodes[0])->id, argPtr);
}

/*
 * override equivalentlyTransform()
 */
void BlockNode::equivalentlyTransform() {
//    for (auto it = childNodes.begin(); it != childNodes.end();) {
//
//    }
}

void VarDefNode::equivalentlyTransform() {
    flattenArray();
}

void ArgumentNode::equivalentlyTransform() {
    if (!isArray)
        return;
    arrayDims.push_back(0);
    for (int i = 0; i < childNodes[1]->childNodes.size(); i++) {
        assert(childNodes[1]->childNodes[i]->nodeType == NodeType::EXP);
        assert(((ExpNode *) (childNodes[1]->childNodes[i]))->expType == ExpType::Number);
        auto numPtr = (NumberNode *) (childNodes[1]->childNodes[i]->childNodes[0]);
        arrayDims.push_back(numPtr->num);
    }
    childNodes[1]->childNodes.clear();
}

void ExpNode::equivalentlyTransform() {
    switch (expType) {
        case ExpType::BinaryExp:
//            childNodes[0]->evalNow();
//            childNodes[2]->evalNow();
            if (((ExpNode *) childNodes[0])->isConst() && ((ExpNode *) childNodes[2])->isConst())
                evalBinary();
            break;
        case ExpType::UnaryExp:
//            childNodes[1]->evalNow();
            if (((ExpNode *) childNodes[1])->isConst())
                evalUnary();
            break;
        default:
            break;
    }
}

void LValNode::equivalentlyTransform() {
    if (isConst)
        return;
    assert(childNodes[0]->nodeType == NodeType::IDENT);
    auto idPtr = static_cast<IdentNode *>(childNodes[0]);
    assert(symbolTablePtr->count(idPtr->id) > 0);
    auto valPtr = (*symbolTablePtr)[idPtr->id];
    idPtr->id = valPtr->newName;

    if (valPtr->arrayDims.size() + 1 == childNodes.size()) {
        if (childNodes.size() > 2) {
            assert(childNodes.size() - 1 == valPtr->arrayDims.size());
            assert(childNodes[1]->nodeType == NodeType::EXP);
            NodePtr expNode = childNodes[1];
            for (int i = 2; i < childNodes.size(); i++) {
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
        if (valDimNum == 0)
            return;
        int valDimSize = 1;
        for (int i = valDimNum; i < arrayDimNum; i++)
            valDimSize *= valPtr->arrayDims[i];
        NodePtr expNode = childNodes[1];
        for (int i = 2; i < childNodes.size(); i++) {
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






