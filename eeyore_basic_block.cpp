//
// Created by KiNAmi on 2021/5/16.
//
#include "eeyore_ast.hpp"

void EeyoreRootNode::generateCFG() {
    for(auto ptr:childList)
        if(ptr->nodeType == EeyoreNodeType::FUNC_DEF)
            static_cast<EeyoreFuncDefNode *>(ptr)->generateCFG();
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
            case EeyoreNodeType::FUNC_CALL: {
                // 如果当前block的最后一个元素不是label，则新开一个block
                if(curBlock->stmtList.back()->nodeType == EeyoreNodeType::LABEL) {
                    curBlock->insertStmt(*it);
                } else {
                    // 过程调用作为新的基本块开始
                    basicBlockList.push_back(curBlock);
                    curBlockId++;
                    curBlock = new BasicBlock(curBlockId);
                    curBlock->insertStmt(*it);
                }
                break;
            }
            case EeyoreNodeType::COMMENT: {
                // TODO 这里可以考虑不插入注释
                // curBlock->insertStmt(*it);
                break;
            }
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
    basicBlockList.push_back(curBlock);
    curBlockId++;
    curBlock = new BasicBlock(curBlockId);
    curBlock->setLabelName("EXIT");
    basicBlockList.push_back(curBlock);
    assert(curBlockId + 1 == basicBlockList.size());

    // 第二轮，建立基本块之间的边
    int blockNum = curBlockId + 1;
    for(int i = 0; i < blockNum - 1; i++) {
        // 看最后一句
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
        }
        else if (basicBlockList[i]->stmtList.back()->nodeType == EeyoreNodeType::RETURN) {
            basicBlockList[i]->nextNodeList.push_back(blockNum - 1);
            basicBlockList[blockNum - 1]->preNodeList.push_back(i);
        }
        else {
            basicBlockList[i]->nextNodeList.push_back(i + 1);
            basicBlockList[i + 1]->preNodeList.push_back(i);
        }
    }
}
