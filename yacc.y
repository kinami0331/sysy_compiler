 //第1段：声明段

%{
#include "ast.hpp"	//lex和yacc要共用的头文件，里面包含了一些头文件，重定义了YYSTYPE

extern "C" {
    void yyerror(const char *s);
    extern int yylex(void);//该函数是在lex.yy.c里定义的，yyparse()里要调用该函数，为了能编译和链接，必须用extern加以声明
}

NodePtr astRoot;

%}

%union {
    int intVal;
    string *strPtr;
    char opStr[4];
    // Node* node;
    NodePtr node;
}

// 字面量
%token          IF
%token          ELSE
%token          WHILE
%token          BREAK
%token          CONTINUE
%token          RETURN
%token          VOID
%token          INT
%token          CONST

// 运算符
%token          ASSIGN      // =
%token<opStr>   REL_OP      // > >= < <=
%token<opStr>   EQ_OP       // == !=
%token<opStr>   ADD_OP      // + -
%token          NOT_OP      // !
%token<opStr>   MUL_OP      // * / %
%token          AND_OP      // &&
%token          OR_OP       // ||



%token<intVal>  INT_CONST   // 整数字面量
%token<strPtr>  IDENT       // 标识符

%nonassoc IFX 
%nonassoc ELSE

%left ADD_OP
%left MUL_OP

%type<node> Start
%type<node> CompUnit
%type<node> ConstDecl
%type<node> ConstDefs
%type<node> ConstDef
%type<node> CEInBrackets
%type<node> ConstInitVal
%type<node> CIVs
%type<node> VarDecl
%type<node> VarDefs
%type<node> VarDef
%type<node> InitVal
%type<node> InitVals
%type<node> FuncDef
%type<node> Arguments
%type<node> Argument
%type<node> Block
%type<node> BlockItems
%type<node> BlockItem
%type<node> Stmt
%type<node> Exp
%type<node> AddExp
%type<node> MulExp
%type<node> UnaryExp
%type<opStr> UnaryOp
%type<node> PrimaryExp
%type<node> LVal
%type<node> EInBrackets
%type<node> Cond
%type<node> Number
%type<node> FuncParams
%type<node> RelExp
%type<node> EqExp
%type<node> LAndExp
%type<node> LOrExp
%type<node> ConstExp


 /* 第二段 */
%%

/* */

Start       : CompUnit {
                ($$) = ($1);
                astRoot = ($$);
                astRoot->standardizing();
            };

CompUnit    : {
                ($$) = new RootNode();
            }
            | ConstDecl CompUnit {
                ($$) = new RootNode();
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($2)->childNodes);
            }
            | VarDecl CompUnit {
                ($$) = new RootNode();
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($2)->childNodes);
            }
            | FuncDef CompUnit {
                ($$) = new RootNode();
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($2)->childNodes);
            }

ConstDecl   : CONST INT ConstDefs ';' {
                ($$) = ($3);
            }

ConstDefs   : ConstDef {
                // ($$) = new ConstDeclNode();
                ($$) = new VarDeclNode(true);
                ($$)->pushNodePtr($1);
            }
            | ConstDef ',' ConstDefs {
                // ($$) = new ConstDeclNode();
                ($$) = new VarDeclNode(true);
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($3)->childNodes);
            }

ConstDef    : IDENT CEInBrackets ASSIGN ConstInitVal {
                // ($$) = new ConstDefNode();
                ($$) = new VarDefNode(true);
                ($$)->pushNodePtr(new IdentNode(*($1)));
                ($$)->pushNodePtr($2);
                ($$)->pushNodePtr($4);
                // ((VarDefNode*)($$))->flattenArray();
            }

 // [const]*
CEInBrackets: {
                ($$) = new CEInBracketsNode();
            }
            | '[' ConstExp ']' CEInBrackets {
                ($$) = new CEInBracketsNode();
                ($$)->pushNodePtr($2);
                ($$)->pushNodePtrList(($4)->childNodes);
            }

ConstInitVal: ConstExp {
                ($$) = new InitValNode(false);
                ($$)->pushNodePtr($1);
            }
            | '{' '}' {
                ($$) = new InitValNode(true);
                NodePtr t1 = new ExpNode(ExpType::Number);
                t1->pushNodePtr(new NumberNode(0));
                NodePtr t2 = new InitValNode(false);
                t2->pushNodePtr(t1);
                ($$)->pushNodePtr(t2);
            }
            | '{' CIVs '}' {
                ($$) = new InitValNode(true);
                ($$)->pushNodePtrList(($2)->childNodes);
            };

