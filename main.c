#include "little-compiler.h"

//调试专用
int main(void ) {
    Token *tok = tokenize("{int x=3,y=5; return x+y;}");

    Function *prog = parse(tok);
    codegen(prog);

    return 0;
}
