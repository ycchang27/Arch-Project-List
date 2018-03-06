# Arch-Project-List
## How to Run (Project1)
* make
* ./sim.exe [-flags] [.dump] [> result_file.txt]
  * [-flags] = -[i,r,m,d] (For more info on the flags, please refer to CSE140_Project1.pdf)
  * [.dump] = any dump file with instructions encoded. sample.dump is given.
  * [> result_file.txt] = pipeline output
  * Ex: ./sim sample.dump > result.txt

## Test Files (Project1)
* lui.dump: tests for correctness of lui instruction
* lwTest1.dump: tests for correctness of lw instruction and checks for a case where the memory address is below the data memory range (less than 0x00401000).
* lwTest2.dump: checks for a case where the memory address is above the data memory range (above 0x00403fff).
* lwTest3.dump: checks for a case where the memory address is not divisible by 4.
* noMemoryTest.dump: tests for correctness for all functions except sw/lw/lui.
* swTest1.dump: tests for correctness of sw instruction and checks for a case where the memory address is below the data memory range (less than 0x00401000).
* swTest2.dump: checks for a case where the memory address is above the data memory range (above 0x00403fff).
* swTest3.dump: checks for a case where the memory address is not divisible by 4.
* .s files: code description for .dump files above.

## How to Create Test Files (Project1)
1) Use MARS MIPS Interpreter to create the test script.
2) Assemble the code.
3) Choose dump machine code to create .dump file (saved as Binary) and .s file (saved as Text/Data Segment Window).