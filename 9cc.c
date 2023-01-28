#include <bits/pthreadtypes.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  ND_ADD,
  ND_SUB,
  ND_MUL,
  ND_DIV,
  ND_PRIM,
  ND_NUM,
  ND_GT,
  ND_GE,
  ND_EQL,
  ND_NEQ,
} NodeKind;

typedef struct Node Node;

struct Node {
  NodeKind kind;
  Node *lhs;
  Node *rhs;
  int val;
};

typedef enum {
  TK_RESERVED, // 記号
  TK_NUM,      // 整数トークン
  TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;
char *user_input;

struct Token {
  TokenKind kind; // トークンの型
  Token *next;    // 次の入力トークン
  int val;        // kindがTK_NUMの場合、その数値
  char *str;      // トークン文字列
  int len;        // トークンの長さ
};

Token *token;
bool dbg;

void error(char *loc, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  exit(1);
}

bool startswith(char *str, char *tar) { 
  int ret = memcmp(str, tar, strlen(tar));
  if (dbg && ret)
    printf("[StartsWith] |%s| |%s|",str,tar);
  return !ret;
}

bool consume(char *op) {
  if (dbg)
    printf("[consume] %s |%s| %d\n", op, token->str, token->len);
  bool isReserved = token->kind == TK_RESERVED;
  bool isValid = !memcmp(token->str, op, token->len);
  if (isReserved && isValid) {
    token = token->next;
    return true;
  }
  return false;
}

int expect_number() {
  if (token->kind != TK_NUM) {
    printf("%s\n", token->str);
    error(token->str, "%s is not a number\n");
  }
  int val = token->val;
  token = token->next;
  return val;
}

bool at_eof() { return token->kind == TK_EOF; }

Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->str = str;
  tok->len = len;
  cur->next = tok;
  return tok;
}

Node *nd_new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *nd_new_num(int val) {
  Node *node = calloc(1, sizeof(Node));
  node->val = val;
  node->kind = ND_NUM;
  return node;
}

// expr = equality
// equality = relational ('==' relational | '!=' relational)*
// relational = add (> add | < add | >= add | <= add)*
// add = mul ('+' mul | '-' mul)*
// mul = unary ('*' unary | '/' unary)*
// unary = ('+' | '-')? primary
// primary = num | "(" expr ")"

Node *expr();

Node *equality();

Node *relational();

Node *add();

Node *mul();

Node *unary();

Node *primary();

Node *expr() {
  Node *node = equality();
  return node;
}

Node *equality() {
  Node *node = relational();

  while (1) {
    if (consume("==")) {
      node = nd_new_node(ND_EQL, node, relational());
      puts(dbg ? "[nd]==\n" : "");
    } else if (consume("!=")) {
      node = nd_new_node(ND_NEQ, node, relational());
      puts(dbg ? "[nd]!=\n" : "");
    } else {
      return node;
    }
  }
}

Node *relational() {
  Node *node = add();

  while (1) {
    if (consume(">")) {
      node = nd_new_node(ND_GT, node, add());
      puts(dbg ? "[nd]>\n" : "");
    } else if (consume(">=")) {
      node = nd_new_node(ND_GE, node, add());
      puts(dbg ? "[nd]>=\n" : "");
    } else if (consume("<")) {
      node = nd_new_node(ND_GT, add(), node);
      puts(dbg ? "[nd]<\n" : "");
    } else if (consume("<=")) {
      node = nd_new_node(ND_GE, add(), node);
      puts(dbg ? "[nd]<=\n" : "");
    } else {
      return node;
    }
  }
}

Node *add() {
  Node *node = mul();

  while (1) {
    if (consume("+")) {
      puts(dbg ? "[nd]+\n" : "");
      node = nd_new_node(ND_ADD, node, mul());
    } else if (consume("-")) {
      puts(dbg ? "[nd]-\n" : "");
      node = nd_new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

Node *mul() {
  Node *node = unary();
  while (1) {
    if (consume("*")) {
      node = nd_new_node(ND_MUL, node, unary());
      puts(dbg ? "[nd]*\n" : "");
    } else if (consume("/")) {
      node = nd_new_node(ND_DIV, node, unary());
      puts(dbg ? "[nd]/\n" : "");
    } else {
      return node;
    }
  }
}

Node *unary() {
  if (consume("+")) {
    puts(dbg ? "[nd]+\n" : "");
    return primary();
  } else if (consume("-")) {
    puts(dbg ? "[nd]-\n" : "");
    return nd_new_node(ND_SUB, nd_new_num(0), primary());
  }
  return primary();
}

Node *primary() {
  Node *node;
  if (consume("(")) {
    puts(dbg ? "[nd](\n" : "");
    node = expr();
    if (consume(")"))
      puts(dbg ? "[nd])\n" : "");
    else
      error(token->str, "invalid syntax");
  } else {
    int num = expect_number();
    if (dbg)
      printf("[nd]%d\n", num);
    node = nd_new_num(num);
  }
  return node;
}

void gen(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  push %d\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
  case ND_ADD:
    printf("  add rax, rdi\n");
    break;
  case ND_SUB:
    printf("  sub rax, rdi\n");
    break;
  case ND_MUL:
    printf("  imul rax, rdi\n");
    break;
  case ND_DIV:
    printf("  cdq\n");
    printf("  idiv rdi\n");
    break;
  case ND_EQL:
    printf("  cmp rax, rdi\n");
    printf("  sete al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_NEQ:
    printf("  cmp rax, rdi\n");
    printf("  setne al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_GT:
    printf("  cmp rax, rdi\n");
    printf("  setg al\n");
    printf("  movzb rax, al\n");
    break;
  case ND_GE:
    printf("  cmp rax, rdi\n");
    printf("  setge al\n");
    printf("  movzb rax, al\n");
    break;
  }

  printf("  push rax\n");
}

Token *tokenize(char *p) {
  Token head;
  head.next = NULL;
  Token *cur = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
    } else if (isdigit(*p)) {
      cur = new_token(TK_NUM, cur, p, 0);
      cur->val = (int)strtol(p, &p, 10);
      if (dbg)
        printf("[tk]%d\n", cur->val);
    } else if (startswith(p, "<=") || startswith(p, ">=") ||
               startswith(p, "==") || startswith(p, "!=")) {
      cur = new_token(TK_RESERVED, cur, p, 2);
      if (dbg)
        printf("[tk]%s | 2\n", p);
      p += 2;
    } else if (strchr("+-*/()<>", *p)) {
      cur = new_token(TK_RESERVED, cur, p, 1);
      if (dbg)
        printf("[tk]%s | 1\n", p);
      p++;
    } else {
      error(p, "cannot tokenize");
    }
  }

  new_token(TK_EOF, cur, p, 0);
  return head.next;
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    error("", "not valid number of arguments");
    return 1;
  }

  if (argc == 2)
    dbg = false;
  else
    dbg = argv[2][0] == 'y';
  user_input = argv[1];
  token = tokenize(user_input);
  Node *node = expr();

  printf(".intel_syntax noprefix\n");
  printf(".globl main\n");
  printf("main:\n");

  gen(node);

  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
