#include "string.h"
#include "vector.h"
#include "map.h"

#define EOF (-1)

#define NULL ((void *) 0)

#define bool _Bool
#define false 0
#define true 1

#define noreturn _Noreturn

#define va_start __builtin_va_start
#define va_end __builtin_va_end
typedef struct {
  int gp_offset;
  int fp_offset;
  void *overflow_arg_area;
  void *reg_save_area;
} va_list[1];

typedef int size_t;
typedef struct _IO_FILE FILE;

extern struct _IO_FILE *stdin;
extern struct _IO_FILE *stdout;
extern struct _IO_FILE *stderr;

int fgetc(FILE *stream);
int ungetc(int c, FILE *stream);
int printf (char *format, ...);
int fprintf(FILE *stream, char *format, ...);
int vfprintf(FILE *s, char *format, va_list arg);
FILE *fopen(char *filename, char *modes);

void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void exit(int status);

int strcmp(char *s1, char *s2);

int isdigit(int c);
int isalpha(int c);
int isalnum(int c);
int isprint(int c);
int isascii(int c);

typedef struct source_char SourceChar;

typedef struct token Token;
typedef struct node Node;

typedef enum type_type TypeType;
typedef struct type Type;
typedef struct struct_member StructMember;

typedef enum symbol_type SymbolType;
typedef struct symbol Symbol;

struct source_char {
  char *filename, *char_ptr, *line_ptr;
  int lineno, column;
};

// token type
// A one-character token is represented by it's ascii code.
enum {
  // white spaces (removed before syntax analysis)
  TK_SPACE = 128,
  TK_NEWLINE,

  // keywords for expressions
  TK_SIZEOF,
  TK_ALIGNOF,

  // keywords for declarations
  TK_VOID,
  TK_CHAR,
  TK_SHORT,
  TK_INT,
  TK_UNSIGNED,
  TK_BOOL,
  TK_STRUCT,
  TK_ENUM,
  TK_TYPEDEF,
  TK_EXTERN,
  TK_NORETURN,

  // keywords for statements
  TK_IF,
  TK_ELSE,
  TK_WHILE,
  TK_DO,
  TK_FOR,
  TK_CONTINUE,
  TK_BREAK,
  TK_RETURN,

  // identifiers, constants, string literals
  TK_IDENTIFIER,
  TK_INTEGER_CONST,
  TK_STRING_LITERAL,

  // punctuators
  TK_ARROW,      // ->
  TK_INC,        // ++
  TK_DEC,        // --
  TK_LSHIFT,     // <<
  TK_RSHIFT,     // >>
  TK_LTE,        // <=
  TK_GTE,        // >=
  TK_EQ,         // ==
  TK_NEQ,        // !=
  TK_AND,        // &&
  TK_OR,         // ||
  TK_MUL_ASSIGN, // *=
  TK_DIV_ASSIGN, // /=
  TK_MOD_ASSIGN, // %=
  TK_ADD_ASSIGN, // +=
  TK_SUB_ASSIGN, // -=
  TK_ELLIPSIS,   // ...

  // EOF (the end of the input source file)
  TK_EOF
};

struct token {
  int tk_type;
  int int_value;
  String *string_literal;
  char *identifier;
  SourceChar **schar, **schar_end;
};

enum {
  // expression
  ND_IDENTIFIER,
  ND_INTEGER,
  ND_STRING,
  ND_CALL,
  ND_DOT,
  ND_ADDRESS,
  ND_INDIRECT,
  ND_UPLUS,
  ND_UMINUS,
  ND_NOT,
  ND_LNOT,
  ND_CAST,
  ND_MUL,
  ND_DIV,
  ND_MOD,
  ND_ADD,
  ND_SUB,
  ND_LSHIFT,
  ND_RSHIFT,
  ND_LT,
  ND_LTE,
  ND_EQ,
  ND_NEQ,
  ND_AND,
  ND_XOR,
  ND_OR,
  ND_LAND,
  ND_LOR,
  ND_CONDITION,
  ND_ASSIGN,
  ND_COMMA,

