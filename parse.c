#include "little-compiler.h"
//将Token解析成节点

Obj *locals;

static Node *declaration(Token **rest, Token *tok);
static Node *compound_stmt(Token **rest, Token *tok);
static Node *stmt(Token **rest, Token *tok);
static Node *expr_stmt(Token **rest, Token *tok);
static Node *expr(Token **rest, Token *tok);
static Node *assign(Token **rest, Token *tok);
static Node *equality(Token **rest, Token *tok);
static Node *relational(Token **rest, Token *tok);
static Node *add(Token **rest, Token *tok);
static Node *mul(Token **rest, Token *tok);
static Node *unary(Token **rest, Token *tok);
static Node *primary(Token **rest, Token *tok);

static Obj *find_var(Token *tok){
    for (Obj *var=locals; var; var=var->next){
        if(strlen(var->name)==tok->len && !strncmp(tok->loc, var->name, tok->len)){
            return var;
        }
    }
    return NULL;
}

static Node *new_node(NodeKind kind, Token *tok){
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->tok = tok;
    return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs, Token *tok){
    Node *node = new_node(kind, tok);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

static Node *new_unary(NodeKind kind, Node *expr, Token *tok){
    Node *node = new_node(kind, tok);
    node->lhs = expr;
    return node;
}

static Node *new_num(int val, Token *tok){
    Node *node = new_node(ND_NUM, tok);
    node->val = val;
    return node;
}

static Node *new_var_node(Obj *var, Token *tok){
    Node *node = new_node(ND_VAR, tok);
    node->var = var;
    return node;
}

static Obj *new_lvar(char *name, Type *ty){
    Obj *var = calloc(1,sizeof(Obj));
    var->name = name;
    var->ty = ty;
    var->next = locals;
    locals = var;
    return var;
}

static char *get_ident(Token *tok){
    if(tok->kind != TK_IDENT)
        error_tok(tok, "expected an identifier");
    return strndup(tok->loc, tok->len);
}

static Type *declspec(Token **rest, Token *tok){
    *rest = skip(tok, "int");
    return ty_int;
}

static Type *declarator(Token **rest, Token *tok, Type *ty){
    while (consume(&tok, tok, "*")){
        ty = pointer_to(ty);
    }
    if(tok->kind != TK_IDENT){
        error_tok(tok, "expected a variable name");
    }

    ty->name = tok;
    *rest = tok->next;
    return ty;
}

static Node *declaration(Token **rest, Token *tok){
    Type *basety = declspec(&tok, tok);

    Node head = {};
    Node *cur = &head;
    int i = 0;

    while (!equal(tok, ";")){
        if(i++ > 0){
            tok = skip(tok, ",");
        }

        Type *ty = declarator(&tok, tok, basety);
        Obj *var = new_lvar(get_ident(ty->name), ty);

        if(!equal(tok, "=")){
            continue;
        }

        Node *lhs = new_var_node(var, ty->name);
        Node *rhs = assign(&tok, tok->next);
        Node *node = new_binary(ND_ASSIGN, lhs, rhs, tok);
        cur = cur->next = new_unary(ND_EXPR_STMT, node, tok);
    }

    Node *node = new_node(ND_BLOCK, tok);
    node->body = head.next;
    *rest = tok->next;
    return node;
}

/* stmt = "return" expr ";"
 *      | "if" "(" expr ")" stmt ("else" stmt)?
 *      | "for" "(" expr-stmt expr? ";" expr? ")" stmt
 *      | "while" "(" expr ")" stmt
 *      | "{" compound-stmt
 *      | expr_stmt
 */
static Node *stmt(Token **rest, Token *tok){
    if(equal(tok, "return")){
        //Node *node = new_unary(ND_RETURN, expr(&tok,tok->next));
        Node *node = new_node(ND_RETURN, tok);
        node->lhs = expr(&tok, tok->next);
        *rest = skip(tok,";");
        return node;
    }
    if(equal(tok, "if")){
        Node *node = new_node(ND_IF, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok,tok);
        tok = skip(tok,")");
        node->then = stmt(&tok,tok);
        if(equal(tok,"else")){
            node->els = stmt(&tok,tok);
        }
        *rest = tok;
        return node;
    }
    if(equal(tok, "for")){
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next,"(");

        node->init = expr_stmt(&tok, tok);
        if(!equal(tok, ";")){
            node->cond = expr(&tok, tok);
        }
        tok = skip(tok, ";");
        if(!equal(tok, ")")){
            node->inc = expr(&tok, tok);
        }
        tok = skip(tok, ")");
        node->then = stmt(rest, tok);
        return node;
    }
    if(equal(tok, "while")){
        Node *node = new_node(ND_FOR, tok);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(rest, tok);
        return node;
    }
    if(equal(tok, "{")){
        return compound_stmt(rest, tok->next);
    }
    return expr_stmt(rest, tok);
}

// compound-stmt = (declaration | stmt)* "}"
static Node *compound_stmt(Token **rest, Token *tok){
    Node *node = new_node(ND_BLOCK, tok);

    Node head = {};
    Node *cur = &head;
    while (!equal(tok, "}")){
        if(equal(tok, "int")){
            cur = cur->next = declaration(&tok, tok);
        } else{
            cur = cur->next = stmt(&tok, tok);
        }
        add_type(cur);
    }

    node->body = head.next;
    *rest = tok->next;
    return node;
}

// expr-stmt = expr? ";"
static Node *expr_stmt(Token **rest, Token *tok){
    if(equal(tok, ";")){
        *rest = tok->next;
        return new_node(ND_BLOCK, tok);
    }
    Node *node = new_node(ND_EXPR_STMT, tok);
    node->lhs = expr(&tok, tok);
    *rest = skip(tok, ";");
    return node;
}

