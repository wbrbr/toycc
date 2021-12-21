#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <vector>
#include <string.h>
extern "C" {
#include "dynarray.h"
#include "hashmap.h"
#include "util.h"
}

enum TokenType {
    TOK_ADD,
    TOK_ASSIGN,
    TOK_DIV,
    TOK_IDENT,
    TOK_INT,
    TOK_LEFT_CURLY_BRACKET,
    TOK_LEFT_PAREN,
    TOK_MUL,
    TOK_RIGHT_CURLY_BRACKET,
    TOK_RIGHT_PAREN,
    TOK_SEMICOLON,
    TOK_SUB,
};

struct Token {
    TokenType kind;
    union {
        int64_t i64;
        char* ident;
    };
};

void print_token(Token tok)
{
    switch(tok.kind) {
        case TOK_INT:
            printf("%ld ", tok.i64);
            break;

        case TOK_ADD:
            printf("+ ");
            break;

        case TOK_SUB:
            printf("- ");
            break;

        case TOK_MUL:
            printf("* ");
            break;

        case TOK_DIV:
            printf("/ ");
            break;

        case TOK_LEFT_PAREN:
            printf("(");
            break;

        case TOK_RIGHT_PAREN:
            printf(")");
            break;

        case TOK_SEMICOLON:
            printf(";");
            break;

        case TOK_IDENT:
            printf("%s", tok.ident);
            break;

        case TOK_ASSIGN:
            printf("=");
            break;

        case TOK_LEFT_CURLY_BRACKET:
            printf("{");
            break;
            
        case TOK_RIGHT_CURLY_BRACKET:
            printf("}");
            break;
    }
}


struct CharIterator {
    const char* data;
    size_t index;
    size_t size;

    CharIterator(const char* data, size_t size) {
        this->data = data;
        index = 0;
        this->size = size;
    }

    char peek() {
        return data[index];
    }

    char next() {
        char c = data[index];
        index++;
        return c;
    }

    bool has_next() {
        return index < size;
    }

    bool consume(char c) {
        if (peek() == c) {
            next();
            return true;
        } else {
            return false;
        }
    }

    bool consume_pred(int (*pred)(int), char* c) {
        if (pred(peek())) {
            char n = next();
            if (c) {
                *c = n;
            }
            return true;
        } else {
            return false;
        }
    }
};

Token match_num(CharIterator& iter) {
    int64_t num = 0;
    while (isdigit(iter.peek())) {
        int v = iter.next() - '0';
        num = 10 * num + v;
    }

    Token tok;
    tok.kind = TOK_INT;
    tok.i64 = num;
    return tok;
}

Token match_ident(CharIterator& iter)
{
    dynarray string;
    dynarray_init(&string, 1);

    char c;
    while (iter.consume_pred(isalnum, &c)) {
        dynarray_push(&string, &c);
    }

    char null = 0;
    dynarray_push(&string, &null);

    Token tok;
    tok.kind = TOK_IDENT;
    tok.ident = (char*)string.data;

    return tok;
}

void tokenize(struct dynarray* tokens, const char* input)
{
    CharIterator iter(input, strlen(input));

    while (iter.has_next()) {
        char c = iter.peek();
        if (isspace(c)) {
            iter.next();
        } else if (isdigit(c)) {
            Token tok = match_num(iter);
            dynarray_push(tokens, &tok);
        } else if (isalpha(c)) {
            Token tok = match_ident(iter);
            dynarray_push(tokens, &tok);
        } else if (iter.consume('+')) {
            Token tok;
            tok.kind = TOK_ADD;
            dynarray_push(tokens, &tok);
        } else if (iter.consume('-')) {
            Token tok;
            tok.kind = TOK_SUB;
            dynarray_push(tokens, &tok);
        } else if (iter.consume('*')) {
            Token tok;
            tok.kind = TOK_MUL;
            dynarray_push(tokens, &tok);
        } else if (iter.consume('/')) {
            Token tok;
            tok.kind = TOK_DIV;
            dynarray_push(tokens, &tok);
        } else if (iter.consume('(')) {
            Token tok;
            tok.kind = TOK_LEFT_PAREN;
            dynarray_push(tokens, &tok);
        } else if (iter.consume(')')) {
            Token tok;
            tok.kind = TOK_RIGHT_PAREN;
            dynarray_push(tokens, &tok);
        } else if (iter.consume(';')) {
            Token tok;
            tok.kind = TOK_SEMICOLON;
            dynarray_push(tokens, &tok);
        } else if (iter.consume('=')) {
            Token tok;
            tok.kind = TOK_ASSIGN;
            dynarray_push(tokens, &tok);
        } else if (iter.consume('{')) {
            Token tok;
            tok.kind = TOK_LEFT_CURLY_BRACKET;
            dynarray_push(tokens, &tok);
        } else if (iter.consume('}')) {
            Token tok;
            tok.kind = TOK_RIGHT_CURLY_BRACKET;
            dynarray_push(tokens, &tok);
        } else {
            fprintf(stderr, "Unexpected token: %c\n", c);
            exit(1);
        }
    }
}

