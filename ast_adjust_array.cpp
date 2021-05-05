//
// Created by KiNAmi on 2021/5/1.
//
#include "ast.hpp"
#include <string>
#include <cassert>

bool InitValNode::hasOnlyOneExp() {
    if (!isList)
        return true;
        // 如果是个列表，则为一个EXP元素的列表
    else if (childNodes.size() == 1) {
        return !((InitValNode *) childNodes[0])->isList;
    }
    return false;
}

NodePtr InitValNode::getTheSingleVal() {
    assert(hasOnlyOneExp());
    if (!isList)
        return this;
    return childNodes[0];
}

void ValDefNode::adjustArray() {
    // 1维或0维直接返回
    int dim = (int) childNodes[1]->childNodes.size();
    if (dim <= 1)
        return;

    vector<int> dims = ((CEInBracketsNode *) childNodes[1])->getDimVector();
//    vector<int> dims = ((CEInBracketsNode *) childNodes[1])->adjustArray();
    // 首先将初始化列表进行标准化
//    print();
    ((InitValNode *) childNodes[2])->standardizing(dims);
}

vector<int> CEInBracketsNode::adjustArray() {
//    print();
    int dim = (int) childNodes.size();
    vector<int> dims;
    if (dim == 0)
        return dims;
    NodePtr expNode = childNodes[dim - 1];
    dims.push_back(((NumberNode *) (childNodes[dim - 1]->childNodes[0]))->num);
    for (int i = dim - 2; i >= 0; i--) {
        NodePtr t = new ExpNode(ExpType::BinaryExp);
        t->pushNodePtr(expNode);
        t->pushNodePtr(new Op2Node(OpType::opMul));
        t->pushNodePtr(childNodes[i]);
        dims.push_back(((NumberNode *) (childNodes[i]->childNodes[0]))->num);
        expNode = t;
    }
    childNodes.clear();
    expNode->evalNow();
    childNodes.push_back(expNode);
//    print();
    return dims;
}

vector<int> CEInBracketsNode::getDimVector() {
    int dim = (int) childNodes.size();
    vector<int> dims;
    if (dim == 0)
        return dims;
    for (int i = dim - 1; i >= 0; i--)
        dims.push_back(((NumberNode *) (childNodes[i]->childNodes[0]))->num);
    return dims;
}


void InitValNode::standardizing(vector<int> &dims) {

    int dimNum = (int) dims.size();
    int childNum = (int) childNodes.size(); // 列表中包含的子节点数
    // 如果只有一维，则每个元素都应该是single的
    if (dimNum == 1) {
        for (int i = 0; i < childNum; i++)
            childNodes[i] = ((InitValNode *) childNodes[i])->getTheSingleVal();
        return;
    }
    int curDim = dims.back();   // 当前层的维数（需要的元素数）
    int curDimSize = 1;         // 当前层的每一维的大小
    int lastDim = dims[0];      // 最深一层的维数
    vector<int> nextDims = dims;  // 向下一层传递的维数数组
    nextDims.pop_back();
    NodePtrList newChildNodes;
    // 计算一个维度的列表数，是中间维度的乘积
    int curDimListNum = 1;
    for (int i = 1; i < dimNum - 1; i++)
        curDimListNum *= dims[i];

    // 扫描所有的元素
    // j的含义为已经确认了一个元素
    int j = 0;
    for (int i = 0; i < childNum; i++) {
        // 如果这个元素是列表，直接j += 1
        if (((InitValNode *) childNodes[i])->isList) {
            newChildNodes.push_back(childNodes[i]);
        } else {
            // 如果这个元素不是list，则需要按以下规则做
            // 假设四位数组[a][b][c][d]，则应当先“填满”一个[b][c][d]
            // 填满的方式是用b * c个列表进行填满， 用这些列表去初始化
            // k是要寻找的列表数
            // newNode是这一轮找到的总列表
            NodePtr newNode = new InitValNode(true);
            int k = curDimListNum;
            while (k > 0 && i < childNum) {
                // 如果当前这个是数字，则从当前数字开始的lastDim个元素应该被处理为列表
                // 这lastDim个元素要么是数字，要么是花括号里只有一个数字
                // 注意封顶的情况
                if (!((InitValNode *) childNodes[i])->isList) {
                    int ii;
                    for (ii = i; ii < childNum && ii < i + lastDim; ii++) {
                        assert(((InitValNode *) childNodes[ii])->hasOnlyOneExp());
                        newNode->pushNodePtr(((InitValNode *) childNodes[ii])->getTheSingleVal());
                    }
                    // 这个循环结束后，找到了一个长度最大为lastDim的列表
                    i = ii;
                } else {
                    // 否则，childNodes[i]本身已经是一个列表了
                    newNode->pushNodePtr(childNodes[i]);
                    i++;
                }
                k--;
            }
            // 这里需要多减一次1
            i--;
            // 这个循环结束时，说明找到了一个新的list
            newChildNodes.push_back(newNode);
        }
        j++;

        if (j == curDim) {
            assert(i == childNum - 1);
        }
    }

    for (int i = 0; i < curDim - j; i++) {
        NodePtr t = new InitValNode(true);
        NodePtr t1 = new ExpNode(ExpType::Number);
        t1->pushNodePtr(new NumberNode(0));
        t->pushNodePtr(t1);
        newChildNodes.push_back(t);
    }

    childNodes = newChildNodes;

    for (auto &childNode : childNodes) {
        ((InitValNode *) childNode)->standardizing(nextDims);
    }
}


