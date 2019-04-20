#include "as.h"

static Label *label_new(char *ident, Token *token) {
  Label *label = (Label *) calloc(1, sizeof(Label));
  label->ident = ident;
  label->token = token;
  return label;
}

static Dir *dir_new(DirType type, Token *token) {
  Dir *dir = (Dir *) calloc(1, sizeof(Dir));
  dir->type = type;
  dir->token = token;
  return dir;
}

static Dir *dir_text(Token *token) {
  return dir_new(DIR_TEXT, token);
}

static Dir *dir_data(Token *token) {
  return dir_new(DIR_DATA, token);
}

static Dir *dir_section(char *ident, Token *token) {
  Dir *dir = dir_new(DIR_SECTION, token);
  dir->ident = ident;
  return dir;
}

static Dir *dir_global(char *ident, Token *token) {
  Dir *dir = dir_new(DIR_GLOBAL, token);
  dir->ident = ident;
  return dir;
}

static Dir *dir_zero(int num, Token *token) {
  Dir *dir = dir_new(DIR_ZERO, token);
  dir->num = num;
  return dir;
}

static Dir *dir_long(int num, Token *token) {
  Dir *dir = dir_new(DIR_LONG, token);
  dir->num = num;
  return dir;
}

static Dir *dir_quad(char *ident, Token *token) {
  Dir *dir = dir_new(DIR_QUAD, token);
  dir->ident = ident;
  return dir;
}

static Dir *dir_ascii(char *string, int length, Token *token) {
  Dir *dir = dir_new(DIR_ASCII, token);
  dir->string = string;
  dir->length = length;
  return dir;
}

static Op *op_new(OpType type, Token *token) {
  Op *op = (Op *) calloc(1, sizeof(Op));
  op->type = type;
  op->token = token;
  return op;
}

static Op *op_reg(RegType regtype, Reg regcode, Token *token) {
  Op *op = op_new(OP_REG, token);
  op->regtype = regtype;
  op->regcode = regcode;
  return op;
}

static Op *op_mem_base(Reg base, Reg disp, Token *token) {
  Op *op = op_new(OP_MEM, token);
  op->base = base;
  op->disp = disp;
  return op;
}

static Op *op_mem_sib(Scale scale, Reg index, Reg base, Reg disp, Token *token) {
  Op *op = op_new(OP_MEM, token);
  op->sib = true;
  op->scale = scale;
  op->index = index;
  op->base = base;
  op->disp = disp;
  return op;
}

static Op *op_rip_rel(char *ident, Token *token) {
  Op *op = op_new(OP_MEM, token);
  op->rip = true;
  op->ident = ident;
  return op;
}

static Op *op_sym(char *ident, Token *token) {
  Op *op = op_new(OP_SYM, token);
  op->ident = ident;
  return op;
}

static Op *op_imm(int imm, Token *token) {
  Op *op = op_new(OP_IMM, token);
  op->imm = imm;
  return op;
}

static Inst *inst_new(InstType type, InstSuffix suffix, Token *token) {
  Inst *inst = (Inst *) calloc(1, sizeof(Inst));
  inst->type = type;
  inst->suffix = suffix;
  inst->token = token;
  return inst;
}

static Inst *inst_op0(InstType type, InstSuffix suffix, Token *token) {
  return inst_new(type, suffix, token);
}

static Inst *inst_op1(InstType type, InstSuffix suffix, Op *op, Token *token) {
  Inst *inst = inst_new(type, suffix, token);
  inst->op = op;
  return inst;
}

static Inst *inst_op2(InstType type, InstSuffix suffix, Op *src, Op *dest, Token *token) {
  Inst *inst = inst_new(type, suffix, token);
  inst->src = src;
  inst->dest = dest;
  return inst;
}

static Stmt *stmt_new(StmtType type) {
  Stmt *stmt = (Stmt *) calloc(1, sizeof(Stmt));
  stmt->type = type;
  return stmt;
}

