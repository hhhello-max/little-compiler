#include "little-compiler.h"
//将输入转成Token
static char *current_input;

//输出错误并且退出
void error(char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void verror_at(char *loc, char *fmt, va_list ap){
    int pos = loc-current_input;
    fprintf(stderr,"%s\n",current_input);
    fprintf(stderr,"%*s", pos, "");
    fprintf(stderr,"^ ");
    vfprintf(stderr,fmt,ap);
    fprintf(stderr,"\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

//如果匹配s则使用当前token
bool equal(Token *tok, char *op){
    return memcmp(tok->loc, op, tok->len)==0 && op[tok->len]=='\0';
}

Token *skip(Token *tok, char *s){
    if(!equal(tok,s)){
        error_tok(tok,"expected '%s'",s);
    }
    return tok->next;
}

bool consume(Token **rest, Token *tok, char *str){
    if(equal(tok, str)){
        *rest = tok->next;
        return true;
    }
    *rest = tok;
    return false;
}

static Token *new_token(TokenKind kind, char *start, char *end){
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end-start;
    return tok;
}

static bool startsWith(char *p, char *q){
    return strncmp(p, q, strlen(q)) == 0;
}

static bool is_ident1(char c){
    return ('a'<=c && c<='z') || ('A'<=c && c<='Z') || c=='_';
}

static bool is_ident2(char c){
    return is_ident1(c) || ('0'<=c && c<='9');
}

static int read_punct(char *p){
    if(startsWith(p, "==") || startsWith(p, "!=") || startsWith(p, ">=") || startsWith(p, "<=")){
        return 2;
    }
    return ispunct(*p)? 1:0;
}

static bool is_keyword(Token *tok){
    static char *kw[] = {"return","if","else","for","while","int"};

    for (int i = 0; i < sizeof(kw)/ sizeof(*kw); i++) {
        if(equal(tok,kw[i])){
            return true;
        }
    }

    return false;
}

static void convert_keywords(Token *tok){
    for (Token *t=tok; t->kind!=TK_EOF; t=t->next){
        if(is_keyword(t)){
            t->kind = TK_KEYWORD;
        }
    }
}

Token *tokenize(char *p){
    current_input = p;
    Token head = {};
    Token *cur = &head;

    while (*p){
        if(isspace(*p)){
            p++;
            continue;
        }
        if(isdigit(*p)){
            cur = cur->next = new_token(TK_NUM, p, p);
            char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p-q;
            continue;
        }
        if(is_ident1(*p)){//变量标识符或关键字
            char *start = p;
            do{
                p++;
            } while (is_ident2(*p));
            cur = cur->next = new_token(TK_IDENT, start, p);
            continue;
        }
        //判断操作符
        int punct_len = read_punct(p);
        if(punct_len){
            cur = cur->next = new_token(TK_PUNCT, p, p+punct_len);
            p += cur->len;
            continue;
        }

        error_at(p,"invalid token");
    }

    cur = cur->next = new_token(TK_EOF, p, p);
    convert_keywords(head.next);
    return head.next;
}