CIVs        : ConstInitVal {
                ($$) = new InitValNode(true);
                ($$)->pushNodePtr($1);
            }
            | ConstInitVal ',' CIVs {
                ($$) = new InitValNode(true);
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($3)->childNodes);
            }

VarDecl     : INT VarDefs ';' {
                ($$) = ($2);
            } 

VarDefs     : VarDef {
                ($$) = new VarDeclNode(false);
                ($$)->pushNodePtr($1);
            }
            | VarDef ',' VarDefs {
                ($$) = new VarDeclNode(false);
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($3)->childNodes);
            };

VarDef      : IDENT CEInBrackets {
                ($$) = new VarDefNode(false);
                ($$)->pushNodePtr(new IdentNode(*($1)));
                ($$)->pushNodePtr($2);
            }
            | IDENT CEInBrackets ASSIGN InitVal {
                ($$) = new VarDefNode(false);
                ($$)->pushNodePtr(new IdentNode(*($1)));
                ($$)->pushNodePtr($2);
                ($$)->pushNodePtr($4);
                // ((VarDefNode*)($$))->flattenArray();
            };

InitVal     : Exp {
                ($$) = new InitValNode(false);
                ($$)->pushNodePtr($1);
            }
            | '{' '}' {
                ($$) = new InitValNode(true);
                NodePtr t1 = new ExpNode(ExpType::Number);
                t1->pushNodePtr(new NumberNode(0));
                NodePtr t2 = new InitValNode(false);
                t2->pushNodePtr(t1);
                ($$)->pushNodePtr(t2);
            }
            | '{' InitVals '}' {
                ($$) = new InitValNode(true);
                ($$)->pushNodePtrList(($2)->childNodes);
            }

InitVals    : InitVal {
                ($$) = new InitValNode(true);
                ($$)->pushNodePtr($1);
            }
            | InitVal ',' InitVals {
                ($$) = new InitValNode(true);
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($3)->childNodes);
            }
            
FuncDef     : INT IDENT '(' ')' Block {
                ($$) = new FuncDefNode(true);
                ($$)->pushNodePtr(new IdentNode(*($2)));
                ($$)->pushNodePtr($5);
            }
            | VOID IDENT '(' ')' Block {
                ($$) = new FuncDefNode(false);
                ($$)->pushNodePtr(new IdentNode(*($2)));
                ($$)->pushNodePtr($5);
            }
            | INT IDENT '(' Arguments ')' Block {
                ($$) = new FuncDefNode(true);
                ($$)->pushNodePtr(new IdentNode(*($2)));
                ($$)->pushNodePtrList(($4)->childNodes);
                ($$)->pushNodePtr($6);
            }
            | VOID IDENT '(' Arguments ')' Block {
                ($$) = new FuncDefNode(false);
                ($$)->pushNodePtr(new IdentNode(*($2)));
                ($$)->pushNodePtrList(($4)->childNodes);
                ($$)->pushNodePtr($6);
            }

Arguments   : Argument {
                ($$) = new FuncDefNode();
                ($$)->pushNodePtr($1);  
            }
            | Argument ',' Arguments {
                ($$) = new FuncDefNode();
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($3)->childNodes);
            }

Argument    : INT IDENT {
                ($$) = new ArgumentNode(false);
                ($$)->pushNodePtr(new IdentNode(*($2)));
            }
            | INT IDENT '[' ']' CEInBrackets {
                ($$) = new ArgumentNode(true);
                ($$)->pushNodePtr(new IdentNode(*($2)));
                ($$)->pushNodePtr($5);
            };

Block       : '{' BlockItems '}' { 
                if($2 != NULL) {
                    ($$) = ($2); 
                }
                else {
                    ($$) = new BlockNode();
                }
                
            };

BlockItems  : { ($$) = NULL; }
            | BlockItem BlockItems {
                ($$) = new BlockNode();
                ($$)->pushNodePtr($1);
                if($2 != NULL) {
                    ($$)->pushNodePtrList(($2)->childNodes);
                }
                
            };

