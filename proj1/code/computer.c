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
int DEBUGGING;
enum JumpOpcode{ JAL = 3 }; // additional enum for instruction decoding
enum OtherFuncts{ SLL = 0, ADDU = 33, SUBU = 35, AND = 36, OR = 37, SLT = 42};
enum OtherOpcodes{BEQ = 4, BNE = 5, JR = 8, ADDIU = 9, ANDI = 12, ORI = 13, LUI = 15, LW = 35, SW = 43};
enum Registers{ RA = 31}; // specific registers

const char *Aop[43];
    
const char *Afuct[43];

void fillin(){
    Aop[33] = "addu";
    Aop[9] = "addiu";
    //Aop[35] = "subu";
    Aop[0] = "sll";
    Aop[2] = "srl";
    Aop[36] = "and";
    Aop[12] = "andi";
    Aop[37] = "or";
    Aop[13] = "ori";
    Aop[15] = "lui";
    Aop[42] = "slt";
    Aop[4] = "beq";
    Aop[5] = "bne";
    Aop[2] = "j";
    Aop[3] = "jal";
    Aop[35] = "lw";
    Aop[43] = "sw";

    Afuct[33] = "addu";
    Afuct[9] = "addiu";
    Afuct[35] = "subu";
    Afuct[0] = "sll";
    Afuct[2] = "srl";
    Afuct[36] = "and";
    Afuct[12] = "andi";
    Afuct[37] = "or";
    Afuct[13] = "ori";
    Afuct[15] = "lui";
    Afuct[42] = "slt";
    Afuct[4] = "beq";
    Afuct[5] = "bne";
    Afuct[2] = "j";
    Afuct[3] = "jal";
    Afuct[8] = "jr";
    //Afuct[35] = "lw";
    Afuct[43] = "sw";
 }