static Stmt *stmt_label(Label *label) {
  Stmt *stmt = stmt_new(STMT_LABEL);
  stmt->label = label;
  return stmt;
}

static Stmt *stmt_dir(Dir *dir) {
  Stmt *stmt = stmt_new(STMT_DIR);
  stmt->dir = dir;
  return stmt;
}

static Stmt *stmt_inst(Inst *inst) {
  Stmt *stmt = stmt_new(STMT_INST);
  stmt->inst = inst;
  return stmt;
}

#define EXPECT(token, expect_type, ...) \
  do { \
    Token *t = (token); \
    if (t->type != (expect_type)) { \
      ERROR(t, __VA_ARGS__); \
    } \
  } while (0)

static Vector *parse_ops(Token **token) {
  Vector *ops = vector_new();
  if (!(*token)) return ops;

  do {
    Token *op_head = *token;

    switch ((*token)->type) {
      case TOK_REG: {
        RegType regtype = (*token)->regtype;
        Reg regcode = (*token)->regcode;
        token++;
        vector_push(ops, op_reg(regtype, regcode, op_head));
      }
      break;

      case TOK_LPAREN:
      case TOK_NUM: {
        Op *op;
        int disp = (*token)->type == TOK_NUM ? (*token++)->num : 0;
        EXPECT(*token++, TOK_LPAREN, "'(' is expected.");
        EXPECT(*token, TOK_REG, "register is expected.");
        if ((*token)->regtype != REG64) {
          ERROR(*token, "64-bit register is expected.");
        }
        Reg base = (*token++)->regcode;
        if ((*token)->type == TOK_COMMA) {
          token++;
          EXPECT(*token, TOK_REG, "register is expected.");
          if ((*token)->regtype != REG64) {
            ERROR(*token, "64-bit register is expected.");
          }
          if ((*token)->regcode == SP) {
            ERROR(*token, "cannot use rsp as index.");
          }
          Reg index = (*token++)->regcode;
          if ((*token)->type == TOK_COMMA) {
            token++;
            EXPECT(*token, TOK_NUM, "scale is expected.");
            Token *scale = *token++;
            if (scale->num == 1) {
              op = op_mem_sib(SCALE1, index, base, disp, op_head);
            } else if (scale->num == 2) {
              op = op_mem_sib(SCALE2, index, base, disp, op_head);
            } else if (scale->num == 4) {
              op = op_mem_sib(SCALE4, index, base, disp, op_head);
            } else if (scale->num == 8) {
              op = op_mem_sib(SCALE8, index, base, disp, op_head);
            } else {
              ERROR(scale, "one of 1, 2, 4, 8 is expected.");
            }
          } else {
            op = op_mem_sib(SCALE1, index, base, disp, op_head);
          }
        } else {
          op = op_mem_base(base, disp, op_head);
        }
        EXPECT(*token++, TOK_RPAREN, "')' is expected.");
        vector_push(ops, op);
      }
      break;

      case TOK_IDENT: {
        char *sym = (*token++)->ident;
        if (*token && (*token)->type == TOK_LPAREN) {
          token++;
          EXPECT(*token++, TOK_RIP, "%rip is expected");
          EXPECT(*token++, TOK_RPAREN, "')' is expected.");
          vector_push(ops, op_rip_rel(sym, op_head));
        } else {
          vector_push(ops, op_sym(sym, op_head));
        }
      }
      break;

      case TOK_IMM: {
        int imm = (*token++)->imm;
        vector_push(ops, op_imm(imm, op_head));
      }
      break;

      default: {
        ERROR(*token, "invalid operand.");
      }
    }
  } while (*token && (*token++)->type == TOK_COMMA);

  return ops;
}

