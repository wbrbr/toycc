#include <stdio.h>
#include "toycc.h"
#include "util.h"

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
void codegen_addr(struct ASTNode node, FILE* fp)
{
    switch(node.kind) {
        case NODE_VAR:
            fprintf(fp, "lea rax, [rbp-%lu]\n", node.var.stack_loc);
            break;

        default:
            fprintf(stderr, "Unknown lvalue type");
    }
}

void codegen_node(struct ASTNode node, FILE* fp);

void codegen_children(struct dynarray* children, FILE* fp)
{
    for (size_t i = 0; i < dynarray_length(children); i++)
    {
        struct ASTNode* child = dynarray_get(children, i);
        codegen_node(*child, fp);
    }
}

void codegen_node(struct ASTNode node, FILE* fp)
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
            struct ASTNode* rhs = dynarray_get(&node.children, 0);
            codegen_node(*rhs, fp);

            struct ASTNode* lhs = dynarray_get(&node.children, 1);
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
            // TODO: use a counter for the labels
            // TODO: else
            struct ASTNode* cond = dynarray_get(&node.children, 0);

            struct ASTNode* body = dynarray_get(&node.children, 1);

            codegen_node(*cond, fp);

            fprintf(fp, "pop rax\ntest rax, rax\njz _false\n");

            codegen_node(*body, fp);

            fprintf(fp, "_false:\n");
            break;
        }

        case NODE_LESS_THAN:
            codegen_children(&node.children, fp);
            fprintf(fp,"pop rcx\n"
                              "pop rbx\n"
                              "xor rax,rax\n"
                              "cmp rbx,rcx\n"
                              "setl al\n"
                              "push rax\n");
            break;

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

        case NODE_WHILE:
        {
            struct ASTNode* cond = dynarray_get(&node.children, 0);
            struct ASTNode* body = dynarray_get(&node.children, 1);

            fprintf(fp, "while.cond:\n");
            codegen_node(*cond, fp);

            fprintf(fp, "pop rax\ntest rax,rax\njz while.end\n");

            codegen_node(*body, fp);

            fprintf(fp, "jmp while.cond\nwhile.end:\n");
            break;
        }
    }
}

void codegen(struct ASTNode program, FILE* fp)
{
    fprintf(fp, "%s", asm_preamble);

    for (size_t i = 0; i < dynarray_length(&program.children); i++)
    {
        // TODO: write the code of the statement in an assembly comment
        fprintf(fp, "; statement %lu\n", i);
        struct ASTNode* child = dynarray_get(&program.children, i);
        codegen_node(*child, fp);
    }

    fprintf(fp, "%s", asm_conclusion);
}
