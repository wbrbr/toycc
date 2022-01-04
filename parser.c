#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "toycc.h"
#include "hashmap.h"
#include "dynarray.h"

struct Context {
    struct Scope* scope;
    unsigned int* frame_size;
};

void Scope_init(struct Scope* scope, const struct Scope* parent)
{
    scope->parent = parent;
    hashmap_init(&scope->decls, sizeof(struct Declaration));
}

bool Scope_find(const struct Scope* scope, const char* ident, struct Declaration* var)
{
    if (hashmap_get(&scope->decls, ident, var)) {
        return true;
    } else if (scope->parent) {
        return Scope_find(scope->parent, ident, var);
    } else {
        return false;
    }
}

void Scope_append(struct Scope* scope, const struct Declaration* var)
{
    // TODO: do a single query
    struct Declaration scratch;
    if (hashmap_get(&scope->decls, var->ident, &scratch)) {
        fprintf(stderr, "Identifier already declared in this scope: %s\n", var->ident);
        exit(1);
    }
    hashmap_set(&scope->decls, var->ident, var);
}

void ASTNode_init(struct ASTNode* node, enum NodeKind kind)
{
    node->kind = kind;
    dynarray_init(&node->children, sizeof(struct ASTNode));
}

void ASTNode_init_binary(struct ASTNode* node, enum NodeKind kind, struct ASTNode left, struct ASTNode right)
{
    dynarray_init_with_capacity(&node->children, sizeof(struct ASTNode), 2);
    dynarray_push(&node->children, &left);
    dynarray_push(&node->children, &right);
    node->kind = kind;
}

struct TokenIterator {
    struct Token *tokens;
    size_t size;
    size_t index;
};

static void TokenIterator_init(struct TokenIterator* iter, struct Token* tokens, size_t size)
{
    iter->tokens = tokens;
    iter->size = size;
    iter->index = 0;
}

static bool consume(struct TokenIterator* iter, enum TokenType kind)
{
    if (iter->index < iter->size && iter->tokens[iter->index].kind == kind) {
        iter->index++;
        return true;
    }
    return false;
}

static struct Token* consume_tok(struct TokenIterator* iter, enum TokenType kind)
{
    if (iter->index < iter->size && iter->tokens[iter->index].kind == kind) {
        struct Token* tok = &iter->tokens[iter->index];
        iter->index++;
        return tok;
    }
    return NULL;
}

static bool consume_keyword(struct TokenIterator* iter, const char* kw)
{
    if (iter->index < iter->size && iter->tokens[iter->index].kind == TOK_IDENT && strcmp(iter->tokens[iter->index].ident, kw) == 0) {
        iter->index++;
        return true;
    }

    return false;
}

// TODO: add error message
static void expect(struct TokenIterator* iter, enum TokenType kind)
{
    if (iter->index < iter->size && iter->tokens[iter->index].kind == kind) {
        iter->index++;
    } else {
        fprintf(stderr, "Expected token type %d, got %d instead\n", kind, iter->tokens[iter->index].kind);
        exit(1);
    }
}

static int64_t expect_int(struct TokenIterator* iter)
{
    if (iter->index < iter->size && iter->tokens[iter->index].kind == TOK_INT) {
        int64_t val = iter->tokens[iter->index].i64;
        iter->index++;
        return val;
    }

    fprintf(stderr, "Expected int, got token kind %d instead\n", iter->tokens[iter->index].kind);
    exit(1);
}

static bool has_next(struct TokenIterator* iter) {
    return iter->index < iter->size;
}

static bool peek(struct TokenIterator* iter, enum TokenType kind) {
    return iter->index < iter->size && iter->tokens[iter->index].kind == kind;
}

static struct ASTNode expr(struct TokenIterator* iter, struct Context ctx);

const char* reserved_identifiers[] = {
        "else",
        "if",
        "int",
        "return",
        "while",
        NULL
};

static bool is_reserved(char* ident)
{
    for (unsigned int i = 0; reserved_identifiers[i] != NULL; i++) {
        if (strcmp(ident, reserved_identifiers[i]) == 0) {
            return true;
        }
    }
    return false;
}

// primary = '(' expr ')' | ident | int
static struct ASTNode primary(struct TokenIterator* iter, struct Context ctx)
{
    struct ASTNode node;

    struct Token* tok;
    if (consume(iter, TOK_LEFT_PAREN)) {
        node = expr(iter, ctx);
        expect(iter,TOK_RIGHT_PAREN);
    } else if ((tok = consume_tok(iter, TOK_IDENT))) {
        if (is_reserved(tok->ident)) {
            fprintf(stderr, "Unexpected reserved identifier\n");
            exit(1);
        } else {
            ASTNode_init(&node, NODE_IDENT);
            if (!Scope_find(ctx.scope, tok->ident, &node.decl)) {
                fprintf(stderr, "Unknown identifier: %s\n", tok->ident);
                exit(1);
            }
        }
    } else {
        node.i64 = expect_int(iter);
        dynarray_init(&node.children, sizeof(struct ASTNode));
        node.kind = NODE_INT;
    }