static Inst *parse_inst(Token **token) {
  Token *inst = *token++;
  Vector *ops = parse_ops(token);

  if (strcmp(inst->ident, "pushq") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG || op->regtype != REG64) {
      ERROR(op->token, "only 64-bits register is supported.");
    }
    return inst_op1(INST_PUSH, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "popq") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG || op->regtype != REG64) {
      ERROR(op->token, "only 64-bits register is supported.");
    }
    return inst_op1(INST_POP, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "movq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG64) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOV, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "movl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG32) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOV, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "movw") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG16) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG16) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOV, INST_WORD, src, dest, inst);
  }

  if (strcmp(inst->ident, "movb") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG8) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG8) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOV, INST_BYTE, src, dest, inst);
  }

  if (strcmp(inst->ident, "movzbq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG8) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVZB, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "movzbl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG8) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVZB, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "movzbw") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG8) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG16) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVZB, INST_WORD, src, dest, inst);
  }

  if (strcmp(inst->ident, "movzwq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG16) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVZW, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "movzwl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG16) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVZW, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "movsbq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG8) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVSB, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "movsbl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG8) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVSB, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "movsbw") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG8) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG16) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVSB, INST_WORD, src, dest, inst);
  }

  if (strcmp(inst->ident, "movswq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG16) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVSW, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "movswl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG16) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVSW, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "movslq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG && src->type != OP_MEM) {
      ERROR(src->token, "register or memory operand is expected.");
    }
    if (src->type == OP_REG && src->regtype != REG32) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_MOVSL, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "leaq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_MEM) {
      ERROR(src->token, "first operand should be memory operand.");
    }
    if (dest->type != OP_REG) {
      ERROR(dest->token, "second operand should be register operand.");
    }
    if (dest->regtype != REG64) {
      ERROR(dest->token, "only 64-bit register is supported.");
    }
    return inst_op2(INST_LEA, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "negq") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG64) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_NEG, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "negl") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG32) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_NEG, INST_LONG, op, inst);
  }

  if (strcmp(inst->ident, "notq") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG64) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_NOT, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "notl") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG32) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_NOT, INST_LONG, op, inst);
  }

  if (strcmp(inst->ident, "addq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG64) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_ADD, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "addl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG32) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_ADD, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "subq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG64) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_SUB, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "subl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG32) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_SUB, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "mulq") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG64) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_MUL, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "mull") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG32) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_MUL, INST_LONG, op, inst);
  }

  if (strcmp(inst->ident, "imulq") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG64) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_IMUL, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "imull") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG32) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_IMUL, INST_LONG, op, inst);
  }

  if (strcmp(inst->ident, "divq") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG64) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_DIV, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "divl") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG32) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_DIV, INST_LONG, op, inst);
  }

  if (strcmp(inst->ident, "idivq") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG64) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_IDIV, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "idivl") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG32) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_IDIV, INST_LONG, op, inst);
  }

  if (strcmp(inst->ident, "andq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG64) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_AND, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "andl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG32) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_AND, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "xorq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG64) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_XOR, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "xorl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG32) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_XOR, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "orq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG64) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_OR, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "orl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG32) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_OR, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "salq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operand.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG || src->regtype != REG8 || src->regcode != CX) {
      ERROR(src->token, "only %%cl is supported.");
    }
    if (dest->type != OP_REG && dest->type != OP_MEM) {
      ERROR(dest->token, "register or memory operand is expected.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_SAL, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "sall") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operand.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG || src->regtype != REG8 || src->regcode != CX) {
      ERROR(src->token, "only %%cl is supported.");
    }
    if (dest->type != OP_REG && dest->type != OP_MEM) {
      ERROR(dest->token, "register or memory operand is expected.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_SAL, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "sarq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operand.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG || src->regtype != REG8 || src->regcode != CX) {
      ERROR(src->token, "only %%cl is supported.");
    }
    if (dest->type != OP_REG && dest->type != OP_MEM) {
      ERROR(dest->token, "register or memory operand is expected.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_SAR, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "sarl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operand.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (src->type != OP_REG || src->regtype != REG8 || src->regcode != CX) {
      ERROR(src->token, "only %%cl is supported.");
    }
    if (dest->type != OP_REG && dest->type != OP_MEM) {
      ERROR(dest->token, "register or memory operand is expected.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_SAR, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "cmpq") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG64) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG64) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_CMP, INST_QUAD, src, dest, inst);
  }

  if (strcmp(inst->ident, "cmpl") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG32) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG32) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_CMP, INST_LONG, src, dest, inst);
  }

  if (strcmp(inst->ident, "cmpw") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG16) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG16) {
      ERROR(dest->token, "operand type mismatched.");
    }

    return inst_op2(INST_CMP, INST_WORD, src, dest, inst);
  }

  if (strcmp(inst->ident, "cmpb") == 0) {
    if (ops->length != 2) {
      ERROR(inst, "'%s' expects 2 operands.", inst->ident);
    }
    Op *src = ops->buffer[0], *dest = ops->buffer[1];
    if (dest->type == OP_IMM) {
      ERROR(dest->token, "destination cannot be an immediate.");
    }
    if (src->type == OP_MEM && dest->type == OP_MEM) {
      ERROR(inst, "both of source and destination cannot be memory operands.");
    }
    if (src->type == OP_REG && src->regtype != REG8) {
      ERROR(src->token, "operand type mismatched.");
    }
    if (dest->type == OP_REG && dest->regtype != REG8) {
      ERROR(dest->token, "operand type mismatched.");
    }
    return inst_op2(INST_CMP, INST_BYTE, src, dest, inst);
  }

  if (strcmp(inst->ident, "sete") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG8) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_SETE, INST_BYTE, op, inst);
  }

  if (strcmp(inst->ident, "setne") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG8) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_SETNE, INST_BYTE, op, inst);
  }

  if (strcmp(inst->ident, "setb") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG8) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_SETB, INST_BYTE, op, inst);
  }

  if (strcmp(inst->ident, "setl") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG8) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_SETL, INST_BYTE, op, inst);
  }

  if (strcmp(inst->ident, "setg") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG8) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_SETG, INST_BYTE, op, inst);
  }

  if (strcmp(inst->ident, "setbe") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG8) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_SETBE, INST_BYTE, op, inst);
  }

  if (strcmp(inst->ident, "setle") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG8) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_SETLE, INST_BYTE, op, inst);
  }

  if (strcmp(inst->ident, "setge") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_REG && op->type != OP_MEM) {
      ERROR(inst, "register or memory operand is expected.");
    }
    if (op->type == OP_REG && op->regtype != REG8) {
      ERROR(inst, "operand type mismatched.");
    }
    return inst_op1(INST_SETGE, INST_BYTE, op, inst);
  }

  if (strcmp(inst->ident, "jmp") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_SYM) {
      ERROR(op->token, "only symbol is supported.");
    }
    return inst_op1(INST_JMP, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "je") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_SYM) {
      ERROR(op->token, "only symbol is supported.");
    }
    return inst_op1(INST_JE, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "jne") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_SYM) {
      ERROR(op->token, "only symbol is supported.");
    }
    return inst_op1(INST_JNE, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "call") == 0) {
    if (ops->length != 1) {
      ERROR(inst, "'%s' expects 1 operand.", inst->ident);
    }
    Op *op = ops->buffer[0];
    if (op->type != OP_SYM) {
      ERROR(op->token, "only symbol is supported.");
    }
    return inst_op1(INST_CALL, INST_QUAD, op, inst);
  }

  if (strcmp(inst->ident, "leave") == 0) {
    if (ops->length != 0) {
      ERROR(inst, "'%s' expects no operand.", inst->ident);
    }
    return inst_op0(INST_LEAVE, INST_QUAD, inst);
  }

  if (strcmp(inst->ident, "ret") == 0) {
    if (ops->length != 0) {
      ERROR(inst, "'%s' expects no operand.", inst->ident);
    }
    return inst_op0(INST_RET, INST_QUAD, inst);
  }

  ERROR(inst, "unknown instruction '%s'.", inst->ident);
}

