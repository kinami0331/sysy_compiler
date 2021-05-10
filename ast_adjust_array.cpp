//
// Created by KiNAmi on 2021/5/1.
//
#include "ast.hpp"

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

void VarDefNode::flattenArray() {
    int childNum = (int) childNodes.size();
    assert(childNum == 3 || childNum == 2);

    // return if this is a single int
    if (childNodes[1]->childNodes.empty()) {
        isArray = false;
        if (isConst) {
            assert(((InitValNode *) childNodes[2])->hasOnlyOneExp());
            auto ptr = ((InitValNode *) childNodes[2])->getTheSingleVal();
            assert(ptr->nodeType == NodeType::INIT_VAL && !((InitValNode *) ptr)->isList);
            constVal = ((NumberNode *) (ptr->childNodes[0]->childNodes[0]))->num;
        }
    } else
        isArray = true;

    if (childNum == 2 && parentNodePtr->parentNodePtr->nodeType == NodeType::ROOT) {
        NodePtr t = new InitValNode(true);
        NodePtr tt = new InitValNode(false);
        tt->pushNodePtr(new ExpNode(ExpType::Number, 0));
        t->pushNodePtr(tt);
        if (isArray) {
            t->setParentPtr(this);
            childNodes.push_back(t);
        } else {
            t->setParentPtr(this);
            childNodes.push_back(tt);
        }
        childNum++;
    }
    auto dims = ((CEInBracketsNode *) childNodes[1])->flattenArray();
    // if the init list exists, standardizing it
    if (isArray && childNum == 3) {
        assert((int) dims.size() > 0);
        ((InitValNode *) childNodes[2])->standardizingInitList(dims);
        ((InitValNode *) childNodes[2])->flattenArray(dims);
    }
    reverse(dims.begin(), dims.end());
    arrayDims = move(dims);

}

