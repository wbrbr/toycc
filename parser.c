#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "toycc.h"
#include "hashmap.h"
#include "dynarray.h"

void Scope_init(struct Scope* scope, const struct Scope* parent)
{
    scope->parent = parent;
    hashmap_init(&scope->variables, sizeof(struct Variable));
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
    scope->next_offset += 8;
    hashmap_set(&scope->variables, ident, var);
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

static struct Token peek(struct TokenIterator* iter) {
    return iter->tokens[iter->index];
}

static struct ASTNode expr(struct TokenIterator* iter, struct Scope* scope);

const char* reserved_identifiers[] = {
        "return",
        "if",
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
static struct ASTNode primary(struct TokenIterator* iter, struct Scope* scope)
{
    struct ASTNode node;

    struct Token* tok;
    if (consume(iter, TOK_LEFT_PAREN)) {
        node = expr(iter, scope);
        expect(iter,TOK_RIGHT_PAREN);
    } else if ((tok = consume_tok(iter, TOK_IDENT))) {
        if (is_reserved(tok->ident)) {
            fprintf(stderr, "Unexpected reserved identifier\n");
            exit(1);
        } else {
            ASTNode_init(&node, NODE_VAR);
            if (!Scope_find(scope, tok->ident, &node.var)) {
                Scope_append(scope, tok->ident, &node.var);
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
static struct ASTNode mul_div(struct TokenIterator* iter, struct Scope* scope)
{
    struct ASTNode node = primary(iter, scope);

    while (has_next(iter)) {
        if (consume(iter, TOK_MUL)) {
            struct ASTNode parent;
            ASTNode_init_binary(&parent, NODE_MUL, node, primary(iter, scope));
            node = parent;
        } else if (consume(iter, TOK_DIV)) {
            struct ASTNode parent;
            ASTNode_init_binary(&parent, NODE_DIV, node, primary(iter, scope));
            node = parent;
        } else {
            break;
        }
    }

    return node;
}

// add_sub = mul_div ( (+ | -) mul_div)*
static struct ASTNode add_sub(struct TokenIterator* iter, struct Scope* scope)
{
    struct ASTNode node = mul_div(iter, scope);

    while (has_next(iter)) {
        if (consume(iter, TOK_ADD)) {
            struct ASTNode parent;
            ASTNode_init_binary(&parent, NODE_ADD, node, mul_div(iter, scope));
            node = parent;
        } else if (consume(iter, TOK_SUB)) {
            struct ASTNode parent;
            ASTNode_init_binary(&parent, NODE_SUB, node, mul_div(iter, scope));
            node = parent;
        } else {
            break;
        }
    }

    return node;
}

// relational_expr = add_sub ( '<' add_sub )*
static struct ASTNode relational_expr(struct TokenIterator* iter, struct Scope* scope)
{
    struct ASTNode node = add_sub(iter, scope);

    while (consume(iter, TOK_LESS_THAN)) {
        struct ASTNode parent;
        ASTNode_init_binary(&parent, NODE_LESS_THAN, node, add_sub(iter, scope));
        node = parent;
    }

    return node;
}

static bool is_lvalue(struct ASTNode node)
{
    return node.kind == NODE_VAR;
}

// assign = relational_expr ( '=' assign )
static struct ASTNode assign_expr(struct TokenIterator* iter, struct Scope* scope)
{
    struct ASTNode lhs = relational_expr(iter, scope);
    if (consume(iter, TOK_ASSIGN)) {
        if (!is_lvalue(lhs)) {
            fprintf(stderr, "Expected lvalue");
            exit(1);
        }
        struct ASTNode rhs = assign_expr(iter, scope);
        struct ASTNode node;

        ASTNode_init_binary(&node, NODE_ASSIGN, lhs, rhs);
        return node;
    } else {
        return lhs;
    }
}

// expr = assign_expr
static struct ASTNode expr(struct TokenIterator* iter, struct Scope* scope)
{
    return assign_expr(iter, scope);
}

static struct ASTNode expr_statement(struct TokenIterator* iter, struct Scope* scope)
{
    struct ASTNode node;
    ASTNode_init(&node, NODE_EXPR_STMT);

    struct ASTNode child = expr(iter, scope);
    dynarray_push(&node.children, &child);

    expect(iter, TOK_SEMICOLON);
    return node;
}

// statement = 'return' expr ';'
//           | 'if' '(' expr ')' statement
//           | 'while' '(' expr ')' statement
//           | '{' statement* '}'
//           | expr_statement
static struct ASTNode statement(struct TokenIterator* iter, struct Scope* scope)
{
    struct ASTNode node;
    if (consume_keyword(iter, "return")) {
        ASTNode_init(&node, NODE_RETURN);

        struct ASTNode child = expr(iter, scope);
        dynarray_push(&node.children, &child);
        expect(iter, TOK_SEMICOLON);
    } else if (consume_keyword(iter, "if")) {
        ASTNode_init(&node, NODE_IF);
        expect(iter, TOK_LEFT_PAREN);
        struct ASTNode cond = expr(iter, scope);
        expect(iter, TOK_RIGHT_PAREN);
        struct ASTNode body = statement(iter, scope);
        dynarray_push(&node.children, &cond);
        dynarray_push(&node.children, &body);
    } else if (consume_keyword(iter, "while")) {
        ASTNode_init(&node, NODE_WHILE);
        expect(iter, TOK_LEFT_PAREN);
        struct ASTNode cond = expr(iter, scope);
        expect(iter, TOK_RIGHT_PAREN);
        struct ASTNode body = statement(iter, scope);
        dynarray_push(&node.children, &cond);
        dynarray_push(&node.children, &body);
    } else if (consume(iter, TOK_LEFT_CURLY_BRACKET)) {
        ASTNode_init(&node, NODE_BLOCK);
        while (!consume(iter, TOK_RIGHT_CURLY_BRACKET)) {
            struct Scope block_scope;
            Scope_init(&block_scope, scope);

            struct ASTNode child = statement(iter, &block_scope);
            dynarray_push(&node.children, &child);
        }
    } else {
        node = expr_statement(iter, scope);
    }
    return node;
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
        struct ASTNode node = statement(&iter, &scope);
        dynarray_push(&program.children, &node);
    }

    return program;
}
