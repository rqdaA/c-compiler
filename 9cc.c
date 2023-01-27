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
char* user_input;

struct Token {
	TokenKind kind; // トークンの型
	Token *next;    // 次の入力トークン
	int val;        // kindがTK_NUMの場合、その数値
	char *str;      // トークン文字列
};

Token *token;
bool dbg;

void error(char* loc, const char *format, ...) {
	va_list ap;
	va_start(ap,format);
	vfprintf(stderr,format,ap);
	exit(1);
}

bool consume(char op) {
	bool isReserved = token->kind == TK_RESERVED;
	bool isOp = token->str[0] == op;
	if (isReserved && isOp)
		token = token->next;
	return isReserved && isOp;
}

int expect_number() {
	if (token->kind != TK_NUM) {
		printf("%s\n",token->str);
		error(token->str,"%s is not a number\n");
	}
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

Token *new_token(TokenKind kind,Token *cur,char *str) {
	Token *tok = calloc(1,sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return tok;
}

Node *nd_new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1,sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *nd_new_num(int val) {
	Node *node = calloc(1,sizeof(Node));
	node->val = val;
	node->kind = ND_NUM;
	return node;
}

// expr = mul ('+' mul | '-' mul)*
// mul = unary ('*' unary | '/' unary)*
// unary = ('+' | '-')? primary
// primary = num | "(" expr ")"

Node *expr();
Node *mul();
Node *unary();
Node *primary();

Node* expr() {
	Node *node = mul();

	while (1) {
		if (consume('+')) {
			node = nd_new_node(ND_ADD, node,mul());
			if (dbg)
				printf("[nd]+ %d\n", node->rhs->val);
		}
		else if (consume('-')) {
			node = nd_new_node(ND_SUB,node,mul());
			if (dbg)
				printf("[nd]- %d\n", node->rhs->val);
		}
		else
			return node;
	}
}

Node* mul() {
	Node *node = unary();
	while (1) {
		if (consume('*')) {
			node = nd_new_node(ND_MUL,node,unary());
			if (dbg)
				printf("[nd]* %d\n", node->rhs->val);
		}
		else if (consume('/')) {
			node = nd_new_node(ND_DIV,node,unary());
			if (dbg)
				printf("[nd]/ %d\n", node->rhs->val);
		}
		else
			return node;
	}
}

Node* unary() {
	if (consume('+')) {
		printf(dbg ? "[nd]+\n":"");
		return primary();
	} else if (consume('-')) {
		printf(dbg ? "[nd]-\n":"");
		return nd_new_node(ND_SUB,nd_new_num(0),primary());
	}
	return primary();
}

Node* primary() {
	Node *node;
	if (consume('(')) {
		printf(dbg ? "[nd](\n":"");
		node = expr();
		if (consume(')'))
			printf(dbg ? "[nd])\n" : "");
		else
			error(token->str,"invalid syntax");
	}
	else {
		node = nd_new_num(expect_number());
	}
	return node;
}

void gen(Node* node) {
	if (node->kind == ND_NUM) {
		printf("  push %d\n",node->val);
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
	}

	printf("  push rax\n");
}

Token* tokenize(char *p) {
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while (*p) {
		if (isspace(*p)) {
			p++;
		}
		else if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
			cur = new_token(TK_RESERVED,cur,p);
			if (dbg)
				printf("[tk] %c\n",*p);
			p++;
		}
		else if (isdigit(*p)) {
			cur = new_token(TK_NUM,cur,p);
			cur->val = strtol(p,&p,10);
			if (dbg)
				printf("[tk] %d\n",cur->val);
		}
		else {
			error(p, "cannot tokenize");
		}
	}

	new_token(TK_EOF,cur,p);
	return head.next;
}

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		error("who","not valid number of arguments");
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