BlockItem   : ConstDecl { ($$) = ($1); }
            | VarDecl { ($$) = ($1); }
            | Stmt { ($$) = ($1); };

Stmt        : LVal ASSIGN Exp ';' {
                ($$) = new AssignNode();
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtr($3);
            }
            | Exp ';' {
                ($$) = new ExpStmtNode();
                ($$)->pushNodePtr($1);
            }
            | ';' {
                ($$) = new NullStmtNode();
            }
            | Block {
                ($$) = ($1);
            }
            | IF '(' Cond ')' Stmt %prec IFX {
                ($$) = new IfNode();
                ($$)->pushNodePtr($3);
                if($5->nodeType != NodeType::BLOCK)
                    ($$)->pushNodePtr((new BlockNode())->pushNodePtr($5));
                else
                    ($$)->pushNodePtr($5);
                // ($$)->pushNodePtr($5);
            }
            | IF '(' Cond ')' Stmt ELSE Stmt {
                ($$) = new IfNode();
                ($$)->pushNodePtr($3);
                if($5->nodeType != NodeType::BLOCK)
                    ($$)->pushNodePtr((new BlockNode())->pushNodePtr($5));
                else
                    ($$)->pushNodePtr($5);
                
                if($7->nodeType != NodeType::BLOCK)
                    ($$)->pushNodePtr((new BlockNode())->pushNodePtr($7));
                else
                    ($$)->pushNodePtr($7);
  
                
            }
            | WHILE '(' Cond ')' Stmt {
                ($$) = new WhileNode();
                ($$)->pushNodePtr($3);
                
                if($5->nodeType != NodeType::BLOCK)
                    ($$)->pushNodePtr((new BlockNode())->pushNodePtr($5));
                else
                    ($$)->pushNodePtr($5);
            }
            | BREAK ';' {
                ($$) = new BreakNode();
            }
            | CONTINUE ';' {
                ($$) = new ContinueNode();
            }
            | RETURN ';' {
                ($$) = new ReturnNode();
            }
            | RETURN Exp ';' {
                ($$) = new ReturnNode();
                ($$)->pushNodePtr($2);
            };

Exp         : AddExp {
                ($$) = ($1); 
            }
            | Cond {
                ($$) = ($1); 
            };

AddExp      : MulExp {
                ($$) = ($1); 
            }
            | AddExp ADD_OP MulExp {
                ($$) = new ExpNode(ExpType::BinaryExp);
                ($$)->pushNodePtr($1);
                if(!strcmp($2, "+"))
                    ($$)->pushNodePtr(new Op2Node(OpType::opPlus));
                else if(!strcmp($2, "-"))
                    ($$)->pushNodePtr(new Op2Node(OpType::opDec));
                ($$)->pushNodePtr($3);
            };

MulExp      : UnaryExp {
                ($$) = ($1); 
            }
            | MulExp MUL_OP UnaryExp {
                ($$) = new ExpNode(ExpType::BinaryExp);
                ($$)->pushNodePtr($1);
                if(!strcmp($2, "*"))
                    ($$)->pushNodePtr(new Op2Node(OpType::opMul));
                else if(!strcmp($2, "/"))
                    ($$)->pushNodePtr(new Op2Node(OpType::opDiv));
                else if(!strcmp($2, "%"))
                    ($$)->pushNodePtr(new Op2Node(OpType::opMod));
                ($$)->pushNodePtr($3);
            };

UnaryExp    : PrimaryExp {
                ($$) = ($1); 
            }
            | IDENT '(' FuncParams ')' {
                ($$) = new ExpNode(ExpType::FuncCall);
                NodePtr t = new FuncCallNode();
                t->pushNodePtr(new IdentNode(*($1)));
                t->pushNodePtrList(($3)->childNodes);
                ($$)->pushNodePtr(t);
            }
            | IDENT '(' ')' {
                ($$) = new ExpNode(ExpType::FuncCall);
                NodePtr t = new FuncCallNode();
                if(*($1) == "starttime") {
                    t->pushNodePtr(new IdentNode("_sysy_starttime"));
                    t->pushNodePtr(new ExpNode(ExpType::Number, 0));
                }
                else if(*($1) == "stoptime") {
                    t->pushNodePtr(new IdentNode("_sysy_stoptime"));
                    t->pushNodePtr(new ExpNode(ExpType::Number, 0));
                }
                else {
                    t->pushNodePtr(new IdentNode(*($1)));
                }
                ($$)->pushNodePtr(t);  
            }
            | UnaryOp UnaryExp {
                ($$) = new ExpNode(ExpType::UnaryExp);
                if(($1[0]) == '!')
                    ($$)->pushNodePtr(new Op1Node(OpType::opNot));
                else if(($1[0]) == '+')
                    ($$)->pushNodePtr(new Op1Node(OpType::opPlus));
                else if(($1[0]) == '-')
                    ($$)->pushNodePtr(new Op1Node(OpType::opDec));
                ($$)->pushNodePtr($2);
            };