/* Debug Macro */
// #define // // DEBUG_PRINT(print, DEBUG) if(DEBUG) print

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

    /* Setup debug flag */
    //DEBUGGING = debugging;

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
    // Calculate and set opcode
    unsigned int opcode = instr >> 26;                  // 31-26
    d->op = opcode;

    /* R-format */
    if(opcode == R) {
        // // DEBUG_PRINT(printf("DEBUG DECODE: Instruction 0x%8.8x is in R-format\n", instr), DEBUGGING);

        // set up DecodedInstr's variables here... 
        d->type = R;
        d->regs.r.rs = (instr & 0x3E00000) >> 21;       // 25-21
        d->regs.r.rt = (instr & 0x1F0000) >> 16;        // 20-16
        d->regs.r.rd = (instr & 0xF800) >> 11;          // 15-11
        d->regs.r.shamt = (instr & 0x7C0) >> 6;         // 10-6
        d->regs.r.funct = instr & 0x3F;                 // 5-0

        /* The instruction does not exist */
        if(Afuct[d->regs.r.funct] == NULL) {
          exit(0);
        }

        // set up Register values here
        rVals->R_rs = mips.registers[d->regs.r.rs];
        rVals->R_rt = mips.registers[d->regs.r.rt];
        rVals->R_rd = mips.registers[d->regs.r.rd];
    }
    /* J-format */
    else if(opcode == J || opcode == JAL) {
        // // DEBUG_PRINT(printf("DEBUG DECODE: Instruction 0x%8.8x is in J-format\n", instr), DEBUGGING);
        
        // set up DecodedInstr's variables here... 
        d->type = J;
        d->regs.j.target = (instr & 0x3FFFFFF)*4;           // 25-0
    }
    /* I-format */
    else {
        // // DEBUG_PRINT(printf("DEBUG DECODE: Instruction 0x%8.8x is in I-format\n", instr), DEBUGGING);
        /* The instruction does not exist */
        if(Aop[opcode] == NULL) {
          exit(0);
        }

        // set up DecodedInstr's variables here... 
        d->type = I;
        d->regs.i.rs = (instr & 0x3E00000) >> 21;       // 25-21
        d->regs.i.rt = (instr & 0x1F0000) >> 16;        // 20-16

        // sign extend immediate (15-0)
        d->regs.i.addr_or_immed = ((int32_t)(int16_t)(instr & 0xFFFF));

        // set up Register values here
        rVals->R_rs = mips.registers[d->regs.r.rs];
        rVals->R_rt = mips.registers[d->regs.r.rt];
    }
    // // DEBUG_PRINT(printf("DEBUG DECODE: Done\n"), DEBUGGING);
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
    // start after finishing Decode
    //DEBUG_PRINT(printf("DEBUG: %d\n",d->type), DEBUGGING);


    if(d->type == R){
            printf("%s $%i, $%i, $%i\n" ,Afuct[d->regs.r.funct], d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
        }
    if(d->type == I && (Aop[d->op] == "beq" || Aop[d->op] == "bne")){
            printf("%s $%i, $%i, 0x%8.8x\n" ,Aop[d->op], d->regs.i.rs, d->regs.i.rt, d->regs.i.addr_or_immed*4 + 4 + mips.pc);
    }

    else if(d->type == I && (Aop[d->op] == "sw" || Aop[d->op] == "lw")){
            printf("%s $%i, %i($%i)\n" ,Aop[d->op], d->regs.i.rt, d->regs.i.addr_or_immed, d->regs.i.rs);
    }
    else if(d->type == I && Aop[d->op] == "lui"){
            printf("%s $%i, %i\n" ,Aop[d->op], d->regs.i.rt, d->regs.i.addr_or_immed);
    }
    else if(d->type == I){
            printf("%s $%i, $%i, %i\n" ,Aop[d->op], d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
    }

    if(d->type == J){
            printf("%s 0x%8.8x\n" ,Aop[d->op], d->regs.j.target);
    }
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
  int function = (d->type == R) ? d->regs.r.funct : d->op;
  int val = 0;
  /* R-format */
  if(d->type == R && function != JR) {
    // // DEBUG_PRINT(printf("DEBUG EXECUTE: R-FORMAT\n"), DEBUGGING);
    function = d->regs.r.funct;
    int rs = rVals->R_rs, rt = rVals->R_rt;

    /* SHIFT instruction */
    if(d->regs.r.shamt != 0) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: SHIFT\n"), DEBUGGING);
      int shamt = d->regs.r.shamt;
      val = (function == SLL) ? rt << shamt : rt >> shamt;
    }
    /* ADD instruction */
    else if(function == ADDU) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: ADD\n"), DEBUGGING);
      val = rs + rt;
    }
    /* SUB instruction */
    else if(function == SUBU || function == SLT) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: SUB\n"), DEBUGGING);
      val = rs - rt;
    }
    /* OR instruction */
    else if(function == OR) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: OR\n"), DEBUGGING);
      val = rs | rt;
    }
    /* AND instruction */
    else if(function == AND) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: AND\n"), DEBUGGING);
      val = rs & rt;
    }
    /* Invalid instruction */
    else {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: INVALID INSTRUCTION\n"), DEBUGGING);
      exit(0);
    }
    
    return val;
  }
  /* I-format */
  else if(d->type == I) {
    // // DEBUG_PRINT(printf("DEBUG EXECUTE: I-FORMAT\n"), DEBUGGING);
    function = d->op;
    int rs = rVals->R_rs, imm = d->regs.i.addr_or_immed;

    /* ADD instruction */
    if(function == ADDIU || function == LW || function == SW) {
      //val = (function == ADDIU) ? rs + imm : mips.memory[rs] + imm;
      val = rs + imm;
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: ADD. val = %d\n", val), DEBUGGING);
    }
    /* BRANCH instruction */
    else if(function == BEQ || function == BNE) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: SUB\n"), DEBUGGING);
      int rt = rVals->R_rt;
      val = (rs == rt);
    }
    /* SW/LW instruction */
    else if(function == SW || function == LW) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: SW/LW\n"), DEBUGGING);
      val = rs + imm;
    }
    /* ORI instruction */
    else if(function == ORI) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: ORI\n"), DEBUGGING);
      val = rs | imm;
    }
    /* AND instruction */
    else if(function == ANDI) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: ANDI\n"), DEBUGGING);
      val = rs & imm;
    }
    /* LUI instruction */
    else if(function == LUI) {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: LUI\n"), DEBUGGING);
      val = 0; // do nothing
    }
    /* Invalid instruction */
    else {
      // // DEBUG_PRINT(printf("DEBUG EXECUTE: INVALID INSTRUCTION\n"), DEBUGGING);
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
    // // DEBUG_PRINT(printf("DEBUG UPDATEPC: J or JAL to 0x%8.8x\n", d->regs.j.target), DEBUGGING);
    *val = mips.pc + 4; // save PC address in case of jal
    mips.pc = d->regs.j.target;
  }
  /* beq */
  else if(d->op == BEQ) {
    // // DEBUG_PRINT(printf("DEBUG UPDATEPC: BEQ = %d\n", val), DEBUGGING);
    mips.pc += (*val == 1) ? (d->regs.i.addr_or_immed*4 + 4) : 4;
  }
  /* bne */
  else if(d->op == BNE) {
    // // DEBUG_PRINT(printf("DEBUG UPDATEPC: BNE\n"), DEBUGGING);
    mips.pc += (*val != 1) ? (d->regs.i.addr_or_immed*4 + 4) : 4;
  }
  /* jr */
  else if(d->type == R && d->regs.r.funct == JR) {
    // // DEBUG_PRINT(printf("DEBUG UPDATEPC: JR to 0x%8.8x\n", mips.registers[RA]), DEBUGGING);
    mips.pc = mips.registers[RA];
  }
  /* other instructions */
  else {
    // // DEBUG_PRINT(printf("DEBUG UPDATEPC: OTHER INSTRUCTIONS\n"), DEBUGGING);
    mips.pc += 4;
  }
  // // DEBUG_PRINT(printf("DEBUG UPDATEPC: New PC = %8.8x\n", mips.pc), DEBUGGING);
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
  if(function != SW && function != LW) {
    // // DEBUG_PRINT(printf("DEBUG MEM: NOT LW OR SW\n"), DEBUGGING);
    *changedMem = -1;
    return val;
  }
  /* value out of range or invalid value */
  if(!(val >= 0x00401000 && val <= 0x00404000) || val % 4 != 0) {
    printf("Memory Access Exception at [0x%08x]: address [0x%08x]\n", mips.pc, val);
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
    mips.registers[rt] = mips.memory[(val-0x00400000)/4];
    *changedMem = -1;
    val = mips.registers[rt];
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
  if(d->regs.r.funct == JR || (d->type != R && (function == BEQ || function == J || function == SW))) {
    // // DEBUG_PRINT(printf("DEBUG REGWRITE: NO WRITE\n"), DEBUGGING);
    *changedReg = -1;
  }
  /* R-format */
  else if(d->type == R) {
    // // DEBUG_PRINT(printf("DEBUG REGWRITE: R-FORMAT\n"), DEBUGGING);
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
    // // DEBUG_PRINT(printf("DEBUG REGWRITE: LW\n"), DEBUGGING);
    int rt = d->regs.r.rt;
    mips.registers[rt] = val;
    *changedReg = rt;
  }
  /* I-format (except JAL) */
  else if(function != JAL && d->type == I) {
    // // DEBUG_PRINT(printf("DEBUG REGWRITE: I-FORMAT (NO JAL). val = %d\n", val), DEBUGGING);
    // Write to register
    int rt = d->regs.i.rt;
    mips.registers[rt] = val;
    *changedReg = rt;
    // // DEBUG_PRINT(printf("DEBUG REGWRITE: changedReg = %d\n", *changedReg), DEBUGGING);
  }
  /* JAL */
  else if(function == JAL) {
    // // DEBUG_PRINT(printf("DEBUG REGWRITE: JAL\n"), DEBUGGING);
    // Write to register
    mips.registers[RA] = val;
    *changedReg = RA;
  }
}