Vector *as_parse(Vector *lines) {
  Vector *stmts = vector_new();

  for (int i = 0; i < lines->length; i++) {
    Vector *line = lines->buffer[i];
    Token **token = (Token **) line->buffer;

    if (line->length == 0) continue;
    EXPECT(token[0], TOK_IDENT, "identifier is expected.");

    if (token[1] && token[1]->type == TOK_SEMICOLON) {
      if (token[2]) {
        ERROR(token[2], "invalid symbol declaration.");
      }
      char *ident = token[0]->ident;
      vector_push(stmts, stmt_label(label_new(ident, token[0])));
    } else if (strcmp(token[0]->ident, ".text") == 0) {
      vector_push(stmts, stmt_dir(dir_text(token[0])));
    } else if (strcmp(token[0]->ident, ".data") == 0) {
      vector_push(stmts, stmt_dir(dir_data(token[0])));
    } else if (strcmp(token[0]->ident, ".section") == 0) {
      if (!token[1] || token[1]->type != TOK_IDENT) {
        ERROR(token[0], "identifier is expected.");
      }
      if (token[2]) {
        ERROR(token[0], "invalid directive.");
      }
      char *ident = token[1]->ident;
      if (strcmp(ident, ".rodata") != 0) {
        ERROR(token[0], "only '.rodata' is supported.");
      }
      vector_push(stmts, stmt_dir(dir_section(ident, token[0])));
    } else if (strcmp(token[0]->ident, ".global") == 0) {
      if (!token[1] || token[1]->type != TOK_IDENT) {
        ERROR(token[0], "identifier is expected.");
      }
      if (token[2]) {
        ERROR(token[0], "invalid directive.");
      }
      char *ident = token[1]->ident;
      vector_push(stmts, stmt_dir(dir_global(ident, token[1])));
    } else if (strcmp(token[0]->ident, ".zero") == 0) {
      if (!token[1]) {
        ERROR(token[0], "'.zero' directive expects integer constant.");
      }
      EXPECT(token[1], TOK_NUM, "'.zero' directive expects integer constant.");
      if (token[2]) {
        ERROR(token[0], "invalid directive.");
      }
      int num = token[1]->num;
      vector_push(stmts, stmt_dir(dir_zero(num, token[0])));
    } else if (strcmp(token[0]->ident, ".long") == 0) {
      if (!token[1]) {
        ERROR(token[0], "'.long' directive expects integer constant.");
      }
      EXPECT(token[1], TOK_NUM, "'.long' directive expects integer constant.");
      if (token[2]) {
        ERROR(token[0], "invalid directive.");
      }
      int num = token[1]->num;
      vector_push(stmts, stmt_dir(dir_long(num, token[0])));
    } else if (strcmp(token[0]->ident, ".quad") == 0) {
      if (!token[1]) {
        ERROR(token[0], "'.quad' directive expects string literal.");
      }
      EXPECT(token[1], TOK_IDENT, "'.quad' directive expects string literal.");
      if (token[2]) {
        ERROR(token[0], "invalid directive.");
      }
      char *ident = token[1]->ident;
      vector_push(stmts, stmt_dir(dir_quad(ident, token[1])));
    } else if (strcmp(token[0]->ident, ".ascii") == 0) {
      if (!token[1]) {
        ERROR(token[0], "'.ascii' directive expects string literal.");
      }
      EXPECT(token[1], TOK_STR, "'.ascii' directive expects string literal.");
      if (token[2]) {
        ERROR(token[0], "invalid directive.");
      }
      int length = token[1]->length;
      char *string = token[1]->string;
      vector_push(stmts, stmt_dir(dir_ascii(string, length, token[1])));
    } else {
      vector_push(stmts, stmt_inst(parse_inst(token)));
    }
  }

  return stmts;
}