enum NodeKind {
    NODE_ADD,
    NODE_ASSIGN,
    NODE_BLOCK,
    NODE_DIV,
    NODE_EXPR_STMT,
    NODE_IF,
    NODE_INT,
    NODE_MUL,
    NODE_PROGRAM,
    NODE_RETURN,
    NODE_SUB,
    NODE_VAR,
};

struct Variable {
    const char* ident;
    size_t stack_loc;
};

struct Scope {
    const Scope* parent;
    struct hashmap variables;
    size_t next_offset;
};

void Scope_init(struct Scope* scope, const struct Scope* parent)
{
    scope->parent = parent;
    hashmap_init(&scope->variables, sizeof(Variable));
    scope->next_offset = parent ? parent->next_offset : 0;
}

bool Scope_find(const struct Scope* scope, const char* ident, struct Variable* var)
{
    if (hashmap_get(&scope->variables, ident, var)) {
        return true;
    } else if (scope->parent) {
        return Scope_find(scope->parent, ident, var);
    } else {
        return false;
    }
}

void Scope_append(struct Scope* scope, const char* ident, struct Variable* var)
{
    var->ident = ident;
    var->stack_loc = scope->next_offset;
    scope->next_offset++;
    hashmap_set(&scope->variables, ident, var);
}

struct ASTNode {
    NodeKind kind;
    struct dynarray children;

    union {
        int64_t i64;
        Variable var;
    };
};

void ASTNode_init(struct ASTNode* node, NodeKind kind)
{
    node->kind = kind;
    dynarray_init(&node->children, sizeof(ASTNode));
}

void ASTNode_init_binary(struct ASTNode* node, NodeKind kind, ASTNode left, ASTNode right)
{
    dynarray_init_with_capacity(&node->children, sizeof(ASTNode), 2);
    dynarray_push(&node->children, &left);
    dynarray_push(&node->children, &right);
    node->kind = kind;
}

struct TokenIterator {
    Token* tokens;
    size_t size;
    size_t index;

    TokenIterator(Token* tokens, size_t size)
    {
        this->tokens = tokens;
        this->size = size;
        index = 0;
    }

    bool consume(TokenType kind)
    {
        if (index < size && tokens[index].kind == kind) {
            index++;
            return true;
        }
        return false;
    }

    bool consume(TokenType kind, Token* tok)
    {
        if (index < size && tokens[index].kind == kind) {
            *tok = tokens[index];
            index++;
            return true;
        }
        return false;
    }

    bool consume_keyword(const char* kw)
    {
        if (index < size && tokens[index].kind == TOK_IDENT && strcmp(tokens[index].ident, kw) == 0) {
            index++;
            return true;
        }

        return false;
    }
    
    // TODO: add error message
    void expect(TokenType kind)
    {
        if (index < size && tokens[index].kind == kind) {
            index++;
        } else {
            fprintf(stderr, "Expected token type %d, got %d instead\n", kind, tokens[index].kind);
            exit(1);
        }
    }

