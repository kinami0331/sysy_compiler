//
// Created by KiNAmi on 2021/5/16.
//
#include "eeyore_ast.hpp"

void EeyoreRootNode::generateGraphviz(ostream &out) {
    out << "digraph root {\n";
    out << "    label = \"root\"\n";
    out << "    graph [\n"
        << "        rankdir = LR\n"
        << "        splines = polyline\n"
        << "    ];\n";
    for(auto ptr:childList)
        if(ptr->nodeType == EeyoreNodeType::FUNC_DEF)
            static_cast<EeyoreFuncDefNode *>(ptr)->generateGraphviz(out);
    out << "}\n";
}

void EeyoreFuncDefNode::generateGraphviz(ostream &out) {
    out << "    subgraph cluster_" << funcName << "{\n";
    out << "        label = \"" << funcName << "\"\n";

    // 打印node
    // 边的颜色
    int edgeCnt = 0;
    // 使用一些颜色
    vector<string> colors{"darkred", "goldenrod1", "lightseagreen", "deepskyblue1", "green", "purple1", "tan2",
                          "royalblue4", "orangered"};
    //
    const int COLOR_NUM = colors.size();
    int s = basicBlockList.size();
    for(int i = 0; i < s; i++) {
        outBlank(out, 8);
        out << '\"' << funcName << "_block" << std::to_string(basicBlockList[i]->blockId) << "\" [\n";

        outBlank(out, 12);
        out << "label = \"<head> block id: " << std::to_string(basicBlockList[i]->blockId) << " \\n "
            << "block label: " << basicBlockList[i]->blockLabel << " \\n "
            << "pre blocks: ";
        for(auto t: basicBlockList[i]->preNodeList)
            out << t << ' ';
        out << "|";
        if(i > 0) {// 这里输入语句
            for(auto ptr:basicBlockList[i]->stmtList) {
                switch(ptr->nodeType) {
                    case EeyoreNodeType::BLOCK_BEGIN:
                    case EeyoreNodeType::BLOCK_END:
                    case EeyoreNodeType::COMMENT:
                    case EeyoreNodeType::FILL_INIT:
                        continue;
                    case EeyoreNodeType::GLOBAL_INIT:
                        out << "global init .....|";
                        break;
                    default:
                        auto rst = ptr->to_eeyore_string();
                        string::size_type pos = 0;
                        if((pos = rst.find(">")) != string::npos)
                            rst.replace(pos, 1, "\\>");
                        if((pos = rst.find("<")) != string::npos)
                            rst.replace(pos, 1, "\\<");
                        if((pos = rst.find("||")) != string::npos)
                            rst.replace(pos, 2, "\\|\\| ");
                        while((pos = rst.find("\n")) != string::npos)
                            rst.replace(pos, 1, "\\n");
                        out << rst << "|";
                }

            }
        }
        out << "<foot>\"\n";
        outBlank(out, 12);
        out << "shape = \"record\"\n";

        outBlank(out, 8);
        out << "];\n";

        // 这里生成边
        for(auto t: basicBlockList[i]->nextNodeList) {
            outBlank(out, 8);
            out << '\"' << funcName << "_block" << std::to_string(basicBlockList[i]->blockId) << "\":foot -> "
                << '\"' << funcName << "_block" << std::to_string(t) << "\":head" << "["
                << "color = " << colors[edgeCnt] << "];\n";
            edgeCnt = (edgeCnt + 1) % COLOR_NUM;
        }

    }
    out << "    }\n";
}