UnaryOp     : ADD_OP {
                ($$)[0] = ($1)[0];
                ($$)[1] = ($1)[1];
            }
            | NOT_OP {
                ($$)[0] = '!';
                ($$)[1] = '\0';
            };

PrimaryExp  : '(' Exp ')' {
                ($$) = ($2);
            }
            | LVal {
                ($$) = new ExpNode(ExpType::LVal);
                ($$)->pushNodePtr($1);
            }
            | Number {
                ($$) = new ExpNode(ExpType::Number);
                ($$)->pushNodePtr($1);
            };

LVal        : IDENT EInBrackets {
                ($$) = new LValNode();
                ($$)->pushNodePtr(new IdentNode(*($1)));
                ($$)->pushNodePtrList(($2)->childNodes);
            }

EInBrackets : {
                ($$) = new LValNode();
            }
            | '[' Exp ']' EInBrackets {
                ($$) = new LValNode();
                ($$)->pushNodePtr($2);
                ($$)->pushNodePtrList(($4)->childNodes);
            }

Cond        : LOrExp {
                ($$) = ($1);
                // ($$) = new CondNode();
                // ($$)->pushNodePtr($1);
            };

Number      : INT_CONST {
                ($$) = new NumberNode($1);
            };

FuncParams  : Exp {
                ($$) = new FuncCallNode();
                ($$)->pushNodePtr($1);
            }
            | Exp ',' FuncParams {
                ($$) = new FuncCallNode();
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtrList(($3)->childNodes);
            }

RelExp      : AddExp {  ($$) = ($1);  }
            | RelExp REL_OP AddExp {
                ($$) = new ExpNode(ExpType::BinaryExp);
                ($$)->pushNodePtr($1);
                if(!strcmp($2, ">="))
                    ($$)->pushNodePtr(new Op2Node(OpType::opGE));
                else if(!strcmp($2, "<="))
                    ($$)->pushNodePtr(new Op2Node(OpType::opLE));
                else if(!strcmp($2, ">"))
                    ($$)->pushNodePtr(new Op2Node(OpType::opG));
                else if(!strcmp($2, "<"))
                    ($$)->pushNodePtr(new Op2Node(OpType::opL));
                ($$)->pushNodePtr($3);
            };

EqExp       : RelExp {  ($$) = ($1);  }
            | EqExp EQ_OP RelExp {
                ($$) = new ExpNode(ExpType::BinaryExp);
                ($$)->pushNodePtr($1);
                if(($2)[0] == '!')
                    ($$)->pushNodePtr(new Op2Node(OpType::opNE));
                else
                    ($$)->pushNodePtr(new Op2Node(OpType::opE));
                ($$)->pushNodePtr($3);
            };

LAndExp     : EqExp {  ($$) = ($1);  }
            | LAndExp AND_OP EqExp {
                ($$) = new ExpNode(ExpType::BinaryExp);
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtr(new Op2Node(OpType::opAnd));
                ($$)->pushNodePtr($3);
            };

LOrExp      : LAndExp {  ($$) = ($1);  }
            | LOrExp OR_OP LAndExp{
                ($$) = new ExpNode(ExpType::BinaryExp);
                ($$)->pushNodePtr($1);
                ($$)->pushNodePtr(new Op2Node(OpType::opOr));
                ($$)->pushNodePtr($3);
            };

ConstExp    : AddExp {  
                ($$) = ($1);
                // ($$)->evalNow();
                // ($$) = new ConstExpNode();
                // ($$)->pushNodePtr($1);
            };


 /* 第三段 */
%%
void yyerror(const char *s)	{
	cerr << s << endl;
}