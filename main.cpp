#include "ast.hpp"
#include "yacc.tab.hpp"
#include "eeyore_ast.hpp"
#include "tigger.hpp"
#include <fstream>
#include <unistd.h>

extern NodePtr astRoot;

int main(int argc, char **argv) {
    int o;

    string inputFileName = "test_in.sy";
    string outFileName = "test_out.sy";
    bool isS = false;
    bool isSysy = false;
    bool isEeyore = false;
    bool isTigger = false;
    bool isDebug = false;
    const char *shortOpts = "t:e:o:i:y:dS";

    while((o = getopt(argc, argv, shortOpts)) != -1) {
        switch(o) {
            case 'S':
                isS = true;
                for(int i = 0; i < argc; i++) {
                    if(i < argc - 2 && string(argv[i]) == "-S" && string(argv[i + 1])[0] != '-')
                        inputFileName = string(argv[i + 1]);
                }
                break;
            case 't':
                isTigger = true;
                inputFileName = string(optarg);
                break;
            case 'e':
                isEeyore = true;
                inputFileName = string(optarg);
                break;
            case 'o':
                outFileName = string(optarg);
                break;
            case 'i':
                inputFileName = string(optarg);
                break;
            case 'd':
                isDebug = true;
                break;
            case 'y':
                isSysy = true;
                inputFileName = string(optarg);
                break;
            case '?':
                printf("error optopt: %c\n", optopt);
                printf("error opterr: %d\n", opterr);
                exit(-1);
            default:
                break;
        }
    }
    // parse
    FILE *fp = fopen(inputFileName.c_str(), "r");
    if(fp == nullptr) {
        cout << "cannot open " << inputFileName << "\n";
        return -1;
    }
    extern FILE *yyin;
    yyin = fp;
    if(yyparse()) {
        exit(1);
    }


    ofstream fout;

    if(isDebug) {
        fout.open("test_step1_out.sy");
        astRoot->generateSysy(fout, 0);
        fout.close();
        auto eeyoreRoot = astRoot->generateEeyoreTree();
        fout.open("test_step2_out.sy");
        eeyoreRoot->generateSysy(fout, 0);
        fout.close();
        fout.open("test_step3_out.eeyore");
        eeyoreRoot->generateEeyore(fout, 0);
        fout.close();

        eeyoreRoot->generateCFG();

        fout.open("test_step4_out.eeyore");
        eeyoreRoot->generateEeyore(fout, 0);
        fout.close();

        fout.open("test_step5_out.dot");
        eeyoreRoot->generateGraphviz(fout);
        fout.close();

        auto tiggerRoot = eeyoreRoot->generateTigger();

        fout.open("test_step6_out.tigger");
        tiggerRoot->generateTigger(fout, 0);
        fout.close();

        fout.open("test_step7_out.riscv");
        tiggerRoot->generateRiscv(fout, 0);
        fout.close();

    } else if(isEeyore) {
        fout.open(outFileName);
        auto eeyoreRoot = astRoot->generateEeyoreTree();
        eeyoreRoot->generateEeyore(fout, 0);
        fout.close();
    } else if(isSysy) {
        fout.open("test_out.sy");
        auto *eeyoreRoot = astRoot->generateEeyoreTree();
        eeyoreRoot->generateSysy(fout, 0);
        fout.close();
    } else if(isTigger) {
        fout.open(outFileName);
        auto eeyoreRoot = astRoot->generateEeyoreTree();
        eeyoreRoot->generateCFG();
        auto tiggerRoot = eeyoreRoot->generateTigger();
        tiggerRoot->generateTigger(fout, 0);
        fout.close();
    } else if(isS) {
        fout.open(outFileName);
        auto eeyoreRoot = astRoot->generateEeyoreTree();
        eeyoreRoot->generateCFG();
        auto tiggerRoot = eeyoreRoot->generateTigger();
        tiggerRoot->generateRiscv(fout, 0);
        fout.close();
    }


    fclose(fp);

    return 0;
}
