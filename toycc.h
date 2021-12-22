#ifndef CCOMP_TOYCC_H
#define CCOMP_TOYCC_H
#include "dynarray.h"
#include "hashmap.h"

enum TokenType {
    TOK_ADD,
    TOK_ASSIGN,
    TOK_DIV,
    TOK_IDENT,
    TOK_INT,
    TOK_LEFT_CURLY_BRACKET,
    TOK_LEFT_PAREN,
    TOK_LESS_THAN,
    TOK_MUL,
    TOK_RIGHT_CURLY_BRACKET,
    TOK_RIGHT_PAREN,
    TOK_SEMICOLON,
    TOK_SUB,
};

struct Token {
    enum TokenType kind;
    union {
        int64_t i64;
        char* ident;
    };
};

enum NodeKind {
    NODE_ADD,
    NODE_ASSIGN,
    NODE_BLOCK,
    NODE_DIV,
    NODE_EXPR_STMT,
    NODE_IF,
    NODE_INT,
    NODE_LESS_THAN,
    NODE_MUL,
    NODE_PROGRAM,
    NODE_RETURN,
    NODE_SUB,
    NODE_VAR,
    NODE_WHILE,
};

struct Variable {
    const char* ident;
    size_t stack_loc;
};

struct Scope {
    const struct Scope* parent;
    struct hashmap variables;
    size_t next_offset;
};

struct ASTNode {
    enum NodeKind kind;
    struct dynarray children;

    union {
        int64_t i64;
        struct Variable var;
    };
};

void codegen(struct ASTNode program, FILE* fp);
struct ASTNode parse(struct dynarray tokens);
#endif //CCOMP_TOYCC_H