    int64_t expect_int()
    {
        if (index < size && tokens[index].kind == TOK_INT) {
            int64_t val = tokens[index].i64;
            index++;
            return val;
        }

        fprintf(stderr, "Expected int, got token kind %d instead\n", tokens[index].kind);
        exit(1);
    }

    bool has_next() {
        return index < size;
    }

    Token peek() {
        return tokens[index];
    }
};

ASTNode expr(TokenIterator& iter, struct Scope* scope);

const char* reserved_identifiers[] = {
    "return",
    "if",
    NULL
};

bool is_reserved(char* ident)
{
    for (unsigned int i = 0; reserved_identifiers[i] != NULL; i++) {
        if (strcmp(ident, reserved_identifiers[i]) == 0) {
            return true;
        }
    }
    return false;
}

// primary = '(' expr ')' | ident | int
ASTNode primary(TokenIterator& iter, struct Scope* scope)
{
    ASTNode node;

    Token tok;
    if (iter.consume(TOK_LEFT_PAREN)) {
        node = expr(iter, scope);
        iter.expect(TOK_RIGHT_PAREN);
    } else if (iter.consume(TOK_IDENT, &tok)) {
        if (is_reserved(tok.ident)) {
            fprintf(stderr, "Unexpected reserved identifier\n");
            exit(1);
        } else {
            ASTNode_init(&node, NODE_VAR);
            if (!Scope_find(scope, tok.ident, &node.var)) {
                Scope_append(scope, tok.ident, &node.var);
            }
        }
    } else {
        node.i64 = iter.expect_int();
        dynarray_init(&node.children, sizeof(ASTNode));
        node.kind = NODE_INT;
    }

    return node;
}

// mul_div = primary ( ('*' | '/') primary)*
ASTNode mul_div(TokenIterator& iter, struct Scope* scope)
{
    ASTNode node = primary(iter, scope);

    while (iter.has_next()) {
        if (iter.consume(TOK_MUL)) {
            ASTNode parent;
            ASTNode_init_binary(&parent, NODE_MUL, node, primary(iter, scope));
            node = parent;
        } else if (iter.consume(TOK_DIV)) {
            ASTNode parent;
            ASTNode_init_binary(&parent, NODE_DIV, node, primary(iter, scope));
            node = parent;
        } else {
            break;
        }
    }

    return node;
}

// add_sub = mul_div ( (+ | -) mul_div)*
ASTNode add_sub(TokenIterator& iter, struct Scope* scope)
{
    ASTNode node = mul_div(iter, scope);

    while (iter.has_next()) {
        if (iter.consume(TOK_ADD)) {
            ASTNode parent;
            ASTNode_init_binary(&parent, NODE_ADD, node, mul_div(iter, scope));
            node = parent;
        } else if (iter.consume(TOK_SUB)) {
            ASTNode parent;
            ASTNode_init_binary(&parent, NODE_SUB, node, mul_div(iter, scope));
            node = parent;
        } else {
            break;
        }
    }

    return node;
}

bool is_lvalue(ASTNode node)
{
    return node.kind == NODE_VAR;
}

// assign = add_sub ( '=' assign )
ASTNode assign_expr(TokenIterator& iter, struct Scope* scope)
{
    ASTNode lhs = add_sub(iter, scope);
    if (iter.consume(TOK_ASSIGN)) {
        if (!is_lvalue(lhs)) {
            fprintf(stderr, "Expected lvalue");
            exit(1);
        }
        ASTNode rhs = assign_expr(iter, scope);
        ASTNode node;
        // use the rhs as the left child so that it is evaluated first
        ASTNode_init_binary(&node, NODE_ASSIGN, rhs, lhs);
        return node;
    } else {
        return lhs;
    }
}

// expr = assign_expr
ASTNode expr(TokenIterator& iter, struct Scope* scope)
{
    return assign_expr(iter, scope);
}

ASTNode expr_statement(TokenIterator& iter, struct Scope* scope)
{
    ASTNode node;
    ASTNode_init(&node, NODE_EXPR_STMT);

    ASTNode child = expr(iter, scope);
    dynarray_push(&node.children, &child);

    iter.expect(TOK_SEMICOLON);
    return node;
}

