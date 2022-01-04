#include <stdio.h>
#include "toycc.h"
#include "util.h"

const char* asm_preamble =
        "global _start\n"
        "section .text\n"
        "_start:\n"
        "mov rbp, rsp\n"
        "call main\n"
        "mov rdi, rax\n"
        "mov rax, 60\n"
        "syscall\n";

void load_stack_loc_rax(size_t stack_loc, FILE* fp)
{
    fprintf(fp, "lea rax, [rbp-%lu]\n", stack_loc);
}

/// Write the address of the lvalue in rax
void codegen_addr(struct ASTNode node, FILE* fp)
{
    switch(node.kind) {
        case NODE_IDENT:
            load_stack_loc_rax(node.decl.var_decl.stack_loc, fp);
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

/// the address of the lvalue must be in rax
/// the value must be on top of the stack
void codegen_assign(FILE* fp)
{
    // copy from top of the stack to [rax] (address of the local variable)
    // don't pop because assignment is an expression too
    fprintf(fp, "mov rbx,[rsp]\n"
                "mov [rax],rbx\n"
                "sub rsp,8\n");
}

void codegen_node(struct ASTNode node, FILE* fp)
{
    // it would be better to use an atomic if we ever want to multithread this
    // but it is not in C99 and I want toycc to be able to compile itself. I might
    // I decide to implement C11 atomics. For now, I don't plan to multithread toycc
    // so this shouldn't be problematic
    static unsigned int label_num = 0;

    // TODO: alphabetical order
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
            struct ASTNode* rhs = dynarray_get(&node.children, 1);
            codegen_node(*rhs, fp);

            struct ASTNode* lhs = dynarray_get(&node.children, 0);
            codegen_addr(*lhs, fp);

            codegen_assign(fp);
            break;
        }

        case NODE_BLOCK:
            codegen_children(&node.children, fp);
            break;

        case NODE_DECL:
            // codegen the rhs
            codegen_children(&node.children, fp);
            // now the value is on top of the stack

            // we need to put the address of the lvalue in rax
            load_stack_loc_rax(node.decl.var_decl.stack_loc, fp);

            codegen_assign(fp);
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

        case NODE_EQUALS:
            codegen_children(&node.children, fp);
            fprintf(fp, "pop rcx\n"
                        "pop rbx\n"
                        "xor rax,rax\n"
                        "cmp rbx,rcx\n"
                        "sete al\n"
                        "push rax\n");
            break;

        case NODE_EXPR_STMT:
            codegen_children(&node.children, fp);
            fprintf(fp, "add rsp, 8\n");
            break;

        case NODE_FUNCTION_DEF:
            fprintf(fp, "%s:\n"
                        "push rbp\n"
                        "mov rbp, rsp\n"
                        "sub rsp, %u\n", node.decl.ident, node.decl.fun_decl.frame_size);
            codegen_children(&node.children, fp);
            fprintf(fp, "mov rsp, rbp\n"
                        "pop rbp\n"
                        "ret\n");
            break;

        case NODE_IF:
        {
            unsigned int cur_label = label_num;
            label_num++;

            struct ASTNode* cond = dynarray_get(&node.children, 0);

            struct ASTNode* body = dynarray_get(&node.children, 1);

            codegen_node(*cond, fp);

            fprintf(fp, "pop rax\ntest rax, rax\njz if.false.%u\n", cur_label);

            codegen_node(*body, fp);
            fprintf(fp, "jmp if.end.%u\n", cur_label);

            fprintf(fp, "if.false.%u:\n", cur_label);

            // else branch
            if (dynarray_length(&node.children) == 3) {
                struct ASTNode* else_body = dynarray_get(&node.children, 2);
                codegen_node(*else_body, fp);
            }

            fprintf(fp, "if.end.%u:\n", cur_label);
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
            fprintf(fp, "pop rax\n"
                        "mov rsp, rbp\n"
                        "pop rbp\n"
                        "ret\n");
            break;

        case NODE_IDENT:
            codegen_children(&node.children, fp);
            fprintf(fp, "push qword [rbp-%lu]\n", node.decl.var_decl.stack_loc);
            break;

        case NODE_WHILE:
        {
            unsigned int cur_label = label_num;
            label_num++;
            struct ASTNode* cond = dynarray_get(&node.children, 0);
            struct ASTNode* body = dynarray_get(&node.children, 1);

            fprintf(fp, "while.cond.%u:\n", cur_label);
            codegen_node(*cond, fp);

            fprintf(fp, "pop rax\ntest rax,rax\njz while.end.%u\n", cur_label);

            codegen_node(*body, fp);

            fprintf(fp, "jmp while.cond.%u\nwhile.end.%u:\n", cur_label, cur_label);
            break;
        }
    }
}

void codegen(struct ASTNode program, FILE* fp)
{
    fprintf(fp, "%s", asm_preamble);

    for (size_t i = 0; i < dynarray_length(&program.children); i++)
    {
        fprintf(fp, "; statement %lu\n", i);
        struct ASTNode* child = dynarray_get(&program.children, i);
        codegen_node(*child, fp);
    }
}
