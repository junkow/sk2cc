#include "as.h"

Reloc *reloc_new(int offset, char *sym, Token *token) {
  Reloc *reloc = (Reloc *) calloc(1, sizeof(Reloc));
  reloc->offset = offset;
  reloc->sym = sym;
  reloc->token = token;
  return reloc;
}

#define REXW_PRE(R, X, B) \
  (0x48 | (((R >> 3) & 1) << 2) | (((X >> 3) & 1) << 1) | ((B >> 3) & 1))

#define REX_PRE(R, X, B) \
  (0x40 | (((R >> 3) & 1) << 2) | (((X >> 3) & 1) << 1) | ((B >> 3) & 1))

#define MOD_RM(Mod, RegOpcode, RM) \
  (((Mod << 6) & 0xc0) | ((RegOpcode << 3) & 0x38) | (RM & 0x07))

#define MOD_DISP0 0
#define MOD_DISP8 1
#define MOD_DISP32 2
#define MOD_REG 3

#define MOD_MEM(disp) \
  (disp == 0 ? MOD_DISP0 : (-128 <= disp && disp < 128 ? MOD_DISP8 : MOD_DISP32))

#define OPCODE_REG(Opcode, Reg) \
  ((Opcode & 0xf8) | (Reg & 0x07))

#define IMM_ID0(id) (id & 0xff)
#define IMM_ID1(id) ((id >> 8) & 0xff)
#define IMM_ID2(id) ((id >> 16) & 0xff)
#define IMM_ID3(id) (id >> 24)

void gen_text(Unit *unit) {
  Vector *insts = unit->insts;
  Binary *text = binary_new();
  int *addrs = (int *) calloc(insts->length, sizeof(int));
  Vector *relocs = vector_new();

  for (int i = 0; i < insts->length; i++) {
    Inst *inst = insts->array[i];
    addrs[i] = text->length;

    switch (inst->type) {
      case INST_PUSH: {
        if (inst->ops->length != 1) {
          ERROR(inst->token, "'pushq' expects 1 operand.\n");
        }
        Op *op = inst->ops->array[0];

        if (op->type == OP_REG) {
          // 50 +rd
          Byte rex = REX_PRE(0, 0, op->reg);
          Byte opcode = OPCODE_REG(0x50, op->reg);
          if (op->reg < 8) {
            binary_append(text, 1, opcode);
          } else {
            binary_append(text, 2, rex, opcode);
          }
        } else {
          ERROR(op->token, "invalid operand type.\n");
        }
      }
      break;

      case INST_POP: {
        if (inst->ops->length != 1) {
          ERROR(inst->token, "'popq' expects 1 operand.\n");
        }
        Op *op = inst->ops->array[0];

        if (op->type == OP_REG) {
          // 58 +rd
          Byte rex = REX_PRE(0, 0, op->reg);
          Byte opcode = OPCODE_REG(0x58, op->reg);
          if (op->reg < 8) {
            binary_append(text, 1, opcode);
          } else {
            binary_append(text, 2, rex, opcode);
          }
        } else {
          ERROR(op->token, "invalid operand type.\n");
        }
      }
      break;

      case INST_MOV: {
        if (inst->ops->length != 2) {
          ERROR(inst->token, "'movq' expects 2 operands.\n");
        }
        Op *src = inst->ops->array[0];
        Op *dest = inst->ops->array[1];

        if (src->type == OP_IMM && dest->type == OP_REG) {
          // REX.W + C7 /0 id
          Byte rex = REXW_PRE(0, 0, dest->reg);
          Byte opcode = 0xc7;
          Byte mod_rm = MOD_RM(MOD_REG, 0, dest->reg);
          Byte imm0 = IMM_ID0(src->imm);
          Byte imm1 = IMM_ID1(src->imm);
          Byte imm2 = IMM_ID2(src->imm);
          Byte imm3 = IMM_ID3(src->imm);
          binary_append(text, 7, rex, opcode, mod_rm, imm0, imm1, imm2, imm3);
        } else if (src->type == OP_REG && dest->type == OP_REG) {
          // REX.W + 8B /r
          Byte rex = REXW_PRE(dest->reg, 0, src->reg);
          Byte opcode = 0x8b;
          Byte mod_rm = MOD_RM(MOD_REG, dest->reg, src->reg);
          binary_append(text, 3, rex, opcode, mod_rm);
        } else if (src->type == OP_REG && dest->type == OP_MEM) {
          if (dest->base == 4) {
            ERROR(dest->token, "rsp is not supported.\n");
          }
          if (dest->base == 5) {
            ERROR(dest->token, "rbp is not supported.\n");
          }
          // REX.W + 89 /r
          int mod = MOD_MEM(dest->disp);
          Byte rex = REXW_PRE(src->reg, 0, dest->base);
          Byte opcode = 0x89;
          Byte mod_rm = MOD_RM(mod, src->reg, dest->base);
          if (mod == MOD_DISP0) {
            binary_append(text, 3, rex, opcode, mod_rm);
          } else if (mod == MOD_DISP8) {
            Byte disp = (signed char) dest->disp;
            binary_append(text, 4, rex, opcode, mod_rm, disp);
          } else if (mod == MOD_DISP32) {
            Byte disp0 = ((unsigned int) dest->disp) & 0xff;
            Byte disp1 = (((unsigned int) dest->disp) >> 8) & 0xff;
            Byte disp2 = (((unsigned int) dest->disp) >> 16) & 0xff;
            Byte disp3 = (((unsigned int) dest->disp) >> 24) & 0xff;
            binary_append(text, 7, rex, opcode, mod_rm, disp0, disp1, disp2, disp3);
          }
        } else if (src->type == OP_MEM && dest->type == OP_REG) {
          if (src->base == 4) {
            ERROR(src->token, "rsp is not supported.\n");
          }
          if (src->base == 5) {
            ERROR(src->token, "rbp is not supported.\n");
          }
          // REX.W + 8B /r
          int mod = MOD_MEM(src->disp);
          Byte rex = REXW_PRE(dest->reg, 0, src->base);
          Byte opcode = 0x8b;
          Byte mod_rm = MOD_RM(mod, dest->reg, src->base);
          if (mod == MOD_DISP0) {
            binary_append(text, 3, rex, opcode, mod_rm);
          } else if (mod == MOD_DISP8) {
            Byte disp = (signed char) src->disp;
            binary_append(text, 4, rex, opcode, mod_rm, disp);
          } else if (mod == MOD_DISP32) {
            Byte disp0 = ((unsigned int) src->disp) & 0xff;
            Byte disp1 = (((unsigned int) src->disp) >> 8) & 0xff;
            Byte disp2 = (((unsigned int) src->disp) >> 16) & 0xff;
            Byte disp3 = (((unsigned int) src->disp) >> 24) & 0xff;
            binary_append(text, 7, rex, opcode, mod_rm, disp0, disp1, disp2, disp3);
          }
        } else {
          ERROR(src->token, "invalid operand types.\n");
        }
      }
      break;

      case INST_CALL: {
        if (inst->ops->length != 1) {
          ERROR(inst->token, "'call' expects 1 operand.\n");
        }
        Op *op = inst->ops->array[0];
        if (op->type == OP_SYM) {
          // E8 cd
          Byte opcode = 0xe8;
          binary_append(text, 5, opcode, 0, 0, 0, 0);
          vector_push(relocs, reloc_new(text->length - 4, op->sym, op->token));
        } else {
          ERROR(inst->token, "invalid operand type.\n");
        }
      }
      break;

      case INST_LEAVE: {
        if (inst->ops->length != 0) {
          ERROR(inst->token, "'leave' expects no operand.\n");
        }
        // C9
        Byte opcode = 0xc9;
        binary_append(text, 1, opcode);
      }
      break;

      case INST_RET: {
        if (inst->ops->length != 0) {
          ERROR(inst->token, "'ret' expects no operand.\n");
        }
        // C3
        Byte opcode = 0xc3;
        binary_append(text, 1, opcode);
      }
      break;

      default: {
        ERROR(inst->token, "unknown instruction.\n");
      }
    }
  }

  unit->text = text;
  unit->relocs = relocs;
  unit->addrs = addrs;
}

