//
// Created by KiNAmi on 2021/5/6.
//
#include "ast.hpp"
#include "eeyore_ast.hpp"

void BaseNode::print() {
    print(0, false);
    cout << '\n';
}

void BaseNode::print(int blank, bool newLine) {
    if(newLine) {
        cout << '\n';
        for(int i = 0; i < blank; i++)
            cout << ' ';
    }
    cout << '/' << setw(15) << name;
    for(int i = 0; i < childNodes.size(); i++) {
        childNodes[i]->print(blank + 1 + 15, i != 0);
    }
}

void BaseNode::outBlank(ostream &out, int n) {
    for(int i = 0; i < n; i++)
        out << ' ';
}

BaseNode *BaseNode::pushNodePtrList(std::vector<BaseNode *> &list) {
    childNodes.insert(childNodes.end(), list.begin(), list.end());
    return this;
}

BaseNode *BaseNode::pushNodePtr(BaseNode *ptr) {
    childNodes.push_back(ptr);
    return this;
}

void BaseNode::standardizing() {
    getParentSymbolTable();
    for(auto ptr: childNodes) {
        ptr->setParentPtr(this);
        updateSymbolTable(ptr);
        ptr->standardizing();
    }
    replaceSymbols();
    equivalentlyTransform();
}

void BaseNode::getParentSymbolTable() {
    assert(parentNodePtr != nullptr);
    symbolTablePtr = parentNodePtr->symbolTablePtr;
}

void BaseNode::setParentPtr(BaseNode *ptr) {
    parentNodePtr = ptr;
}

vector<EeyoreBaseNode *> BaseNode::generateEeyoreNode() {
    vector<EeyoreBaseNode *> eeyoreList;
    for(auto ptr:childNodes) {
        auto t = ptr->generateEeyoreNode();
        eeyoreList.insert(eeyoreList.end(), t.begin(), t.end());
    }
    return eeyoreList;
}

EeyoreRootNode *BaseNode::generateEeyoreTree() {
    auto t = generateEeyoreNode();
    assert(t.size() == 1);
    return static_cast<EeyoreRootNode *>(t[0]);
}