vector<int> CEInBracketsNode::flattenArray() {
    vector<int> dims;
    int dim = (int) childNodes.size();
    if (dim == 0)
        return dims;
    // if there is an array A[3][4][5][6]
    // return vector<int>{6, 5, 4, 3}
    int product = 1;
    for (int i = dim - 1; i >= 0; i--) {
        assert(childNodes[i]->childNodes[0]->nodeType == NodeType::NUMBER);
        dims.push_back(((NumberNode *) (childNodes[i]->childNodes[0]))->num);
        product *= ((NumberNode *) (childNodes[i]->childNodes[0]))->num;
    }
    childNodes.clear();
    childNodes.push_back(new ExpNode(ExpType::Number, product));
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

vector<int> InitValNode::getIntList() {
    assert(isList);
    assert(!childNodes.empty());
    vector<int> intList;
    bool hasNonzero = false;
    for (auto ptrIt = childNodes.rbegin(); ptrIt != childNodes.rend(); ptrIt++) {
        auto ptr = *ptrIt;
        assert(!((InitValNode *) ptr)->isList);
        assert(ptr->childNodes[0]->childNodes[0]->nodeType == NodeType::NUMBER);
        auto numPtr = (NumberNode *) ptr->childNodes[0]->childNodes[0];
        if (!hasNonzero && numPtr->num == 0)
            continue;
        else {
            intList.push_back(numPtr->num);
            hasNonzero = true;
        }
    }

    return intList;
}

vector<int> InitValNode::getIntList(vector<int> &dims) {
//    cerr << dims[dims.size() - 1] << endl << flush;
    int dimNum = (int) dims.size();
    if (dimNum == 1)
        return getIntList();
    vector<int> nextDims = dims;
    int curDimLen = 1;
    for (int i = 0; i < dimNum - 1; i++)
        curDimLen *= dims[i];
    vector<int> intList;
    nextDims.pop_back();
    bool hasNonzero = false;
    for (auto it = childNodes.rbegin(); it != childNodes.rend(); it++) {
        auto tList = ((InitValNode *) (*it))->getIntList(nextDims);
        if (!hasNonzero)
            for (int &it2 : tList) {
                if (!hasNonzero && it2 == 0)
                    continue;
                else {
                    intList.push_back(it2);
                    hasNonzero = true;
                }
            }
        else {
            int s = (int) tList.size();
            for (int i = 0; i < curDimLen - s; i++)
                intList.push_back(0);
            for (int i = 0; i < s; i++)
                intList.push_back(tList[i]);
        }
    }
    return intList;
}

vector<NodePtr> InitValNode::getExpList() {
    assert(isList);
    assert(!childNodes.empty());
    vector<NodePtr> expList;
    bool hasNonzero = false;
    for (auto ptrIt = childNodes.rbegin(); ptrIt != childNodes.rend(); ptrIt++) {
        auto ptr = *ptrIt;
        assert(!((InitValNode *) ptr)->isList);
        assert(ptr->childNodes[0]->nodeType == NodeType::EXP);
        if (!hasNonzero && ptr->childNodes[0]->childNodes[0]->nodeType == NodeType::NUMBER
            && ((NumberNode *) ptr->childNodes[0]->childNodes[0])->num == 0)
            continue;
        else {
            expList.push_back(ptr->childNodes[0]);
            hasNonzero = true;
        }
    }

    return expList;
}

vector<NodePtr> InitValNode::getExpList(vector<int> &dims) {
//    cerr << dims[dims.size() - 1] << endl << flush;
    int dimNum = (int) dims.size();
    if (dimNum == 1)
        return getExpList();
    vector<int> nextDims = dims;
    int curDimLen = 1;
    for (int i = 0; i < dimNum - 1; i++)
        curDimLen *= dims[i];
    vector<NodePtr> expList;
    nextDims.pop_back();
    bool hasNonzero = false;
    for (auto it = childNodes.rbegin(); it != childNodes.rend(); it++) {
        auto tList = ((InitValNode *) (*it))->getExpList(nextDims);
        if (!hasNonzero)
            for (auto &it2 : tList) {
                assert(it2->nodeType == NodeType::EXP);
                if (!hasNonzero && it2->childNodes[0]->nodeType == NodeType::NUMBER
                    && ((NumberNode *) it2->childNodes[0])->num == 0)
                    continue;
                else {
                    expList.push_back(it2);
                    hasNonzero = true;
                }
            }
        else {
            int s = (int) tList.size();
            for (int i = 0; i < curDimLen - s; i++)
                expList.push_back(new ExpNode(ExpType::Number, 0));
            for (int i = 0; i < s; i++)
                expList.push_back(tList[i]);
        }
    }
    return expList;
}

void InitValNode::flattenArray(vector<int> &dims) {
    assert(parentNodePtr != nullptr);
    assert(parentNodePtr->nodeType == NodeType::VAR_DEF);
    assert((int) dims.size() >= 1);
    if (((VarDefNode *) parentNodePtr)->isConst) {
        auto intList = getIntList(dims);
        reverse(intList.begin(), intList.end());
        childNodes.clear();
        for (auto i:intList) {
            NodePtr t = new InitValNode(false);
            t->pushNodePtr(new ExpNode(ExpType::Number, i));
            childNodes.push_back(t);
        }
        ((VarDefNode *) parentNodePtr)->arrayValues = move(intList);
    } else {
        auto expList = getExpList(dims);
        reverse(expList.begin(), expList.end());
        childNodes = expList;
    }
}

void InitValNode::standardizingInitList(vector<int> &dims) {

    int dimNum = (int) dims.size();
    int childNum = (int) childNodes.size(); // 列表中包含的子节点数
    // 如果只有一维，则每个元素都应该是single的
    if (dimNum == 1) {
        for (int i = 0; i < childNum; i++)
            childNodes[i] = ((InitValNode *) childNodes[i])->getTheSingleVal();
        return;
    }
    int curDim = dims.back();   // 当前层的维数（需要的元素数）
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

    // fill the empty lists with a single '0'
    for (int i = 0; i < curDim - j; i++) {
        NodePtr t = new InitValNode(true);
        NodePtr tt = new InitValNode(false);
        tt->pushNodePtr(new ExpNode(ExpType::Number, 0));
        t->pushNodePtr(tt);
        newChildNodes.push_back(t);
    }

    childNodes = newChildNodes;

    for (auto &childNode : childNodes) {
        ((InitValNode *) childNode)->standardizingInitList(nextDims);
    }
}