void gen_symtab(Unit *unit) {
  Map *labels = unit->labels;
  int *addrs = unit->addrs;
  Binary *symtab = binary_new();
  String *strtab = string_new();
  Map *gsyms = map_new();

  binary_write(symtab, calloc(1, sizeof(Elf64_Sym)), sizeof(Elf64_Sym));
  string_push(strtab, '\0');

  for (int i = 0; i < labels->count; i++) {
    char *key = labels->keys[i];
    Label *val = labels->values[i];

    Elf64_Sym *sym = (Elf64_Sym *) calloc(1, sizeof(Elf64_Sym));
    sym->st_name = strtab->length;
    sym->st_info = ELF64_ST_INFO(STB_GLOBAL, STT_NOTYPE);
    sym->st_other = STV_DEFAULT;
    sym->st_shndx = 1;
    sym->st_value = addrs[val->inst];

    binary_write(symtab, sym, sizeof(Elf64_Sym));
    string_write(strtab, key);
    string_push(strtab, '\0');
    map_put(gsyms, key, (void *) (intptr_t) (i + 1));
  }

  unit->symtab = symtab;
  unit->strtab = strtab;
  unit->gsyms = gsyms;
}

void gen_rela_text(Unit *unit) {
  Vector *relocs = unit->relocs;
  Map *gsyms = unit->gsyms;
  Binary *rela_text = binary_new();

  for (int i = 0; i < relocs->length; i++) {
    Reloc *reloc = relocs->array[i];
    int index = (int) (intptr_t) map_lookup(gsyms, reloc->sym);
    if (index == 0) {
      ERROR(reloc->token, "undefined symbol: %s.\n", reloc->sym);
    }

    Elf64_Rela *rela = (Elf64_Rela *) calloc(1, sizeof(Elf64_Rela));
    rela->r_offset = reloc->offset;
    rela->r_info = ELF64_R_INFO(index, R_X86_64_PC32);
    rela->r_addend = -4;

    binary_write(rela_text, rela, sizeof(Elf64_Rela));
  }

  unit->rela_text = rela_text;
}

void gen(Unit *unit) {
  gen_text(unit);
  gen_symtab(unit);
  gen_rela_text(unit);
}
