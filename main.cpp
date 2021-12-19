#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <vector>
#include <string.h>

enum TokenType {
    TOK_INT,
    TOK_ADD,
    TOK_SUB,
    TOK_MUL,
    TOK_DIV,
    TOK_LEFT_PAREN,
    TOK_RIGHT_PAREN,
};

struct Token {
    TokenType kind;
    union {
        int64_t i64;
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
    }
}


template<typename T>
struct Iterator {
    const T* data;
    size_t index;
    size_t size;

    Iterator(const T* data, size_t size) {
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
};

using CharIterator = Iterator<char>;

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

std::vector<Token> tokenize(const char* input)
{
    CharIterator iter(input, strlen(input));
    std::vector<Token> tokens;

    while (iter.has_next()) {
        char c = iter.peek();
        if (isspace(c)) {
            iter.next();
        } else if (isdigit(c)) {
            tokens.push_back(match_num(iter));
        } else if (c == '+') {
            iter.next();
            Token tok;
            tok.kind = TOK_ADD;
            tokens.push_back(tok);
        } else if (c == '-') {
            iter.next();
            Token tok;
            tok.kind = TOK_SUB;
            tokens.push_back(tok);
        } else if (c == '*') {
            iter.next();
            Token tok;
            tok.kind = TOK_MUL;
            tokens.push_back(tok);
        } else if (c == '/') {
            iter.next();
            Token tok;
            tok.kind = TOK_DIV;
            tokens.push_back(tok);
        } else if (c == '(') {
            iter.next();
            Token tok;
            tok.kind = TOK_LEFT_PAREN;
            tokens.push_back(tok);
        } else if (c == ')') {
            iter.next();
            Token tok;
            tok.kind = TOK_RIGHT_PAREN;
            tokens.push_back(tok);
        } else {
            fprintf(stderr, "Unexpected token: %c\n", c);
            exit(1);
        }
    }

    return tokens;
}

enum NodeKind {
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_INT
};

struct ASTNode {
    ASTNode* children;
    size_t num_children;
    NodeKind kind;

    union {
        int64_t i64;
    };
};

ASTNode new_binary(NodeKind kind, ASTNode left, ASTNode right)
{
    ASTNode node;
    node.num_children = 2;
    node.children = (ASTNode*)malloc(2*sizeof(ASTNode));
    node.children[0] = left;
    node.children[1] = right;
    node.kind = kind;

    return node;
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
    
    void expect(TokenType kind)
    {
        if (index < size && tokens[index].kind == kind) {
            index++;
        } else {
            fprintf(stderr, "Unexpected token\n");
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

        fprintf(stderr, "Unexpected token\n");
        exit(1);
    }

    bool has_next() {
        return index < size;
    }

    Token peek() {
        return tokens[index];
    }
};

ASTNode expr(TokenIterator& iter);

// primary = '(' expr ')' | int
ASTNode primary(TokenIterator& iter)
{
    ASTNode node;

    if (iter.consume(TOK_LEFT_PAREN)) {
        node = expr(iter);
        iter.expect(TOK_RIGHT_PAREN);
    } else {
        node.i64 = iter.expect_int();
        node.children = nullptr;
        node.num_children = 0;
        node.kind = NODE_INT;
    }

    return node;
}

// mul_div = primary ( ('*' | '/') primary)*
ASTNode mul_div(TokenIterator& iter)
{
    ASTNode node = primary(iter);

    while (iter.has_next()) {
        if (iter.consume(TOK_MUL)) {
            ASTNode parent = new_binary(NODE_MUL, node, primary(iter));
            node = parent;
        } else if (iter.consume(TOK_DIV)) {
            ASTNode parent = new_binary(NODE_DIV, node, primary(iter));
            node = parent;
        } else {
            break;
        }
    }

    return node;
}

// expr = mul_div ( (+|-) mul_div)*
ASTNode expr(TokenIterator& iter)
{
    ASTNode node = mul_div(iter);

    while (iter.has_next()) {
        if (iter.consume(TOK_ADD)) {
            ASTNode parent = new_binary(NODE_ADD, node, mul_div(iter));
            node = parent;
        } else if (iter.consume(TOK_SUB)) {
            ASTNode parent = new_binary(NODE_SUB, node, mul_div(iter));
            node = parent;
        } else {
            break;
        }
    }

    return node;
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

        case NODE_SUB:
            printf("SUB\n");
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

        default:
            fprintf(stderr, "Unknown node kind\n");
            exit(1);
    }

    for (size_t i = 0; i < node->num_children; i++) {
        print_ast(&node->children[i], indent+4);
    }
}

const char* asm_preamble =
    "global _start\n"
    "section .text\n"
    "_start:\n";

const char* asm_conclusion =
   "mov rax,60\n"
   "pop rdi\n"
   "syscall\n";

void codegen_node(ASTNode node, FILE* fp)
{
    for (size_t i = 0; i < node.num_children; i++)
    {
        codegen_node(node.children[i], fp);
    }

    switch(node.kind) {
        case NODE_INT:
            fprintf(fp, "push %ld\n", node.i64);
            break;

        case NODE_ADD:
            fprintf(fp, "pop rbx\npop rax\nadd rax, rbx\npush rax\n");
            break;

        case NODE_SUB:
            fprintf(fp, "pop rbx\npop rax\nsub rax, rbx\npush rax\n");
            break;

        case NODE_MUL:
            fprintf(fp, "pop rbx\npop rax\nimul rax, rbx\npush rax\n");
            break;
            
        case NODE_DIV:
            fprintf(fp, "pop rbx\npop rax\nidiv rbx\npush rax\n");
            break;

        default:
            fprintf(stderr, "Unknown node kind\n");
            exit(1);
    }
}

void codegen(ASTNode root, FILE* fp)
{
    fprintf(fp, "%s", asm_preamble);

    codegen_node(root, fp);

    fprintf(fp, "%s", asm_conclusion);
}

int main(int argc, char** argv)
{
    std::vector<Token> tokens = tokenize("(3 + 2)*5 + 10/3");
    for (Token tok : tokens) {
        print_token(tok);
    }
    printf("\n");
    fflush(stdout);

    TokenIterator tok_iter(tokens.data(), tokens.size());
    ASTNode ast = expr(tok_iter);

    print_ast(&ast, 0);

    FILE* fp = fopen("out.asm", "w");
    if (!fp) {
        fprintf(stderr, "Failed to open out.asm");
        exit(1);
    }

    codegen(ast, fp);

    return 0;
}
