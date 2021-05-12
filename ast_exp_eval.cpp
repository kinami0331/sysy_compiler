#include "ast.hpp"

void ExpNode::evalUnary() {
    int ans, t = ((NumberNode *) childNodes[1]->childNodes[0])->num;
    switch(((Op1Node *) childNodes[0])->opType) {
        case OpType::opPlus:
            ans = t;
            break;
        case OpType::opDec:
            ans = -t;
            break;
        case OpType::opNot:
            ans = !t;
            break;
        default:
            assert(false);
    }
    childNodes.clear();
    pushNodePtr(new NumberNode(ans));
    setExpType(ExpType::Number);
}

void ExpNode::evalBinary() {
    int ans, t1 = ((NumberNode *) childNodes[0]->childNodes[0])->num,
            t2 = ((NumberNode *) childNodes[2]->childNodes[0])->num;
    switch(((Op2Node *) childNodes[1])->opType) {
        case OpType::opPlus:
            ans = t1 + t2;
            break;
        case OpType::opDec:
            ans = t1 - t2;
            break;
        case OpType::opMul:
            ans = t1 * t2;
            break;
        case OpType::opDiv:
            ans = t1 / t2;
            break;
        case OpType::opMod:
            ans = t1 % t2;
            break;
        case OpType::opAnd:
            ans = t1 && t2;
            break;
        case OpType::opOr:
            ans = t1 || t2;
            break;
        case OpType::opG:
            ans = t1 > t2;
            break;
        case OpType::opL:
            ans = t1 < t2;
            break;
        case OpType::opGE:
            ans = t1 >= t2;
            break;
        case OpType::opLE:
            ans = t1 <= t2;
            break;
        case OpType::opE:
            ans = t1 == t2;
            break;
        case OpType::opNE:
            ans = t1 != t2;
            break;
        default:
            cout << "sth wrong!\n";
            return;
    }
    childNodes.clear();
    pushNodePtr(new NumberNode(ans));
    setExpType(ExpType::Number);
}