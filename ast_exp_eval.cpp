#include "ast.hpp"

void ExpNode::evalNow() {
//        cout << " I'm OK\n";
    switch (expType) {
        case ExpType::LVal:
            break;
        case ExpType::FuncCall:
            break;
        case ExpType::Number:
            break;
        case ExpType::BinaryExp:
            childNodes[0]->evalNow();
            childNodes[2]->evalNow();
            if (((ExpNode *) childNodes[0])->isConst() && ((ExpNode *) childNodes[2])->isConst())
                evalBinary();
            break;
        case ExpType::UnaryExp:
            childNodes[1]->evalNow();
            if (((ExpNode *) childNodes[1])->isConst())
                evalUnary();
            break;
    }
}

void ExpNode::evalUnary() {

}

void ExpNode::evalBinary() {
    int ans, t1 = ((NumberNode *) childNodes[0]->childNodes[0])->num,
            t2 = ((NumberNode *) childNodes[2]->childNodes[0])->num;
//        cout << "log: " << t1 << " " << t2 << endl;
//        childNodes[0]->print();
    switch (((Op2Node *) childNodes[1])->opType) {
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
            break;
    }
    childNodes.clear();
    pushNodePtr(new NumberNode(ans));
    setExpType(ExpType::Number);
}