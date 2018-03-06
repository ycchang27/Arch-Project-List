#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */

unsigned int endianSwap(unsigned int);

void PrintInfo (int changedReg, int changedMem);
unsigned int Fetch (int);
void Decode (unsigned int, DecodedInstr*, RegVals*);
int Execute (DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int *);
void PrintInstruction (DecodedInstr*);

/* Globally accessible Computer variable */
Computer mips;
RegVals rVals;
const int HIGHEST_OPCODE_OR_FUNCT = 43;
enum JumpOpcode{ JAL = 3 }; // additional enum for instruction decoding
enum OtherFuncts{ SLL = 0, ADDU = 33, SUBU = 35, AND = 36, OR = 37, SLT = 42};
enum OtherOpcodes{BEQ = 4, BNE = 5, JR = 8, ADDIU = 9, ANDI = 12, ORI = 13, LUI = 15, LW = 35, SW = 43};
enum Registers{ RA = 31}; // specific registers

// global look up tables
const char *Aop[44];
const char *Afuct[44];

/*
 * Fills in the look up table of supported instructions
 */
void fillin(){
    // supports opcode (I/J-format)
    Aop[9] = "addiu";
    Aop[12] = "andi";
    Aop[13] = "ori";
    Aop[15] = "lui";
    Aop[4] = "beq";
    Aop[5] = "bne";
    Aop[2] = "j";
    Aop[3] = "jal";
    Aop[35] = "lw";
    Aop[43] = "sw";

    // supports funct (R-format)
    Afuct[33] = "addu";
    Afuct[35] = "subu";
    Afuct[0] = "sll";
    Afuct[2] = "srl";
    Afuct[36] = "and";
    Afuct[37] = "or";
    Afuct[42] = "slt";
    Afuct[8] = "jr";
 }

/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer (FILE* filein, int printingRegisters, int printingMemory,
  int debugging, int interactive) {
    int k;
    unsigned int instr;

    /* Initialize registers and memory */
    for (k=0; k<32; k++) {
        mips.registers[k] = 0;
    }
    
    /* stack pointer - Initialize to highest address of data segment */
    mips.registers[29] = 0x00400000 + (MAXNUMINSTRS+MAXNUMDATA)*4;

    for (k=0; k<MAXNUMINSTRS+MAXNUMDATA; k++) {
        mips.memory[k] = 0;
    }

    k = 0;
    while (fread(&instr, 4, 1, filein)) {
	  /*swap to big endian, convert to host byte order. Ignore this.*/
        mips.memory[k] = ntohl(endianSwap(instr));
        k++;
        if (k>MAXNUMINSTRS) {
            fprintf (stderr, "Program too big.\n");
            exit (1);
        }
    }

    mips.printingRegisters = printingRegisters;
    mips.printingMemory = printingMemory;
    mips.interactive = interactive;
    mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
    return (i>>24)|(i>>8&0x0000ff00)|(i<<8&0x00ff0000)|(i<<24);
}

/*
 *  Run the simulation.
 */