// statement = 'return' expr ';' | 'if' '(' expr ')' statement | '{' statement* '}' | expr_statement
ASTNode statement(TokenIterator& iter, struct Scope* scope)
{
    ASTNode node;
    if (iter.consume_keyword("return")) {
        ASTNode_init(&node, NODE_RETURN);

        ASTNode child = expr(iter, scope);
        dynarray_push(&node.children, &child);
        iter.expect(TOK_SEMICOLON);
    } else if (iter.consume_keyword("if")) {
        ASTNode_init(&node, NODE_IF);
        iter.expect(TOK_LEFT_PAREN);
        ASTNode cond = expr(iter, scope);
        iter.expect(TOK_RIGHT_PAREN);
        ASTNode body = statement(iter, scope);
        dynarray_push(&node.children, &cond);
        dynarray_push(&node.children, &body);
    } else if (iter.consume(TOK_LEFT_CURLY_BRACKET)) {
        ASTNode_init(&node, NODE_BLOCK);
        while (!iter.consume(TOK_RIGHT_CURLY_BRACKET)) {
            struct Scope block_scope;
            Scope_init(&block_scope, scope);

            ASTNode child = statement(iter, &block_scope);
            dynarray_push(&node.children, &child);
        }
    } else {
        node = expr_statement(iter, scope);
    }
    return node;
}

// program = statement*
ASTNode program(TokenIterator& iter)
{
    ASTNode program;
    program.kind = NODE_PROGRAM;
    dynarray_init(&program.children, sizeof(ASTNode));

    struct Scope scope;
    Scope_init(&scope, NULL);
    while(iter.has_next()) {
        ASTNode node = statement(iter, &scope);
        dynarray_push(&program.children, &node);
    }

    return program;
}

void print_ast(ASTNode* node, unsigned int indent)
{
    for (unsigned int i = 0; i < indent; i++) {
        printf(" ");
    }

    switch(node->kind) {
        case NODE_ADD:
            printf("ADD\n");
            break;

        case NODE_ASSIGN:
            printf("ASSIGN\n");
            break;

        case NODE_BLOCK:
            printf("BLOCK\n");
            break;

        case NODE_SUB:
            printf("SUB\n");
            break;

        case NODE_IF:
            printf("IF\n");
            break;

        case NODE_INT:
            printf("%ld\n", node->i64);
            break;

        case NODE_MUL:
            printf("MUL\n");
            break;
            
        case NODE_DIV:
            printf("DIV\n");
            break;

        case NODE_EXPR_STMT:
            printf("NODE_EXPR_STMT\n");
            break;

        case NODE_PROGRAM:
            printf("PROGRAM\n");
            break;

        case NODE_RETURN:
            printf("RETURN\n");
            break;

        case NODE_VAR:
            printf("VAR %s\n", node->var.ident);
            break;
    }

    for (size_t i = 0; i < dynarray_length(&node->children); i++) {
        ASTNode* child = (ASTNode*)dynarray_get(&node->children, i);
        print_ast(child, indent+4);
    }
}

const char* asm_preamble =
    "global _start\n"
    "section .text\n"
    "_start:\n"
    "mov rbp, rsp\n";

const char* asm_conclusion =
   "mov rax,60\n"
   "pop rdi\n"
   "syscall\n";

/// Write the address of the lvalue in rax
void codegen_addr(ASTNode node, FILE* fp)
{
    switch(node.kind) {
        case NODE_VAR:
            fprintf(fp, "lea rax, [rbp-%lu]\n", node.var.stack_loc);
            break;

        default:
            fprintf(stderr, "Unknown lvalue type");
    }
}

void codegen_node(ASTNode node, FILE* fp);

void codegen_children(struct dynarray* children, FILE* fp)
{
    for (size_t i = 0; i < dynarray_length(children); i++)
    {
        ASTNode* child = (ASTNode*)dynarray_get(children, i);
        codegen_node(*child, fp);
    }
}

