#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

void error(char *message) {
  fprintf(stderr, "error: %s\n", message);
  exit(1);
}

char peek_char() {
  char c = fgetc(stdin);
  ungetc(c, stdin);
  return c;
}

char get_char() {
  return fgetc(stdin);
}

enum token_type {
  tINT,
  tNOT,
  tLNOT,
  tADD,
  tSUB,
  tMUL,
  tDIV,
  tMOD,
  tLSHIFT,
  tRSHIFT,
  tLT,
  tGT,
  tLTE,
  tGTE,
  tEQ,
  tNEQ,
  tAND,
  tOR,
  tXOR,
  tLAND,
  tLOR,
  tQUESTION,
  tCOLON,
  tLPAREN,
  tRPAREN,
  tEND
};

struct token {
  enum token_type type;
  int int_value;
};
typedef struct token Token;

Token lex() {
  Token token;

  while (1) {
    char c = peek_char();
    if (c != ' ' && c != '\n') break;
    get_char();
  }

  if (peek_char() == EOF) {
    token.type = tEND;
    return token;
  }

  char c = get_char();
  if (isdigit(c)) {
    int n = c - '0';
    while (1) {
      char d = peek_char();
      if (!isdigit(d)) break;
      get_char();
      n = n * 10 + (d - '0');
    }
    token.type = tINT;
    token.int_value = n;
  } else if (c == '~') {
    token.type = tNOT;
  } else if (c == '+') {
    token.type = tADD;
  } else if (c == '-') {
    token.type = tSUB;
  } else if (c == '*') {
    token.type = tMUL;
  } else if (c == '/') {
    token.type = tDIV;
  } else if (c == '%') {
    token.type = tMOD;
  } else if (c == '<') {
    char d = peek_char();
    if (d == '=') {
      token.type = tLTE;
      get_char();
    } else if (d == '<') {
      token.type = tLSHIFT;
      get_char();
    } else {
      token.type = tLT;
    }
  } else if (c == '>') {
    char d = peek_char();
    if (d == '=') {
      token.type = tGTE;
      get_char();
    } else if (d == '>') {
      token.type = tRSHIFT;
      get_char();
    } else {
      token.type = tGT;
    }
  } else if (c == '=') {
    if (peek_char() == '=') {
      token.type = tEQ;
      get_char();
    }
  } else if (c == '!') {
    if (peek_char() == '=') {
      token.type = tNEQ;
      get_char();
    } else {
      token.type = tLNOT;
    }
  } else if (c == '&') {
    if (peek_char() == '&') {
      token.type = tLAND;
      get_char();
    } else {
      token.type = tAND;
    }
  } else if (c == '|') {
    if (peek_char() == '|') {
      token.type = tLOR;
      get_char();
    } else {
      token.type = tOR;
    }
  } else if (c == '^') {
    token.type = tXOR;
  } else if (c == '?') {
    token.type = tQUESTION;
  } else if (c == ':') {
    token.type = tCOLON;
  } else if (c == '(') {
    token.type = tLPAREN;
  } else if (c == ')') {
    token.type = tRPAREN;
  } else {
    error("unexpected character.");
  }

  return token;
}

bool has_next_token = false;
Token next_token;

Token peek_token() {
  if (has_next_token) {
    return next_token;
  }
  has_next_token = true;
  return next_token = lex();
}

Token get_token() {
  if (has_next_token) {
    has_next_token = false;
    return next_token;
  }
  return lex();
}

enum node_type {
  CONST,
  UPLUS,
  UMINUS,
  NOT,
  LNOT,
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  LSHIFT,
  RSHIFT,
  LT,
  GT,
  LTE,
  GTE,
  EQ,
  NEQ,
  AND,
  OR,
  XOR,
  LAND,
  LOR,
  CONDITION
};

struct node {
  enum node_type type;
  int int_value;
  struct node *condition;
  struct node *left;
  struct node *right;
};
typedef struct node Node;

Node *node_new() {
  Node *node = (Node *) malloc(sizeof(Node));
  return node;
}

Node *expression();

Node *primary_expression() {
  Token token = get_token();
  Node *node;

  if (token.type == tINT) {
    node = node_new();
    node->type = CONST;
    node->int_value = token.int_value;
  } else if (token.type == tLPAREN) {
    node = expression();
    if (get_token().type != tRPAREN) {
      error("tRPAREN is expected.");
    }
  } else {
    error("unexpected primary expression.");
  }

  return node;
}