static Node *expr(Token **rest, Token *tok){
    return assign(rest, tok);
}

static Node *assign(Token **rest, Token *tok){
    Node *node = equality(&tok, tok);
    if(equal(tok, "=")){
        return new_binary(ND_ASSIGN, node, assign(rest, tok->next), tok);
    }
    *rest = tok;
    return node;
}

static Node *equality(Token **rest, Token *tok){
    Node *node = relational(&tok, tok);
    for (;;){
        Token *start = tok;
        if(equal(tok, "==")){
            node = new_binary(ND_EQ, node, relational(&tok,tok->next), start);
            continue;
        }
        if(equal(tok, "!=")){
            node = new_binary(ND_NE, node, relational(&tok,tok->next), start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

//relational = add ("<" add | "<=" add | ">" add | ">=" add)
static Node *relational(Token **rest, Token *tok){
    Node *node = add(&tok, tok);
    for (;;){
        Token *start = tok;
        if(equal(tok, "<")){
            node = new_binary(ND_LT, node, add(&tok,tok->next),start);
            continue;
        }
        if(equal(tok, "<=")){
            node = new_binary(ND_LE, node, add(&tok,tok->next),start);
            continue;
        }
        if(equal(tok, ">")){
            node = new_binary(ND_LT, add(&tok,tok->next), node,start);
            continue;
        }
        if(equal(tok, ">=")){
            node = new_binary(ND_LE, add(&tok,tok->next), node,start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

static Node *new_add(Node *lhs, Node *rhs, Token *tok){
    add_type(lhs);
    add_type(rhs);

    if(is_integer(lhs->ty) && is_integer(rhs->ty)){
        return new_binary(ND_ADD, lhs, rhs, tok);
    }
    if(lhs->ty->base && rhs->ty->base){
        error_tok(tok, "invalid operands");
    }
    if(!lhs->ty->base && rhs->ty->base){
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    rhs = new_binary(ND_MUL, rhs, new_num(8, tok), tok);
    return new_binary(ND_ADD, lhs, rhs, tok);
}

static Node *new_sub(Node *lhs, Node *rhs, Token *tok){
    add_type(lhs);
    add_type(rhs);

    // num-num
    if(is_integer(lhs->ty) && is_integer(rhs->ty)){
        return new_binary(ND_SUB, lhs, rhs, tok);
    }
    // prt-num
    if(lhs->ty->base && is_integer(rhs->ty)){
        rhs = new_binary(ND_MUL, rhs, new_num(8, tok), tok);
        add_type(rhs);
        Node *node = new_binary(ND_SUB, lhs, rhs, tok);
        node->ty = lhs->ty;
        return node;
    }
    //prt-ptr, which returns how many elements are between the two.
    if(lhs->ty->base && rhs->ty->base){
        Node *node = new_binary(ND_SUB, lhs, rhs, tok);
        node->ty = ty_int;
        return new_binary(ND_DIV, node, new_num(8, tok), tok);
    }

    error_tok(tok, "invalid operands");
}

// add = mul("+" mul | "-" mul)
static Node *add(Token **rest, Token *tok){
    Node *node = mul(&tok, tok);//先进行乘法，然后加减
    for (;;){
        Token *start = tok;
        if(equal(tok, "+")){
            node = new_add(node, mul(&tok, tok->next),start);
            continue;
        }
        if(equal(tok, "-")){
            node = new_sub(node, mul(&tok, tok->next),start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

// mul = mul("*" unary | "/" unary)
static Node *mul(Token **rest, Token *tok){
    Node *node = unary(&tok, tok);//判断负号
    for (;;){//对比下一个tok是不是*或者/，是的话继续进行
        Token *start = tok;
        if(equal(tok, "*")){
            node = new_binary(ND_MUL, node, unary(&tok, tok->next),start);
            continue;
        }
        if(equal(tok, "/")){
            node = new_binary(ND_DIV, node, unary(&tok, tok->next),start);
            continue;
        }
        *rest = tok;
        return node;
    }
}

//单运算符 unary = ("+" | "-" | "*" | "&") unary | primary
static Node *unary(Token **rest, Token *tok){
    if(equal(tok, "+")){
        return unary(rest, tok->next);
    }
    if(equal(tok, "-")){
        return new_unary(ND_NEG, unary(rest, tok->next),tok);
    }
    if(equal(tok, "&")){
        return new_unary(ND_ADDR, unary(rest, tok->next),tok);
    }
    if(equal(tok, "*")){
        return new_unary(ND_DEREF, unary(rest, tok->next),tok);
    }
    return primary(rest, tok);
}

//primary = "(" expr ")" | ident | num
static Node *primary(Token **rest, Token *tok){
    if(equal(tok,"(")){
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    if(tok->kind == TK_IDENT){
        Obj *var = find_var(tok);
        if(!var){
            //var = new_lvar(strndup(tok->loc, tok->len));
            error_tok(tok, "undefined variable");
        }
        *rest = tok->next;
        return new_var_node(var, tok);
    }

    if(tok->kind == TK_NUM){
        Node *node = new_num(tok->val, tok);
        *rest = tok->next;
        return node;
    }
    error_tok(tok,"expected an expression");
}

//program
Function *parse(Token *tok){
    tok = skip(tok, "{");

    Function *prog = calloc(1, sizeof(Function));
    prog->body = compound_stmt(&tok, tok);
    prog->locals = locals;
    return prog;
}