  // declaration
  ND_DECL,

  // statement
  ND_COMP,
  ND_EXPR,
  ND_IF,
  ND_WHILE,
  ND_DO,
  ND_FOR,
  ND_CONTINUE,
  ND_BREAK,
  ND_RETURN,

  // function definition
  ND_FUNC
};

typedef struct node Node;
typedef struct expr Expr;
typedef struct decl Decl;
typedef struct init Init;
typedef struct stmt Stmt;
typedef struct func Func;
typedef struct trans_unit TransUnit;

// AST node
// This struct is used for checking the node type.
// After checking, the pointer is casted to the pointer of each node type.
// There are 4 kinds of node: Expr, Decl, Stmt, Func.
struct node {
  int nd_type;
  Token *token;
};

// AST node for expression
struct expr {
  int nd_type;
  Token *token;

  // type of the expression
  Type *type;

  // child expression node
  Expr *expr;      // for unary expr, postfix expr, cast expr
  Expr *lhs, *rhs; // for binary expr, conditional expr
  Expr *cond;      // for conditional expr

  // identifier
  char *identifier;
  Symbol *symbol;

  // integer constant
  int int_value;

  // string literal
  String *string_literal;
  int string_label;

  // call
  Vector *args; // vector of Expr*

  // member access
  char *member;
  int offset;
};

// AST node for declaration
struct decl {
  int nd_type;
  Token *token;

  Vector *declarations; // vector of Symbol*
};

// initializer
struct init {
  Type *type;
  Expr *expr;
  Vector *list; // vector of Init*
};

// AST node for statement
struct stmt {
  int nd_type;
  Token *token;

  // compound statement (block)
  Vector *block_items; // vector of Node* (Decl* or Stmt*)

  // expression statement
  Expr *expr; // optional

  // if-else statement
  Expr *if_cond;
  Stmt *then_body;
  Stmt *else_body; // optional

  // while statement
  Expr *while_cond;
  Stmt *while_body;

  // do-while statement
  Expr *do_cond;
  Stmt *do_body;

  // for statement
  Node *for_init;  // optional, Decl* or Expr*
  Expr *for_cond;  // optional
  Expr *for_after; // optional
  Stmt *for_body;

  // return statement
  Expr *ret_expr; // optional
};

// AST node for function definition
struct func {
  int nd_type;
  Token *token;

  Symbol *symbol;
  Stmt *body;
  int stack_size; // stack size for local variables
};

// translation unit
struct trans_unit {
  Vector *string_literals; // vector of String*
  Vector *external_decls;  // vector of Node* (Decl* or Func*)
};

enum type_type {
  VOID,
  BOOL,
  CHAR,
  UCHAR,
  SHORT,
  USHORT,
  INT,
  UINT,
  POINTER,
  ARRAY,
  STRUCT,
  FUNCTION
};

struct type {
  TypeType ty_type;
  int size, align;
  Type *pointer_to;
  Type *array_of;
  int array_size;
  Type *function_returning;
  Vector *params;
  bool ellipsis;
  Map *members;
  int original_size;
  bool array_pointer;
  bool definition;
  bool external;
  bool incomplete;
};

struct struct_member {
  Type *type;
  int offset;
};

enum symbol_type {
  GLOBAL,
  LOCAL,
  TYPENAME,
  ENUM_CONST
};

struct symbol {
  SymbolType sy_type;
  Token *token;
  char *identifier;
  Type *type;
  Init *init;
  int enum_value;
  int offset;
  bool defined;
};

extern noreturn void error(Token *token, char *format, ...);

extern Vector *scan(char *filename);

extern char *token_name(int tk_type);

extern Vector *tokenize(Vector *input_src);

extern Vector *preprocess(Vector *pp_tokens);

