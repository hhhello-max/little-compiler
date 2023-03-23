#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#define _POSIX_C_SOURCE 200809L

typedef struct Type Type;
typedef struct Node Node;

typedef enum {
    TK_IDENT,   //标识符
    TK_PUNCT,   //标点符号
    TK_KEYWORD,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token{
    TokenKind kind; //符号类型
    Token *next;
    int val;
    char *loc;
    int len;
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *s);
bool consume(Token **rest, Token *tok, char  *str);
Token *tokenize(char *p);

//局部变量
typedef struct Obj Obj;
struct Obj{
    Obj *next;
    char *name; //变量名
    Type *ty;
    int offset; //
};

//方法
typedef struct Function Function;
struct Function{
    Node *body;
    Obj *locals;
    int stack_size;
};

//后续将Token转为Node
typedef enum {
    ND_ADD,         // +
    ND_SUB,         // -
    ND_MUL,         // *
    ND_DIV,         // /
    ND_NEG,         // 负号-
    ND_EQ,          // ==
    ND_NE,          // !=
    ND_LT,          // <
    ND_LE,          // >=
    ND_ASSIGN,      // 赋值=
    ND_VAR,         // 变量
    ND_ADDR,        // 取地址 &
    ND_DEREF,       // 解引用 *
    ND_EXPR_STMT,   // 表达语句
    ND_IF,          // if判断
    ND_FOR,         // for或while循环
    ND_BLOCK,       // 代码块 {...}
    ND_RETURN,      // 返回
    ND_NUM          // integer
} NodeKind;

struct Node{
    NodeKind kind;
    Node *next;
    Type *ty;
    Token *tok; // Representative token
    Node *lhs;
    Node *rhs;

    // "if" or "for" statement
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    Obj *var;   // 变量名 if kind == ND_VAR
    Node *body; // Block
    int val;    // Used if kind == ND_NUM
};

Function *parse(Token *tok);

typedef enum {
    TY_INT,
    TY_PTR,
}TypeKind;

struct Type {
    TypeKind kind;
    Type *base;     //Pointer
    Token *name;    //Declaration
};

extern Type *ty_int;

bool is_integer(Type *ty);
Type *pointer_to(Type *base);
void add_type(Node *node);

void codegen(Function *prog);