Node *unary_expression() {
  Token token = peek_token();
  Node *node;

  if (token.type == tADD) {
    get_token();
    node = node_new();
    node->type = UPLUS;
    node->left = unary_expression();
  } else if (token.type == tSUB) {
    get_token();
    node = node_new();
    node->type = UMINUS;
    node->left = unary_expression();
  } else if (token.type == tNOT) {
    get_token();
    node = node_new();
    node->type = NOT;
    node->left = unary_expression();
  } else if (token.type == tLNOT) {
    get_token();
    node = node_new();
    node->type = LNOT;
    node->left = unary_expression();
  } else {
    node = primary_expression();
  }

  return node;
}

Node *multiplicative_expression() {
  Node *node = unary_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tMUL) type = MUL;
    else if (op.type == tDIV) type = DIV;
    else if (op.type == tMOD) type = MOD;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = unary_expression();

    node = parent;
  }

  return node;
}

Node *additive_expression() {
  Node *node = multiplicative_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tADD) type = ADD;
    else if (op.type == tSUB) type = SUB;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = multiplicative_expression();

    node = parent;
  }

  return node;
}

Node *shift_expression() {
  Node *node = additive_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tLSHIFT) type = LSHIFT;
    else if (op.type == tRSHIFT) type = RSHIFT;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = additive_expression();

    node = parent;
  }

  return node;
}

Node *relational_expression() {
  Node *node = shift_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tLT) type = LT;
    else if (op.type == tGT) type = GT;
    else if (op.type == tLTE) type = LTE;
    else if (op.type == tGTE) type = GTE;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = shift_expression();

    node = parent;
  }

  return node;
}

Node *equality_expression() {
  Node *node = relational_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tEQ) type = EQ;
    else if (op.type == tNEQ) type = NEQ;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = relational_expression();

    node = parent;
  }

  return node;
}

Node *and_expression() {
  Node *node = equality_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tAND) type = AND;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = equality_expression();

    node = parent;
  }

  return node;
}

Node *or_expression() {
  Node *node = and_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tOR) type = OR;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = and_expression();

    node = parent;
  }

  return node;
}

Node *xor_expression() {
  Node *node = or_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tXOR) type = XOR;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = or_expression();

    node = parent;
  }

  return node;
}

Node *logical_and_expression() {
  Node *node = xor_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tLAND) type = LAND;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = xor_expression();

    node = parent;
  }

  return node;
}

Node *logical_or_expression() {
  Node *node = logical_and_expression();

  while (1) {
    Token op = peek_token();
    enum node_type type;
    if (op.type == tLOR) type = LOR;
    else break;
    get_token();

    Node *parent = node_new();
    parent->type = type;
    parent->left = node;
    parent->right = logical_and_expression();

    node = parent;
  }

  return node;
}

Node *conditional_expression() {
  Node *node = logical_or_expression();

  if (peek_token().type == tQUESTION) {
    get_token();

    Node *parent = node_new();
    parent->type = CONDITION;
    parent->condition = node;
    parent->left = expression();
    if (get_token().type != tCOLON) {
      error("tCOLON is expected.");
    }
    parent->right = conditional_expression();

    node = parent;
  }

  return node;
}

Node *expression() {
  return conditional_expression();
}

Node *parse() {
  Node *node = expression();

  if (peek_token().type != tEND) {
    error("invalid expression.");
  }

  return node;
}

void generate_immediate(int value) {
  printf("  sub $4, %%rsp\n");
  printf("  movl $%d, 0(%%rsp)\n", value);
}

void generate_push(char *reg) {
  printf("  sub $4, %%rsp\n");
  printf("  movl %%%s, 0(%%rsp)\n", reg);
}

void generate_pop(char *reg) {
  printf("  movl 0(%%rsp), %%%s\n", reg);
  printf("  add $4, %%rsp\n");
}

int label_no = 0;

