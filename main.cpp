#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <vector>
#include <string.h>
extern "C" {
#include "dynarray.h"
#include "hashmap.h"
#include "toycc.h"
}


void print_token(struct Token tok)
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

        case TOK_LESS_THAN:
            printf("<");
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
        } else if (iter.consume('<')) {
            Token tok;
            tok.kind = TOK_LESS_THAN;
            dynarray_push(tokens, &tok);
        } else {
            fprintf(stderr, "Unexpected token: %c\n", c);
            exit(1);
        }
    }
}

int ast_to_dot_file_rec(FILE* fp, const ASTNode* node, int node_id, int parent_id)
{
    fprintf(fp, "n%d [label=\"", node_id);

    // TODO: alphabetical order
    switch(node->kind) {
        case NODE_ADD:
            fprintf(fp, "+");
            break;

        case NODE_ASSIGN:
            fprintf(fp, "=");
            break;

        case NODE_BLOCK:
            fprintf(fp, "{ }");
            break;

        case NODE_DECL:
            fprintf(fp, "int %s\n", node->var.ident);
            break;

        case NODE_SUB:
            fprintf(fp, "-");
            break;

        case NODE_IF:
            fprintf(fp, "if");
            break;

        case NODE_INT:
            fprintf(fp, "%ld", node->i64);
            break;

        case NODE_MUL:
            fprintf(fp, "*");
            break;

        case NODE_DIV:
            fprintf(fp, "/");
            break;

        case NODE_EXPR_STMT:
            fprintf(fp, ";");
            break;

        case NODE_PROGRAM:
            fprintf(fp, "PROGRAM");
            break;

        case NODE_RETURN:
            fprintf(fp, "return");
            break;

        case NODE_VAR:
            fprintf(fp, "%s", node->var.ident);
            break;

        case NODE_LESS_THAN:
            fprintf(fp, "<=");
            break;

        case NODE_WHILE:
            fprintf(fp, "while");
            break;
    }
    fprintf(fp, "\"];\n");

    if (parent_id >= 0) {
        fprintf(fp, "n%d -> n%d;\n", parent_id, node_id);
    }

    int child_id = node_id+1;
    for (unsigned int i = 0; i < dynarray_length(&node->children); i++) {
        child_id = ast_to_dot_file_rec(fp, (ASTNode*)dynarray_get(&node->children, i), child_id, node_id);
    }

    return child_id;
}

// returns the next available id
void ast_to_dot_file(FILE* fp, const ASTNode* node)
{
    fprintf(fp, "digraph {\n");
    ast_to_dot_file_rec(fp, node, 0, -1);
    fprintf(fp, "}\n");
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

    ASTNode ast = parse(tokens);

    FILE* dot = fopen("ast.dot", "w");
    ast_to_dot_file(dot, &ast);
    fclose(dot);

    FILE* fp = fopen("out.s", "w");
    if (!fp) {
        fprintf(stderr, "Failed to open out.asm");
        exit(1);
    }

    codegen(ast, fp);
    fclose(fp);

    dynarray_destroy(&tokens);

    return 0;
}