    return node;
}

// mul_div = primary ( ('*' | '/') primary)*
static struct ASTNode mul_div(struct TokenIterator* iter, struct Context ctx)
{
    struct ASTNode node = primary(iter, ctx);

    while (has_next(iter)) {
        if (consume(iter, TOK_MUL)) {
            struct ASTNode parent;
            ASTNode_init_binary(&parent, NODE_MUL, node, primary(iter, ctx));
            node = parent;
        } else if (consume(iter, TOK_DIV)) {
            struct ASTNode parent;
            ASTNode_init_binary(&parent, NODE_DIV, node, primary(iter, ctx));
            node = parent;
        } else {
            break;
        }
    }

    return node;
}

// add_sub = mul_div ( (+ | -) mul_div)*
static struct ASTNode add_sub(struct TokenIterator* iter, struct Context ctx)
{
    struct ASTNode node = mul_div(iter, ctx);

    while (has_next(iter)) {
        if (consume(iter, TOK_ADD)) {
            struct ASTNode parent;
            ASTNode_init_binary(&parent, NODE_ADD, node, mul_div(iter, ctx));
            node = parent;
        } else if (consume(iter, TOK_SUB)) {
            struct ASTNode parent;
            ASTNode_init_binary(&parent, NODE_SUB, node, mul_div(iter, ctx));
            node = parent;
        } else {
            break;
        }
    }

    return node;
}

// relational_expr = add_sub ( '<' add_sub )*
static struct ASTNode relational_expr(struct TokenIterator* iter, struct Context ctx)
{
    struct ASTNode node = add_sub(iter, ctx);

    while (consume(iter, TOK_LESS_THAN)) {
        struct ASTNode parent;
        ASTNode_init_binary(&parent, NODE_LESS_THAN, node, add_sub(iter, ctx));
        node = parent;
    }

    return node;
}

// equality_expr = relational_expr ( '==' relational_expr )*
static struct ASTNode equality_expr(struct TokenIterator* iter, struct Context ctx)
{
    struct ASTNode node = relational_expr(iter, ctx);

    while (consume(iter, TOK_EQUALS)) {
        struct ASTNode parent;
        ASTNode_init_binary(&parent, NODE_EQUALS, node, relational_expr(iter, ctx));
        node = parent;
    }

    return node;
}

static bool is_lvalue(struct ASTNode node)
{
    return node.kind == NODE_IDENT || node.decl.kind == DECL_VARIABLE;
}

// assign = equality_expr ( '=' equality_expr )
static struct ASTNode assign_expr(struct TokenIterator* iter, struct Context ctx)
{
    struct ASTNode lhs = equality_expr(iter, ctx);
    if (consume(iter, TOK_ASSIGN)) {
        if (!is_lvalue(lhs)) {
            fprintf(stderr, "Expected lvalue");
            exit(1);
        }
        struct ASTNode rhs = assign_expr(iter, ctx);
        struct ASTNode node;

        ASTNode_init_binary(&node, NODE_ASSIGN, lhs, rhs);
        return node;
    } else {
        return lhs;
    }
}

// expr = assign_expr
static struct ASTNode expr(struct TokenIterator* iter, struct Context ctx)
{
    return assign_expr(iter, ctx);
}

static struct ASTNode expr_statement(struct TokenIterator* iter, struct Context ctx)
{
    struct ASTNode node;
    ASTNode_init(&node, NODE_EXPR_STMT);

    struct ASTNode child = expr(iter, ctx);
    dynarray_push(&node.children, &child);

    expect(iter, TOK_SEMICOLON);
    return node;
}

static struct ASTNode compound_statement(struct TokenIterator* iter, struct Context ctx);

