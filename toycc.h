#ifndef CCOMP_TOYCC_H
#define CCOMP_TOYCC_H
#include "dynarray.h"
#include "hashmap.h"

enum TokenType {
    TOK_ADD,
    TOK_ASSIGN,
    TOK_ASSIGN_ADD,
    TOK_COMMA,
    TOK_DIV,
    TOK_EQUALS,
    TOK_IDENT,
    TOK_INCREMENT,
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
    NODE_ASSIGN_ADD,
    NODE_BLOCK,
    NODE_DECL,
    NODE_DIV,
    NODE_EQUALS,
    NODE_EXPR_STMT,
    NODE_FUNCTION_DEF,
    NODE_IDENT,
    NODE_IF,
    NODE_INT,
    NODE_LESS_THAN,
    NODE_MUL,
    NODE_POSTFIX_INCREMENT,
    NODE_PROGRAM,
    NODE_RETURN,
    NODE_SUB,
    NODE_WHILE,
};

enum DeclKind {
    DECL_VARIABLE,
    DECL_FUNCTION
};

struct FunctionDeclaration {
    // TODO: return type
    // TODO: parameter types
    unsigned int frame_size;
};

struct VariableDeclaration {
    size_t stack_loc;
};

struct Declaration {
    const char* ident;
    enum DeclKind kind;
    union {
        struct VariableDeclaration var_decl;
        struct FunctionDeclaration fun_decl;
    };
};

struct Scope {
    const struct Scope* parent;
    struct hashmap decls;
};

struct ASTNode {
    enum NodeKind kind;
    struct dynarray children;

    union {
        int64_t i64;
        struct Declaration decl;
    };
};

void codegen(struct ASTNode program, FILE* fp);
struct ASTNode parse(struct dynarray tokens);
#endif //CCOMP_TOYCC_H
