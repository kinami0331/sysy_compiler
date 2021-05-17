LEX=flex
YACC=bison
CC=g++
OBJECTS = main.o lex.yy.o yacc.tab.o ast_adjust_array.o ast_base.o ast_generate_sy.o ast_constructor.o \
			ast_exp_eval.o ast_standardizing.o eeyore_ast_generate_sy.o ast_generate_eeyore_node.o \
			eeyore_generate.o eeyore_basic_block.o eeyore_graphviz.o eeyore_to_tigger.o

all: $(OBJECTS)
	$(CC) -o compiler $(OBJECTS) -g -std=c++17
	@rm -f *.o

do: $(OBJECTS)
	$(CC) -o compiler $(OBJECTS) -g -std=c++17
	@rm -f *.o
	@./compiler -d test_in.sy > test_out.out
	@dot test_step5_out.dot -T png -o test_step5_out.png

test:  test_clion.cpp
	$(CC) -o test_clion test_clion.cpp -g -std=c++17

lex:
	$(LEX) -o lex.yy.cpp lex.l

yacc:
#	bison使用-d参数编译.y文件
	$(YACC) -d --report=all -o yacc.tab.c yacc.y
# $(YACC) -d -o yacc.tab.c yacc.y

count:
	@cloc $$(git ls-files) --by-file-by-lang --hide-rate --exclude-ext="md" --md --report-file="code_statistics.md"

clean:
	@rm -f $(OBJECT)  *.o yacc.tab.* lex.yy.*

main.o: yacc.tab.hpp
	$(CC) -c main.cpp -g -std=c++17

ast_adjust_array.o: ast.hpp
	$(CC) -c ast_adjust_array.cpp -g -std=c++17

ast_base.o: ast.hpp
	$(CC) -c ast_base.cpp -g -std=c++17

ast_generate_sy.o: ast.hpp
	$(CC) -c ast_generate_sy.cpp -g -std=c++17

ast_constructor.o: ast.hpp
	$(CC) -c ast_constructor.cpp -g -std=c++17

ast_exp_eval.o:ast.hpp
	$(CC) -c ast_exp_eval.cpp -g -std=c++17

ast_standardizing.o: ast.hpp
	$(CC) -c ast_standardizing.cpp -g -std=c++17

eeyore_ast_generate_sy.o: eeyore_ast_generate_sy.cpp
	$(CC) -c eeyore_ast_generate_sy.cpp -g -std=c++17

ast_generate_eeyore_node.o: ast_generate_eeyore_node.cpp
	$(CC) -c ast_generate_eeyore_node.cpp -g -std=c++17

eeyore_generate.o: eeyore_generate.cpp
	$(CC) -c eeyore_generate.cpp -g -std=c++17

eeyore_basic_block.o: eeyore_basic_block.cpp
	$(CC) -c eeyore_basic_block.cpp -g -std=c++17

eeyore_graphviz.o: eeyore_graphviz.cpp
	$(CC) -c eeyore_graphviz.cpp -g -std=c++17

eeyore_to_tigger.o: eeyore_to_tigger.cpp
	$(CC) -c eeyore_to_tigger.cpp -g -std=c++17

lex.yy.o: lex.yy.cpp  yacc.tab.hpp
	$(CC) -c lex.yy.cpp -g -std=c++17

yacc.tab.o: yacc.tab.cpp
	$(CC) -c yacc.tab.cpp -g -std=c++17

yacc.tab.cpp  yacc.tab.hpp: yacc.y
#	bison使用-d参数编译.y文件
	$(YACC) -d -o yacc.tab.cpp yacc.y

lex.yy.cpp: lex.l
	$(LEX) -o lex.yy.cpp lex.l