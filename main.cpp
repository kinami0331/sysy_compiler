#include "ast.hpp"
#include "yacc.tab.hpp"
#include "eeyore_ast.hpp"
#include <fstream>
#include <unistd.h>

extern NodePtr astRoot;

int main(int argc, char **argv) {
    int o;
    string syFileName;
    string outFileName = "test_out.sy";
    const char *shortOpts = "t:";
    while((o = getopt(argc, argv, shortOpts)) != -1) {
        switch(o) {
            case 't':
                syFileName = string(optarg);
                // cout << syFileName;
                // printf("opt is a, oprarg is: %s\n", optarg);
                break;
            case '?':
                printf("error optopt: %c\n", optopt);
                printf("error opterr: %d\n", opterr);
                exit(-1);
                break;
            default:
                break;
        }
    }
    // const char *sFile = "test.sy";
    FILE *fp = fopen(syFileName.c_str(), "r");
    if(fp == nullptr) {
        cout << "cannot open " << syFileName << "\n";
        return -1;
    }
    extern FILE *yyin;
    yyin = fp;

    printf("-----begin parsing %s\n", syFileName.c_str());
    if(yyparse()) {
        exit(1);
    }

    ofstream fout;
    fout.open(outFileName);
    astRoot->generateSysy(fout, 0);
    fout.close();

    auto *eeyoreRoot = new EeyoreRootNode();
    eeyoreRoot->childList = move(astRoot->generateEeyoreNode());

    fout.open("test_eeyore_sy.sy");
    eeyoreRoot->generateSysy(fout, 0);
    fout.close();

    for(auto ptr:eeyoreRoot->childList)
        ptr->print();


    fclose(fp);

    return 0;
}
