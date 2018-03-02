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
void UpdatePC(DecodedInstr*, int);
void PrintInstruction (DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

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

	    UpdatePC(&d,val);

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

/* Decode instr, returning decoded instruction. */
void Decode ( unsigned int instr, DecodedInstr* d, RegVals* rVals) {
    /* Your code goes here */
    /*
    Given an instruction, Hill out the corresponding information in a
    DecodedInstr struct. Perform register reads and Hill the RegVals struct. The
    addr_or_immed Hield of the IRegs struct should contain the properly extended version
    of the 16 bits of the immediate Hield.
    */
}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction ( DecodedInstr* d) {
    /* Your code goes here */
    /*
    The PrintInstruction function prints the current instruction and its operands in text.
    Here are the details on the output format and sample.output Hile contains the expected
    output for sample.dump:
        • The disassembled instruction must have the instruction name followed by a "tab"
        character (In C, this character is '\t'), followed by a comma-and-space separated list
        of the operations.
        • For addiu, srl, sll, lw and sw, the immediate value must be printed as a decimal number
        (with the negative sign, if required) with no leading zeroes unless the value is
        exactly zero (printed as 0).
        • For andi, ori, and lui, the immediate must be printed in hex, with a leading 0x and no
        leading zeroes unless the value is exactly zero (which is printed as 0x0).
        • For the branch and jump instructions (except for jr), the target must be printed as a
        full 8-digit hex number, even if it has leading zeroes. (Note the difference between
        this format and the branch and jump assembly language instructions that you
        write.) Finally, the target of the branch or jump should be printed as an absolute
        address, rather than being PC relative.
        • All hex values must use lower-case letters and have the leading 0x.
        • Instruction arguments must be separated by a comma followed by a single space.
        • Registers must be identiHied by number, with no leading zeroes (e.g. $10 and $3) and
        not by name (e.g. $t2).
        • Terminate your output from the PrintInstruction function with a newline.
        Here are examples of good instructions printed by PrintInstruction:
            addiu $1, $0, -2
            lw $1, 8($3)
            srl $6, $7, 3
            ori $1, $1, 0x1234
            lui $10, 0x5678
            j 0x0040002c
            bne $3, $4, 0x00400044
            jr $31
    */
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
    /* Your code goes here */
    /*
    Perform any ALU computation associated with the instruction, and return
    the value. For a lw instruction, for example, this would involve computing the base +
    the offset address. For this project, branch comparisons also occur in this stage.
    */
  return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int val) {
    mips.pc+=4;
    /* Your code goes here */
    /*
    In the UpdatePC function, you should perform the PC update associated with the current
    instruction. For most instructions, this corresponds with an increment of 4 (which we have
    already added).
    */
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
    /* Your code goes here */
    /*
    Perform any memory reads or writes associated with the instruction. Note
    that as in the Fetch function, we map the MIPS address 0x00400000 to index 0 in
    our internal memory array, MIPS address 0x00400004 to index 1, and so forth. If
    an instruction accesses an invalid memory address (outside of our data memory
    range, 0x00401000 - 0x00403fff, or not word aligned for lw or sw), your code
    must print the message, "Memory Access Exception at [PC val]: address [address]",
    where [PC val] is the current PC, and [address] is the offending address, both printed
    in hex (with leading 0x). Then you must call exit(0).
    */
  return 0;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
    /* Your code goes here */
    /*
    Perform any register writes needed.
    
    */
}
