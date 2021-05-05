LEX=flex
YACC=bison
CC=g++
OBJECTS = main.o lex.yy.o yacc.tab.o ast_adjust_array.o ast_base.o ast_generate_sy.o ast_constructor.o ast_exp_eval.o

all: $(OBJECTS)
	$(CC) -o compiler $(OBJECTS) -g
	@rm -f *.o

do: $(OBJECTS)
	$(CC) -o compiler $(OBJECTS) -g
	@rm -f *.o
	@./compiler -t test_in.sy > test_out.out

lex:
	$(LEX) -o lex.yy.cpp lex.l

yacc:
#	bison使用-d参数编译.y文件
# $(YACC) -d --report=all -o yacc.tab.c yacc.y
	$(YACC) -d -o yacc.tab.c yacc.y

clean:
	@rm -f $(OBJECT)  *.o yacc.tab.* lex.yy.*

main.o: yacc.tab.hpp
	$(CC) -c main.cpp -g

ast_adjust_array.o: ast.hpp
	$(CC) -c ast_adjust_array.cpp -g

ast_base.o: ast.hpp
	$(CC) -c ast_base.cpp -g

ast_generate_sy.o: ast.hpp
	$(CC) -c ast_generate_sy.cpp -g

ast_constructor.o: ast.hpp
	$(CC) -c ast_constructor.cpp -g

ast_exp_eval.o:ast.hpp
	$(CC) -c ast_exp_eval.cpp -g

lex.yy.o: lex.yy.cpp  yacc.tab.hpp
	$(CC) -c lex.yy.cpp -g 

yacc.tab.o: yacc.tab.cpp
	$(CC) -c yacc.tab.cpp -g

yacc.tab.cpp  yacc.tab.hpp: yacc.y
#	bison使用-d参数编译.y文件
	$(YACC) -d -o yacc.tab.cpp yacc.y

lex.yy.cpp: lex.l
	$(LEX) -o lex.yy.cpp lex.l