void generate_expression(Node *node) {
  if (node->type == CONST) {
    generate_immediate(node->int_value);
  } else if (node->type == LAND) {
    int label_false = label_no++;
    int label_end = label_no++;
    generate_expression(node->left);
    generate_pop("eax");
    printf("  cmpl $0, %%eax\n");
    printf("  je .L%d\n", label_false);
    generate_expression(node->right);
    generate_pop("eax");
    printf("  cmpl $0, %%eax\n");
    printf("  je .L%d\n", label_false);
    printf("  movl $1, %%eax\n");
    printf("  jmp .L%d\n", label_end);
    printf(".L%d:\n", label_false);
    printf("  movl $0, %%eax\n");
    printf(".L%d:\n", label_end);
    generate_push("eax");
  } else if (node->type == LOR) {
    int label_true = label_no++;
    int label_end = label_no++;
    generate_expression(node->left);
    generate_pop("eax");
    printf("  cmpl $0, %%eax\n");
    printf("  jne .L%d\n", label_true);
    generate_expression(node->right);
    generate_pop("eax");
    printf("  cmpl $0, %%eax\n");
    printf("  jne .L%d\n", label_true);
    printf("  movl $0, %%eax\n");
    printf("  jmp .L%d\n", label_end);
    printf(".L%d:\n", label_true);
    printf("  movl $1, %%eax\n");
    printf(".L%d:\n", label_end);
    generate_push("eax");
  } else if (node->type == CONDITION) {
    int label_false = label_no++;
    int label_end = label_no++;
    generate_expression(node->condition);
    generate_pop("eax");
    printf("  cmpl $0, %%eax\n");
    printf("  je .L%d\n", label_false);
    generate_expression(node->left);
    printf("  jmp .L%d\n", label_end);
    printf(".L%d:\n", label_false);
    generate_expression(node->right);
    printf(".L%d:\n", label_end);
  } else if (node->type == UPLUS) {
    generate_expression(node->left);
  } else if (node->type == UMINUS) {
    generate_expression(node->left);
    generate_pop("eax");
    printf("  negl %%eax\n");
    generate_push("eax");
  } else if (node->type == NOT) {
    generate_expression(node->left);
    generate_pop("eax");
    printf("  notl %%eax\n");
    generate_push("eax");
  } else if (node->type == LNOT) {
    generate_expression(node->left);
    generate_pop("eax");
    printf("  cmpl $0, %%eax\n");
    printf("  sete %%al\n");
    printf("  movzbl %%al, %%eax\n");
    generate_push("eax");
  } else {
    generate_expression(node->left);
    generate_expression(node->right);
    generate_pop("ecx");
    generate_pop("eax");
    if (node->type == ADD) {
      printf("  addl %%ecx, %%eax\n");
      generate_push("eax");
    } else if (node->type == SUB) {
      printf("  subl %%ecx, %%eax\n");
      generate_push("eax");
    } else if (node->type == MUL) {
      printf("  imull %%ecx\n");
      generate_push("eax");
    } else if (node->type == DIV) {
      printf("  movl $0, %%edx\n");
      printf("  idivl %%ecx\n");
      generate_push("eax");
    } else if (node->type == MOD) {
      printf("  movl $0, %%edx\n");
      printf("  idivl %%ecx\n");
      generate_push("edx");
    } else if (node->type == LSHIFT) {
      printf("  sall %%cl, %%eax\n");
      generate_push("eax");
    } else if (node->type == RSHIFT) {
      printf("  sarl %%cl, %%eax\n");
      generate_push("eax");
    } else if (node->type == LT) {
      printf("  cmpl %%ecx, %%eax\n");
      printf("  setl %%al\n");
      printf("  movzbl %%al, %%eax\n");
      generate_push("eax");
    } else if (node->type == GT) {
      printf("  cmpl %%ecx, %%eax\n");
      printf("  setg %%al\n");
      printf("  movzbl %%al, %%eax\n");
      generate_push("eax");
    } else if (node->type == LTE) {
      printf("  cmpl %%ecx, %%eax\n");
      printf("  setle %%al\n");
      printf("  movzbl %%al, %%eax\n");
      generate_push("eax");
    } else if (node->type == GTE) {
      printf("  cmpl %%ecx, %%eax\n");
      printf("  setge %%al\n");
      printf("  movzbl %%al, %%eax\n");
      generate_push("eax");
    } else if (node->type == EQ) {
      printf("  cmpl %%ecx, %%eax\n");
      printf("  sete %%al\n");
      printf("  movzbl %%al, %%eax\n");
      generate_push("eax");
    } else if (node->type == NEQ) {
      printf("  cmpl %%ecx, %%eax\n");
      printf("  setne %%al\n");
      printf("  movzbl %%al, %%eax\n");
      generate_push("eax");
    } else if (node->type == AND) {
      printf("  andl %%ecx, %%eax\n");
      generate_push("eax");
    } else if (node->type == OR) {
      printf("  orl %%ecx, %%eax\n");
      generate_push("eax");
    } else if (node->type == XOR) {
      printf("  xorl %%ecx, %%eax\n");
      generate_push("eax");
    }
  }
}

void generate(Node *node) {
  printf("  .global main\n");
  printf("main:\n");
  printf("  push %%rbp\n");
  printf("  mov %%rsp, %%rbp\n");

  generate_expression(node);

  generate_pop("eax");

  printf("  pop %%rbp\n");
  printf("  ret\n");
}

int main(void) {
  Node *node = parse();
  generate(node);

  return 0;
}
