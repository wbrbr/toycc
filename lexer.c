#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "toycc.h"

struct CharIterator {
    const char *data;
    size_t index;
    size_t size;
};

static void CharIterator_init(struct CharIterator *iter, const char* data, size_t size) {
    iter->data = data;
    iter->index = 0;
    iter->size = size;
}

static char peek(const struct CharIterator* iter) {
    return iter->data[iter->index];
}

static char next(struct CharIterator* iter) {
    char c = iter->data[iter->index];
    iter->index++;
    return c;
}

static bool has_next(const struct CharIterator* iter) {
    return iter->index < iter->size;
}

static bool consume(struct CharIterator* iter, char c) {
    if (peek(iter) == c) {
        next(iter);
        return true;
    } else {
        return false;
    }
}

static bool consume_pred(struct CharIterator* iter, int (*pred)(int), char* c) {
    if (pred(peek(iter))) {
        char n = next(iter);
        if (c) {
            *c = n;
        }
        return true;
    } else {
        return false;
    }
}

static struct Token match_num(struct CharIterator* iter) {
    int64_t num = 0;
    while (isdigit(peek(iter))) {
        int v = next(iter) - '0';
        num = 10 * num + v;
    }

    struct Token tok;
    tok.kind = TOK_INT;
    tok.i64 = num;
    return tok;
}

static struct Token match_ident(struct CharIterator* iter)
{
    struct dynarray string;
    dynarray_init(&string, 1);

    char c;
    while (consume_pred(iter, isalnum, &c)) {
        dynarray_push(&string, &c);
    }

    char null = 0;
    dynarray_push(&string, &null);

    struct Token tok;
    tok.kind = TOK_IDENT;
    tok.ident = (char*)string.data;

    return tok;
}

void tokenize(struct dynarray* tokens, const char* input)
{
    struct CharIterator iter;
    CharIterator_init(&iter, input, strlen(input));

    while (has_next(&iter)) {
        struct Token tok;
        char c = peek(&iter);
        if (isspace(c)) {
            next(&iter);
            continue;
        } else if (isdigit(c)) {
            tok = match_num(&iter);
        } else if (isalpha(c)) {
            tok = match_ident(&iter);
        } else if (consume(&iter, '+')) {
            if (consume(&iter, '=')) {
                tok.kind = TOK_ASSIGN_ADD;
            } else if (consume(&iter, '+')) {
                tok.kind = TOK_INCREMENT;
            } else {
                tok.kind = TOK_ADD;
            }
        } else if (consume(&iter, '-')) {
            tok.kind = TOK_SUB;
        } else if (consume(&iter, '*')) {
            tok.kind = TOK_MUL;
        } else if (consume(&iter, '/')) {
            tok.kind = TOK_DIV;
        } else if (consume(&iter, '(')) {
            tok.kind = TOK_LEFT_PAREN;
        } else if (consume(&iter, ')')) {
            tok.kind = TOK_RIGHT_PAREN;
        } else if (consume(&iter, ';')) {
            tok.kind = TOK_SEMICOLON;
        } else if (consume(&iter, '=')) {
            if (consume(&iter, '=')) {
                tok.kind = TOK_EQUALS;
            } else {
                tok.kind = TOK_ASSIGN;
            }
        } else if (consume(&iter, '{')) {
            tok.kind = TOK_LEFT_CURLY_BRACKET;
        } else if (consume(&iter, '}')) {
            tok.kind = TOK_RIGHT_CURLY_BRACKET;
        } else if (consume(&iter, '<')) {
            tok.kind = TOK_LESS_THAN;
        } else if (consume(&iter, ',')) {
            tok.kind = TOK_COMMA;
        } else {
            fprintf(stderr, "Unexpected token: %c\n", c);
            exit(1);
        }
        dynarray_push(tokens, &tok);
    }
}
