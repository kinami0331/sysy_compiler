/*
 * 作者：kinami
 * 
 */


/* 第一部分：声明段 */
%{

#include "ast.hpp"
#include "yacc.tab.hpp"

extern "C"
{
    int yywrap(void);
    int yylex(void);
}
%}

/* 定义COMMENT状态
 * TODO: 关于lex中的状态可以进一步了解
 */
%x COMMENT
%x SC


nondigit        [_A-Za-z]
digit           [0-9]
identifier      {nondigit}({nondigit}|{digit})*


dec_int         0|[1-9]{digit}*
oct_int         0[0-7]+
hex_int         0[xX][a-fA-F0-9]+

/* 空白符及换行 */
blank_chars     [ \f\r\t\v\n]+
/* 直接返回的符号 */
symbols         [,;\[\]\(\)\{\}]
rel_op          ">"|">="|"<"|"<="
eq_op           "=="|"!="
add_op          \+|-
not_op          !
mul_op          \*|\/|%
and_op          &&
or_op           \|\|

/* 第二部分：规则段 */
%%

"//"            {
                    // cout<<"(begin comment)"<<endl;
                    BEGIN COMMENT;
                }
<COMMENT>.      {}
<COMMENT>\n     {   
                    // cout<<"(end comment)"<<endl;
                    BEGIN INITIAL;  
                }
"/*"            {
                    // cout<<"(begin star_comment)"<<endl;
                    BEGIN SC;
                }
<SC>.|\n        {}
<SC>"*/"        {
                    // cout<<"(end star_comment)"<<endl;
                    BEGIN INITIAL;  
                }


 /* 各种常量 */
{blank_chars}   {}

if              {   return IF;  }
else            {   return ELSE;    }
while           {   return WHILE;   }
break           {   return BREAK;   }
continue        {   return CONTINUE;    }
return          {   return RETURN;  }
void            {   return VOID;    }
int             {   return INT;     }
const           {   return CONST;   }

{symbols}       {   return yytext[0];   }
=               {   return ASSIGN;  }
{rel_op}        {   
                    strcpy(yylval.opStr, yytext);
                    return REL_OP;
                }
{eq_op}         {
                    strcpy(yylval.opStr, yytext);
                    return EQ_OP;
                }
{add_op}        {
                    strcpy(yylval.opStr, yytext);
                    return ADD_OP;
                }
{not_op}        {
                    return NOT_OP;
                }
{mul_op}        {
                    strcpy(yylval.opStr, yytext);
                    return MUL_OP;
                }                
{and_op}        {   return AND_OP;  }
{or_op}         {   return OR_OP;   }

{identifier}    {
                    yylval.strPtr = new string(yytext);
                    return IDENT;
                }
{dec_int}       {
                    yylval.intVal=strtol(yytext, NULL, 10);
                    return INT_CONST;
                }
{oct_int}       {
                    yylval.intVal=strtol(yytext, NULL, 8);
                    return INT_CONST;
                }
{hex_int}       {
                    yylval.intVal=strtol(yytext, NULL, 16);
                    return INT_CONST;
                }                




%%

 //第3段：C函数定义段
int yywrap(void)
{
    puts("-----the file is end");
    return 1;                                //返回1表示读取全部结束。如果要接着读其它文件，可以这里fopen该文件，文件指针赋给yyin，并返回0
}