void codegen_node(ASTNode node, FILE* fp)
{
    switch(node.kind) {
        case NODE_INT:
            codegen_children(&node.children, fp);
            fprintf(fp, "push %ld\n", node.i64);
            break;

        case NODE_ADD:
            codegen_children(&node.children, fp);
            fprintf(fp, "pop rbx\npop rax\nadd rax, rbx\npush rax\n");
            break;

        case NODE_ASSIGN:
            {
                ASTNode* rhs = (ASTNode*)dynarray_get(&node.children, 0);
                codegen_node(*rhs, fp);

                ASTNode* lhs = (ASTNode*)dynarray_get(&node.children, 1);
                codegen_addr(*lhs, fp);

                // copy from top of the stack to [rax] (address of the local variable)
                // don't pop because assignment is an expression too
                fprintf(fp, "mov rbx,[rsp]\nmov [rax],rbx\n");
                break;
            }

        case NODE_BLOCK:
            codegen_children(&node.children, fp);
            break;

        case NODE_SUB:
            codegen_children(&node.children, fp);
            fprintf(fp, "pop rbx\npop rax\nsub rax, rbx\npush rax\n");
            break;

        case NODE_MUL:
            codegen_children(&node.children, fp);
            fprintf(fp, "pop rbx\npop rax\nimul rax, rbx\npush rax\n");
            break;
            
        case NODE_DIV:
            codegen_children(&node.children, fp);
            fprintf(fp, "pop rbx\npop rax\nidiv rbx\npush rax\n");
            break;

        case NODE_EXPR_STMT:
            codegen_children(&node.children, fp);
            fprintf(fp, "add rsp, 8\n");
            break;

        case NODE_IF:
            {
                ASTNode* cond = (ASTNode*)dynarray_get(&node.children, 0);

                ASTNode* body = (ASTNode*)dynarray_get(&node.children, 1);

                codegen_node(*cond, fp);

                fprintf(fp, "pop rax\ntest rax, rax\njz _false\n");

                codegen_node(*body, fp);

                fprintf(fp, "_false:\n");
                break;
            }

        case NODE_PROGRAM:
            ASSERT(0);
            break;

        case NODE_RETURN:
            codegen_children(&node.children, fp);
            fprintf(fp, "%s", asm_conclusion);
            break;

        case NODE_VAR:
            codegen_children(&node.children, fp);
            fprintf(fp, "push qword [rbp-%lu]\n", node.var.stack_loc);
            break;
    }
}

void codegen(ASTNode program, FILE* fp)
{
    fprintf(fp, "%s", asm_preamble);

    for (size_t i = 0; i < dynarray_length(&program.children); i++)
    {
        // TODO: write the code of the statement in an assembly comment
        fprintf(fp, "; statement %lu\n", i);
        ASTNode* child = (ASTNode*)dynarray_get(&program.children, i);
        codegen_node(*child, fp);
    }

    fprintf(fp, "%s", asm_conclusion);
}

void print_int(void* ptr)
{
    printf("%d\n", *(int*)ptr);
}

int main(int argc, char** argv)
{
    int f = 4;
    struct dynarray test;
    dynarray_init(&test, sizeof(int));
    dynarray_push(&test, &f);
    f = -5;
    dynarray_push(&test, &f);
    dynarray_map(&test, print_int);

    dynarray_destroy(&test);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <expr>\n", argv[0]);
        exit(1);
    }
    struct dynarray tokens;
    dynarray_init(&tokens, sizeof(Token));
    tokenize(&tokens, argv[1]);

    for (size_t i = 0; i < dynarray_length(&tokens); i++) {
        Token* tok = (Token*)dynarray_get(&tokens, i);
        print_token(*tok);
    }

    printf("\n");
    fflush(stdout);

    TokenIterator tok_iter((Token*)tokens.data, dynarray_length(&tokens));
    ASTNode ast = program(tok_iter);

    print_ast(&ast, 0);

    FILE* fp = fopen("out.asm", "w");
    if (!fp) {
        fprintf(stderr, "Failed to open out.asm");
        exit(1);
    }

    codegen(ast, fp);
    fclose(fp);

    dynarray_destroy(&tokens);

    return 0;
}