void Simulate () {
    char s[40];  /* used for handling interactive input */
    unsigned int instr;
    int changedReg=-1, changedMem=-1, val;
    DecodedInstr d;
    
    fillin(); // fill in the look up table for instructions

    /* Initialize the PC to the start of the code section */
    mips.pc = 0x00400000;
    while (1) {
        if (mips.interactive) {
            printf ("> ");
            fgets (s,sizeof(s),stdin);
            if (s[0] == 'q') {
                return;
            }
        }

        /* Fetch instr at mips.pc, returning it in instr */
        instr = Fetch (mips.pc);

        printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

        /* 
    	   * Decode instr, putting decoded instr in d
    	   * Note that we reuse the d struct for each instruction.
    	   */
        Decode (instr, &d, &rVals);

        /*Print decoded instruction*/
        PrintInstruction(&d); 

        /* 
    	   * Perform computation needed to execute d, returning computed value 
    	   * in val 
    	   */
        val = Execute(&d, &rVals);

	      UpdatePC(&d,&val);

        /* 
      	 * Perform memory load or store. Place the
      	 * address of any updated memory in *changedMem, 
      	 * otherwise put -1 in *changedMem. 
      	 * Return any memory value that is read, otherwise return -1.
         */
        val = Mem(&d, val, &changedMem);

        /* 
      	 * Write back to register. If the instruction modified a register--
      	 * (including jal, which modifies $ra) --
         * put the index of the modified register in *changedReg,
         * otherwise put -1 in *changedReg.
         */
        RegWrite(&d, val, &changedReg);

        PrintInfo (changedReg, changedMem);
    }
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo ( int changedReg, int changedMem) {
    int k, addr;
    printf ("New pc = %8.8x\n", mips.pc);
    if (!mips.printingRegisters && changedReg == -1) {
        printf ("No register was updated.\n");
    } else if (!mips.printingRegisters) {
        printf ("Updated r%2.2d to %8.8x\n",
        changedReg, mips.registers[changedReg]);
    } else {
        for (k=0; k<32; k++) {
            printf ("r%2.2d: %8.8x  ", k, mips.registers[k]);
            if ((k+1)%4 == 0) {
                printf ("\n");
            }
        }
    }
    if (!mips.printingMemory && changedMem == -1) {
        printf ("No memory location was updated.\n");
    } else if (!mips.printingMemory) {
        printf ("Updated memory at address %8.8x to %8.8x\n",
        changedMem, Fetch (changedMem));
    } else {
        printf ("Nonzero memory\n");
        printf ("ADDR	  CONTENTS\n");
        for (addr = 0x00400000+4*MAXNUMINSTRS;
             addr < 0x00400000+4*(MAXNUMINSTRS+MAXNUMDATA);
             addr = addr+4) {
            if (Fetch (addr) != 0) {
                printf ("%8.8x  %8.8x\n", addr, Fetch (addr));
            }
        }
    }
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch ( int addr) {
    return mips.memory[(addr-0x00400000)/4];
}

/* Decode instr, returning decoded instruction. 
 * Instruction Description: https://www.d.umn.edu/~gshute/mips/instruction-types.xhtml
 */
void Decode ( unsigned int instr, DecodedInstr* d, RegVals* rVals) {
    /* Calculate and set opcode */
    unsigned int opcode = instr >> 26;                  // 31-26
    d->op = opcode;

    /* R-format */
    if(opcode == R) {
        /* set up DecodedInstr's variables */
        d->type = R;
        d->regs.r.rs = (instr & 0x3E00000) >> 21;       // 25-21
        d->regs.r.rt = (instr & 0x1F0000) >> 16;        // 20-16
        d->regs.r.rd = (instr & 0xF800) >> 11;          // 15-11
        d->regs.r.shamt = (instr & 0x7C0) >> 6;         // 10-6
        d->regs.r.funct = instr & 0x3F;                 // 5-0

        /* The instruction does not exist */
        if(d->regs.r.funct > HIGHEST_OPCODE_OR_FUNCT || Afuct[d->regs.r.funct] == NULL) {
          exit(0);
        }

        /* set up Register values */
        rVals->R_rs = mips.registers[d->regs.r.rs];
        rVals->R_rt = mips.registers[d->regs.r.rt];
        rVals->R_rd = mips.registers[d->regs.r.rd];
    }
    /* J-format */
    else if(opcode == J || opcode == JAL) {
        /* set up DecodedInstr's variables */
        d->type = J;
        d->regs.j.target = (instr & 0x3FFFFFF)*4;           // 25-0
    }
    /* I-format */
    else {
        /* The instruction does not exist */
        if(opcode > HIGHEST_OPCODE_OR_FUNCT || Aop[opcode] == NULL) {
          exit(0);
        }

        /* set up DecodedInstr's variables */ 
        d->type = I;
        d->regs.i.rs = (instr & 0x3E00000) >> 21;       // 25-21
        d->regs.i.rt = (instr & 0x1F0000) >> 16;        // 20-16

        // sign extend immediate (15-0)
        d->regs.i.addr_or_immed = ((int32_t)(int16_t)(instr & 0xFFFF));

        /* set up Register values */
        rVals->R_rs = mips.registers[d->regs.r.rs];
        rVals->R_rt = mips.registers[d->regs.r.rt];
    }
}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 *  Format:
 *  [instruction name]\t[register1][other registers/immediate]
 *    - [instruction name] = name of the instruction (ex: addi)
 *    - [other registers/immediate] = can be one of the following:
 *        1) , [register2], [register3] (ex: add $0, $0, $1)
 *        2) , [register2], [immediate] (ex: addi $0, $0. 1)
 *        3) , [immediate](register2) (ex: lw $1, 0($2))
 *        4) , [immediate] (ex: j 0x0040002c)
 *        5) [blank] (ex: jr $31)
 *    - [register1/2/3] = register identified by number (ex: $1)
 *    - [immediate] = can be one of the following:
 *        1) decimal number (with negative sign if required) 
 *        (applies toaddiu, srl, sll, lw and sw)
 *        2) hex with leading 0x (applies to andi, ori, and lui)
 *        3) full 8-digit hex (put leading zeros if necessary)
 *        (applies to branch and jump instructions (except for jr))
 *    Note: 
 *        * All hex values must use lower-case letters. 
 *        * The target of the branch or jump should be printed as an 
 *        absolute address, rather than being PC relative
 */