// statement = 'return' expr ';'
//           | 'if' '(' expr ')' statement ( 'else' statement )?
//           | 'while' '(' expr ')' statement
//           | 'int' ident ( '=' assign_expr )? ';'
//           | compound_statement
//           | expr_statement
static struct ASTNode statement(struct TokenIterator* iter, struct Context ctx)
{
    struct ASTNode node;
    if (consume_keyword(iter, "return")) {
        ASTNode_init(&node, NODE_RETURN);

        struct ASTNode child = expr(iter, ctx);
        dynarray_push(&node.children, &child);
        expect(iter, TOK_SEMICOLON);
    } else if (consume_keyword(iter, "if")) {
        ASTNode_init(&node, NODE_IF);
        expect(iter, TOK_LEFT_PAREN);
        struct ASTNode cond = expr(iter, ctx);
        expect(iter, TOK_RIGHT_PAREN);
        struct ASTNode body = statement(iter, ctx);
        dynarray_push(&node.children, &cond);
        dynarray_push(&node.children, &body);

        if (consume_keyword(iter, "else")) {
            struct ASTNode else_body = statement(iter, ctx);
            dynarray_push(&node.children, &else_body);
        }
    } else if (consume_keyword(iter, "while")) {
        ASTNode_init(&node, NODE_WHILE);
        expect(iter, TOK_LEFT_PAREN);
        struct ASTNode cond = expr(iter, ctx);
        expect(iter, TOK_RIGHT_PAREN);
        struct ASTNode body = statement(iter, ctx);
        dynarray_push(&node.children, &cond);
        dynarray_push(&node.children, &body);
    } else if (consume_keyword(iter, "int")) {
        ASTNode_init(&node, NODE_DECL);
        struct Token* ident = consume_tok(iter, TOK_IDENT);

        if (consume(iter, TOK_ASSIGN)) {
            struct ASTNode rhs = assign_expr(iter, ctx);

            struct ASTNode assign;
            ASTNode_init(&assign, NODE_ASSIGN);

            struct ASTNode lvalue;
            ASTNode_init(&lvalue, NODE_IDENT);
            dynarray_push(&node.children, &rhs);
        }

        expect(iter, TOK_SEMICOLON);
        node.decl.ident = ident->ident;
        node.decl.kind = DECL_VARIABLE;
        node.decl.var_decl.stack_loc = *ctx.frame_size;
        *ctx.frame_size += 8;
        Scope_append(ctx.scope, &node.decl);
    } else if (peek(iter, TOK_LEFT_CURLY_BRACKET)) {
        node = compound_statement(iter, ctx);
    } else {
        node = expr_statement(iter, ctx);
    }
    return node;
}

// compound_statement = '{' statement* '}'
static struct ASTNode compound_statement(struct TokenIterator* iter, struct Context ctx)
{
    expect(iter, TOK_LEFT_CURLY_BRACKET);

    struct ASTNode node;
    ASTNode_init(&node, NODE_BLOCK);

    // TODO: use a better datastructure
    struct Scope* scope = malloc(sizeof(struct Scope));
    Scope_init(scope, ctx.scope);

    struct Context block_ctx = ctx;
    ctx.scope = scope;

    while (!consume(iter, TOK_RIGHT_CURLY_BRACKET)) {
        struct ASTNode child = statement(iter, block_ctx);
        dynarray_push(&node.children, &child);
    }

    return node;
}

static struct ASTNode function_definition(struct TokenIterator* iter, struct Scope* scope)
{
    if (consume_keyword(iter, "int")) {
        struct Token* tok = consume_tok(iter, TOK_IDENT);

        struct Declaration decl;
        decl.ident = tok->ident;
        decl.kind = DECL_FUNCTION;
        decl.fun_decl.frame_size = 0;

        expect(iter, TOK_LEFT_PAREN);

        struct Scope fun_scope;
        Scope_init(&fun_scope, scope);

        while (!consume(iter, TOK_RIGHT_PAREN)) {
            if (!consume_keyword(iter, "int")) {
                fprintf(stderr, "invalid parameter declaration\n");
                exit(1);
            }

            struct Token* param = consume_tok(iter, TOK_IDENT);

            struct Declaration param_decl;
            param_decl.kind = DECL_VARIABLE;
            param_decl.ident = param->ident;

            param_decl.var_decl.stack_loc = decl.fun_decl.frame_size;
            decl.fun_decl.frame_size += 8;

            Scope_append(&fun_scope, &param_decl);

            consume(iter, TOK_COMMA);
        }

        struct Context ctx;
        ctx.scope = &fun_scope;
        ctx.frame_size = &decl.fun_decl.frame_size;

        // we have to declare the function before parsing the body even though we don't know the frame size yet
        // otherwise we can't handle recursion
        Scope_append(scope, &decl);

        struct ASTNode body = compound_statement(iter, ctx);

        // overwrite the declaration with the correct frame size
        hashmap_set(&scope->decls, tok->ident, &decl);

        struct ASTNode node;
        ASTNode_init(&node, NODE_FUNCTION_DEF);
        node.decl = decl;
        dynarray_push(&node.children, &body);

        return node;
    } else {
        fprintf(stderr, "expected function definition\n");
        exit(1);
    }
}

// program = statement*
struct ASTNode parse(struct dynarray tokens)
{
    struct TokenIterator iter;
    TokenIterator_init(&iter, tokens.data, dynarray_length(&tokens));
    struct ASTNode program;
    program.kind = NODE_PROGRAM;
    dynarray_init(&program.children, sizeof(struct ASTNode));

    struct Scope scope;
    Scope_init(&scope, NULL);
    while(has_next(&iter)) {
        //struct ASTNode node = statement(&iter, &scope);
        struct ASTNode node = function_definition(&iter, &scope);
        dynarray_push(&program.children, &node);
    }

    return program;
}
