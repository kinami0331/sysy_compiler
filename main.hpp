//
// Created by KiNAmi on 2021/5/11.
//

#ifndef SYSY_COMPILER_MAIN_HPP
#define SYSY_COMPILER_MAIN_HPP

#include <iostream>
#include <optional>
#include <memory>
#include <utility>
#include <vector>
#include <string>
#include <utility>
#include <iomanip>
#include <cstdarg>
#include <string>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <map>
#include <set>
#include <algorithm>
#include "main.hpp"

using namespace std;

enum class OpType {
    opPlus, opDec, opMul, opDiv, opMod, opNot, opAnd, opOr, opG, opL, opGE, opLE, opE, opNE,
    opBitAnd, opBitOr, NONE
};
enum class NodeType {
    IDENT, ROOT, CONST_DECL, CONST_DEF, CE_IN_BRACKET, CONST_INIT_VAL, VAL_DECL, VAR_DEF,
    INIT_VAL, FUNC_DEF, ARGUMENT, BLOCK, ASSIGN, IF, WHILE, BREAK, CONTINUE, RETURN, OP_1,
    OP_2, EXP, UNARY_EXP, NUMBER, L_VAL, COND, BASE, AST, LEAF, OR_COND, AND_COND, FUNC_CALL,
    STMT, CONST_EXP, NULL_STMT, EXP_STMT, IF_GOTO, WHILE_GOTO
};


enum class ExpType {
    Number, LVal, FuncCall, BinaryExp, UnaryExp
};

enum class EeyoreNodeType {
    ROOT, VAR_DECL, FUNC_DEF, WHILE, IF, GOTO, LABEL, FUNC_CALL, RETURN, EXP, LEFT_VALUE, RIGHT_VALUE, IF_GOTO, ASSIGN,
    BLOCK_BEGIN, BLOCK_END, COMMENT, FILL_INIT, GLOBAL_INIT, PARAM
};

enum class TiggerNodeType {
    ROOT, GLOBAL_DECL, FUNC_DEF, COMMENT, ASSIGN, IF_GOTO, GOTO, LABEL, FUNC_CALL, RETURN, STORE, LOAD, LOAD_ADDR
};

enum class TiggerAssignType {
    RR_BINARY, RN_BINARY, R_UNARY, R, N, LEFT_ARRAY, RIGHT_ARRAY
};

class Util {
public:
    static string getNodeTypeName(NodeType type) {
        switch(type) {
            case NodeType::EXP_STMT:
                return "exp_stmt";
            case NodeType::IDENT:
                return "ident";
            case NodeType::ROOT:
                return "root";
            case NodeType::CONST_DECL:
                return "const_decl";
            case NodeType::CONST_DEF:
                return "const_def";
            case NodeType::CE_IN_BRACKET:
                return "ce_in_bracket";
            case NodeType::CONST_INIT_VAL:
                return "const_init_val";
            case NodeType::VAL_DECL:
                return "val_decl";
            case NodeType::VAR_DEF:
                return "var_def";
            case NodeType::INIT_VAL:
                return "init_val";
            case NodeType::FUNC_DEF:
                return "func_def";
            case NodeType::ARGUMENT:
                return "argument";
            case NodeType::BLOCK:
                return "block";
            case NodeType::ASSIGN:
                return "assign";
            case NodeType::IF:
                return "if";
            case NodeType::WHILE:
                return "while";
            case NodeType::BREAK:
                return "break";
            case NodeType::CONTINUE:
                return "continue";
            case NodeType::RETURN:
                return "return";
            case NodeType::OP_1:
                return "op_1";
            case NodeType::OP_2:
                return "op_2";
            case NodeType::EXP:
                return "exp";
            case NodeType::UNARY_EXP:
                return "unary_exp";
            case NodeType::NUMBER:
                return "number";
            case NodeType::L_VAL:
                return "l_val";
            case NodeType::COND:
                return "cond";
            case NodeType::BASE:
                return "base";
            case NodeType::AST:
                return "ast";
            case NodeType::LEAF:
                return "leaf";
            case NodeType::OR_COND:
                return "or_cond";
            case NodeType::AND_COND:
                return "and_cond";
            case NodeType::FUNC_CALL:
                return "func_call";
            case NodeType::STMT:
                return "stmt";
            case NodeType::CONST_EXP:
                return "const_exp";
            case NodeType::NULL_STMT:
                return "null_stmt";
            case NodeType::IF_GOTO:
                return "if_goto";
            case NodeType::WHILE_GOTO:
                return "while_goto";
        }
        return "";
    };

    static string getExpTypeName(ExpType type) {
        switch(type) {
            case ExpType::Number:
                return "Num";
            case ExpType::LVal:
                return "LVal";
            case ExpType::UnaryExp:
                return "Unary";
            case ExpType::FuncCall:
                return "Func";
            case ExpType::BinaryExp:
                return "Binary";
        }
        return "";
    }

    static string getOpTypeName(OpType type) {
        switch(type) {
            case OpType::opBitAnd:
                return "*";
            case OpType::opBitOr:
                return "+";
            case OpType::opPlus:
                return "+";
            case OpType::opDec:
                return "-";
            case OpType::opMul:
                return "*";
            case OpType::opDiv:
                return "/";
            case OpType::opMod:
                return "%";
            case OpType::opNot:
                return "!";
            case OpType::opAnd:
                return "&&";
            case OpType::opOr:
                return "||";
            case OpType::opG:
                return ">";
            case OpType::opL:
                return "<";
            case OpType::opGE:
                return ">=";
            case OpType::opLE:
                return "<=";
            case OpType::opE:
                return "==";
            case OpType::opNE:
                return "!=";
            case OpType::NONE:
                return "none";
        }
        return "";
    }

    static string getEeyoreNodeType(EeyoreNodeType type) {
        switch(type) {
            case EeyoreNodeType::GLOBAL_INIT:
                return "global_init";
            case EeyoreNodeType::BLOCK_BEGIN:
                return "block_begin";
            case EeyoreNodeType::BLOCK_END:
                return "block_end";
            case EeyoreNodeType::COMMENT:
                return "comment";
            case EeyoreNodeType::ROOT:
                return "root";
            case EeyoreNodeType::VAR_DECL:
                return "var_decl";
            case EeyoreNodeType::FUNC_DEF:
                return "func_def";
            case EeyoreNodeType::WHILE:
                return "while";
            case EeyoreNodeType::IF:
                return "if";
            case EeyoreNodeType::GOTO:
                return "goto";
            case EeyoreNodeType::LABEL:
                return "label";
            case EeyoreNodeType::FUNC_CALL:
                return "func_call";
            case EeyoreNodeType::RETURN:
                return "return";
            case EeyoreNodeType::EXP:
                return "exp";
            case EeyoreNodeType::LEFT_VALUE:
                return "left_value";
            case EeyoreNodeType::RIGHT_VALUE:
                return "right_value";
            case EeyoreNodeType::IF_GOTO:
                return "if_goto";
            case EeyoreNodeType::ASSIGN:
                return "assign";
            default:
                return "";
        }
    }
};


#endif //SYSY_COMPILER_MAIN_HPP
