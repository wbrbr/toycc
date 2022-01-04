#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include "dynarray.h"
#include "hashmap.h"
#include "toycc.h"
#include "util.h"

void print_token(struct Token tok)
{
    switch(tok.kind) {
        case TOK_ADD:
            printf("+ ");
            break;

        case TOK_ASSIGN:
            printf("=");
            break;

        case TOK_ASSIGN_ADD:
            printf("+= ");
            break;

        case TOK_COMMA:
            printf(", ");
            break;

        case TOK_DIV:
            printf("/ ");
            break;

        case TOK_EQUALS:
            printf("== ");
            break;

        case TOK_IDENT:
            printf("%s ", tok.data.ident);
            break;

        case TOK_INCREMENT:
            printf("++ ");
            break;

        case TOK_INT:
            printf("%ld ", tok.data.i64);
            break;

        case TOK_LEFT_CURLY_BRACKET:
            printf("{ ");
            break;

        case TOK_LEFT_PAREN:
            printf("(");
            break;

        case TOK_LESS_THAN:
            printf("< ");
            break;

        case TOK_MUL:
            printf("* ");
            break;

        case TOK_RIGHT_CURLY_BRACKET:
            printf("} ");
            break;

        case TOK_RIGHT_PAREN:
            printf(")");
            break;

        case TOK_SEMICOLON:
            printf(";");
            break;

        case TOK_SUB:
            printf("- ");
            break;
    }
}

int ast_to_dot_file_rec(FILE* fp, const struct ASTNode* node, int node_id, int parent_id)
{
    fprintf(fp, "n%d [label=\"", node_id);

    switch(node->kind) {
        case NODE_ADD:
            fprintf(fp, "+");
            break;

        case NODE_ASSIGN:
            fprintf(fp, "=");
            break;

        case NODE_ASSIGN_ADD:
            fprintf(fp, "+=");
            break;

        case NODE_BLOCK:
            fprintf(fp, "{ }");
            break;

        case NODE_DECL:
            fprintf(fp, "int %s", node->data.decl.ident);
            break;

        case NODE_DIV:
            fprintf(fp, "/");
            break;

        case NODE_EQUALS:
            fprintf(fp, "==");
            break;

        case NODE_EXPR_STMT:
            fprintf(fp, ";");
            break;

        case NODE_FOR:
            fprintf(fp, "for");
            break;

        case NODE_FUNCTION_DEF:
            fprintf(fp, "fun %s", node->data.decl.ident);
            break;

        case NODE_IDENT:
            fprintf(fp, "%s", node->data.decl.ident);
            break;

        case NODE_IF:
            fprintf(fp, "if");
            break;

        case NODE_INT:
            fprintf(fp, "%ld", node->data.i64);
            break;

        case NODE_LESS_THAN:
            fprintf(fp, "<");
            break;

        case NODE_MUL:
            fprintf(fp, "*");
            break;

        case NODE_POSTFIX_INCREMENT:
            fprintf(fp, "++");
            break;

        case NODE_PROGRAM:
            fprintf(fp, "PROGRAM");
            break;

        case NODE_RETURN:
            fprintf(fp, "return");
            break;

        case NODE_SUB:
            fprintf(fp, "-");
            break;

        case NODE_WHILE:
            fprintf(fp, "while");
            break;
    }
    fprintf(fp, "\"];\n");

    if (parent_id >= 0) {
        fprintf(fp, "n%d -> n%d;\n", parent_id, node_id);
    }

    fflush(fp);

    int child_id = node_id+1;
    for (unsigned int i = 0; i < dynarray_length(&node->children); i++) {
        child_id = ast_to_dot_file_rec(fp, dynarray_get(&node->children, i), child_id, node_id);
    }

    return child_id;
}

// returns the next available id
void ast_to_dot_file(FILE* fp, const struct ASTNode* node)
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
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    char* input = read_file(argv[1]);

    struct dynarray tokens;
    dynarray_init(&tokens, sizeof(struct Token));
    tokenize(&tokens, input);

    for (size_t i = 0; i < dynarray_length(&tokens); i++) {
        struct Token* tok = dynarray_get(&tokens, i);
        print_token(*tok);
    }

    printf("\n");
    fflush(stdout);

    struct ASTNode ast = parse(tokens);

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
