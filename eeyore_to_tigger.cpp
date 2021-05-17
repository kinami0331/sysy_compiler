//
// Created by KiNAmi on 2021/5/17.
//
#include "eeyore_ast.hpp"
#include "tigger.hpp"

TiggerRootNode *EeyoreRootNode::generateTigger() {
    auto tiggerRoot = new TiggerRootNode();
    int s = childList.size();
    unsigned int globalCnt = 0;
    for(int i = 0; i < s; i++) {
        // 全局变量改名
        if(childList[i]->nodeType == EeyoreNodeType::VAR_DECL) {
            auto varDeclPtr = static_cast<EeyoreVarDeclNode *>(childList[i]);
            auto newName = "v" + std::to_string(globalCnt);
            tiggerRoot->eeyoreSymbolToTigger[varDeclPtr->name] = newName;
            globalCnt++;
            tiggerRoot->tiggerGlobalSymbol[newName] = TiggerVarInfo();
            tiggerRoot->tiggerGlobalSymbol[newName].isGlobal = true;
            if(EeyoreBaseNode::globalSymbolTable[varDeclPtr->name].isArray) {
                tiggerRoot->tiggerGlobalSymbol[newName].isArray = true;
                tiggerRoot->tiggerGlobalSymbol[newName].arraySize = EeyoreBaseNode::globalSymbolTable[varDeclPtr->name].arraySize;
            }
            tiggerRoot->childList.push_back(new TiggerGlobalDeclNode(newName));
        } else {
            auto funDefPtr = static_cast<EeyoreFuncDefNode *>(childList[i]);
            auto tiggerFunc = new TiggerFuncDefNode(tiggerRoot->tiggerGlobalSymbol, tiggerRoot->eeyoreSymbolToTigger);
            tiggerFunc->translateEeyore(funDefPtr);
        }
    }
}

void TiggerFuncDefNode::translateEeyore(EeyoreFuncDefNode *eeyoreFunc) {
//    int s = eeyoreFunc->childList.size(), flag = 0， stack_cnt =
    // 第一轮，注册变量

}
