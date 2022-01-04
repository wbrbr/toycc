#ifndef CCOMP_TOYCC_H
#define CCOMP_TOYCC_H
#include <stdio.h>
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
    } data;
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
    NODE_FOR,
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

enum TypeKind {
    TYPE_FLOAT,
    TYPE_INCOMPLETE,
    TYPE_INT,
    TYPE_VOID,
};

struct Type {
    enum TypeKind kind;
    int size; // we use an int because the size of some types is unknown at compile-time (incomplete types, VLA)
    unsigned int alignment;
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
    struct Type type;
    union {
        struct VariableDeclaration var;
        struct FunctionDeclaration fun;
    } data;
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
    } data;
};

void tokenize(struct dynarray* tokens, const char* input);
void codegen(struct ASTNode program, FILE* fp);
struct ASTNode parse(struct dynarray tokens);
#endif //CCOMP_TOYCC_H