extern Type *type_new();
extern Type *type_void();
extern Type *type_bool();
extern Type *type_char();
extern Type *type_uchar();
extern Type *type_short();
extern Type *type_ushort();
extern Type *type_int();
extern Type *type_uint();
extern Type *type_pointer_to(Type *type);
extern Type *type_array_of(Type *type, int array_size);
extern Type *type_incomplete_array_of(Type *type);
extern Type *type_function_returning(Type *returning, Vector *params, bool ellipsis);
extern Type *type_struct(Vector *symbols);
extern Type *type_convert(Type *type);
extern void type_copy(Type *dest, Type *src);
extern bool type_integer(Type *type);
extern bool type_pointer(Type *type);
extern bool type_scalar(Type *type);
extern bool type_same(Type *type1, Type *type2);

extern Expr *expr_identifier(char *identifier, Symbol *symbol, Token *token);
extern Expr *expr_integer(int int_value, Token *token);
extern Expr *expr_string(String *string_literal, int string_label, Token *token);
extern Expr *expr_subscription(Expr *expr, Expr *index, Token *token);
extern Expr *expr_call(Expr *expr, Vector *args, Token *token);
extern Expr *expr_dot(Expr *expr, char *member, Token *token);
extern Expr *expr_arrow(Expr *expr, char *member, Token *token);
extern Expr *expr_post_inc(Expr *expr, Token *token);
extern Expr *expr_post_dec(Expr *expr, Token *token);
extern Expr *expr_pre_inc(Expr *expr, Token *token);
extern Expr *expr_pre_dec(Expr *expr, Token *token);
extern Expr *expr_address(Expr *expr, Token *token);
extern Expr *expr_indirect(Expr *expr, Token *token);
extern Expr *expr_uplus(Expr *expr, Token *token);
extern Expr *expr_uminus(Expr *expr, Token *token);
extern Expr *expr_not(Expr *expr, Token *token);
extern Expr *expr_lnot(Expr *expr, Token *token);
extern Expr *expr_cast(Type *type, Expr *expr, Token *token);
extern Expr *expr_mul(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_div(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_mod(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_add(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_sub(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_lshift(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_rshift(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_lt(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_gt(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_lte(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_gte(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_eq(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_neq(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_and(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_xor(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_or(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_land(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_lor(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_conditional(Expr *cond, Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_assign(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_mul_assign(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_div_assign(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_mod_assign(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_add_assign(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_sub_assign(Expr *lhs, Expr *rhs, Token *token);
extern Expr *expr_comma(Expr *lhs, Expr *rhs, Token *token);
extern Decl *decl_new(Vector *declarations);
extern Init *init_expr(Expr *expr, Type *type);
extern Init *init_list(Vector *list, Type *type);
extern Stmt *stmt_comp(Vector *block_items, Token *token);
extern Stmt *stmt_expr(Expr *expr, Token *token);
extern Stmt *stmt_if(Expr *if_cond, Stmt *then_body, Stmt *else_body, Token *token);
extern Stmt *stmt_while(Expr *while_cond, Stmt *while_body, Token *token);
extern Stmt *stmt_do(Expr *do_cond, Stmt *do_body, Token *token);
extern Stmt *stmt_for(Node *for_init, Expr *for_cond, Expr *for_after, Stmt *for_body, Token *token);
extern Stmt *stmt_continue(int continue_level, Token *token);
extern Stmt *stmt_break(int break_level, Token *token);
extern Stmt *stmt_return(Expr *ret_expr, Token *token);
extern Func *func_new(Symbol *symbol, Stmt *body, int stack_size, Token *token);
extern TransUnit *trans_unit_new(Vector *string_literals, Vector *external_decls);

extern Symbol *symbol_new();
extern void symbol_put(char *identifier, Symbol *symbol);
extern TransUnit *parse(Vector *tokens);

extern void gen(TransUnit *node);