void PrintInstruction ( DecodedInstr* d) {

	if(d->type == R && Afuct[d->regs.r.funct] == "jr"){
            printf("%s\t$%i\n" ,Afuct[d->regs.r.funct], d->regs.r.rs);
        }
    else if(d->type == R && (Afuct[d->regs.r.funct] == "sll" || Afuct[d->regs.r.funct] == "srl")){
            printf("%s\t$%i, $%i, %i\n" ,Afuct[d->regs.r.funct], d->regs.r.rd, d->regs.r.rt, d->regs.r.shamt);
        }

    else if(d->type == R){
            printf("%s\t$%i, $%i, $%i\n" ,Afuct[d->regs.r.funct], d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
        }
        
    if(d->type == I && (Aop[d->op] == "beq" || Aop[d->op] == "bne")){
            printf("%s\t$%i, $%i, 0x%8.8x\n" ,Aop[d->op], d->regs.i.rs, d->regs.i.rt, d->regs.i.addr_or_immed*4 + 4 + mips.pc);
    }

    else if(d->type == I && (Aop[d->op] == "sw" || Aop[d->op] == "lw")){
            printf("%s\t$%i, %i($%i)\n" ,Aop[d->op], d->regs.i.rt, d->regs.i.addr_or_immed, d->regs.i.rs);
    }
    else if(d->type == I && Aop[d->op] == "lui"){
            printf("%s\t$%i, 0x%8.8x\n" ,Aop[d->op], d->regs.i.rt, d->regs.i.addr_or_immed);
    }
    else if(d->type == I && (Aop[d->op] == "andi" || Aop[d->op] == "ori")){
            printf("%s\t$%i, $%i, 0x%8.8x\n" ,Aop[d->op], d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
    }
    else if(d->type == I){
            printf("%s\t$%i, $%i, %i\n" ,Aop[d->op], d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
    }

    if(d->type == J ){
            printf("%s\t0x%8.8x\n" ,Aop[d->op], d->regs.j.target);
    }
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
  int function = (d->type == R) ? d->regs.r.funct : d->op;
  int val = 0;
  /* R-format */
  if(d->type == R && function != JR) {
    function = d->regs.r.funct;
    int rs = rVals->R_rs, rt = rVals->R_rt;

    /* SHIFT instruction */
    if(d->regs.r.shamt != 0) {
      int shamt = d->regs.r.shamt;
      val = (function == SLL) ? rt << shamt : rt >> shamt;
    }
    /* ADD instruction */
    else if(function == ADDU) {
      val = rs + rt;
    }
    /* SUB instruction */
    else if(function == SUBU) {
      val = rs - rt;
    }
    /* < instruction */
    else if(function == SLT) {
      val = (rs < rt);
    }
    /* OR instruction */
    else if(function == OR) {
      val = rs | rt;
    }
    /* AND instruction */
    else if(function == AND) {
      val = rs & rt;
    }
    /* Invalid instruction */
    else {
      exit(0);
    }

    return val;
  }
  /* I-format */
  else if(d->type == I) {
    function = d->op;
    int rs = rVals->R_rs, imm = d->regs.i.addr_or_immed;

    /* ADDI, SW/LW instruction */
    if(function == ADDIU || function == SW || function == LW) {
      val = rs + imm;
    }
    /* BRANCH instruction */
    else if(function == BEQ || function == BNE) {
      int rt = rVals->R_rt;
      val = (rs == rt);
    }
    /* ORI instruction */
    else if(function == ORI) {
      val = rs | imm;
    }
    /* ANDI instruction */
    else if(function == ANDI) {
      val = rs & imm;
    }
    /* LUI instruction */
    else if(function == LUI) {
      val = 0; // do nothing
    }
    /* Invalid instruction */
    else {
      exit(0);
    }
    
    return val;
  }
  /* J-format */
  // // DEBUG_PRINT(printf("DEBUG EXECUTE: J-FORMAT\n"), DEBUGGING);
  return val;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int* val) {
  /* jumps (except jr) */
  if(d->type == J) {
    *val = mips.pc + 4; // save PC address in case of jal
    mips.pc = d->regs.j.target;
  }
  /* beq */
  else if(d->op == BEQ) {
    mips.pc += (*val == 1) ? (d->regs.i.addr_or_immed*4 + 4) : 4;
  }
  /* bne */
  else if(d->op == BNE) {
    mips.pc += (*val != 1) ? (d->regs.i.addr_or_immed*4 + 4) : 4;
  }
  /* jr */
  else if(d->type == R && d->regs.r.funct == JR) {
    mips.pc = mips.registers[RA];
  }
  /* other instructions */
  else {
    mips.pc += 4;
  }
}

/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 *
 */
int Mem( DecodedInstr* d, int val, int *changedMem) {
  int function = d->op;
  /* any instructions that aren't LW/SW */
  if(function != SW && function != LW) {
    *changedMem = -1;
    return val;
  }

  /* value out of range or invalid value */
  if(!(val >= 0x00401000 && val <= 0x00403FFF) || val % 4 != 0) {
    printf("Memory Access Exception at [0x%08x]: address [0x%08x]\n", (mips.pc-4), val);
    exit(0);
  }

  int rt = d->regs.i.rt;
  
  /* SW */
  if(function == SW) {
    mips.memory[(val-0x00400000)/4] = mips.registers[rt];
    *changedMem = val;
  }
  
  /* LW */
  else {
    val = mips.memory[(val-0x00400000)/4];
    *changedMem = -1;
  }

  return val;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
  int function = d->op;
  /* No reg write case */
  if(d->regs.r.funct == JR || (d->type != R && (function == BEQ || function == BNE || function == J || function == SW))) {
    *changedReg = -1;
  }
  /* R-format */
  else if(d->type == R) {
    function = d->regs.r.funct;
    
    // Write to register
    int rd = d->regs.r.rd;
    mips.registers[rd] = val;
    *changedReg = rd;
  }
  /* LUI */
  else if(function == LUI) {
    int rt = d->regs.r.rt;
    int shifted_imm = d->regs.i.addr_or_immed << 16;
    mips.registers[rt] = shifted_imm;
    *changedReg = rt;
  }
  /* LW */
  else if(function == LW) {
    int rt = d->regs.r.rt;
    mips.registers[rt] = val;
    *changedReg = rt;
  }
  /* I-format (except JAL) */
  else if(function != JAL && d->type == I) {
    // Write to register
    int rt = d->regs.i.rt;
    mips.registers[rt] = val;
    *changedReg = rt;
  }
  /* JAL */
  else if(function == JAL) {
    // Write to register
    mips.registers[RA] = val;
    *changedReg = RA;
  }
}
