 Address    Code        Basic                     Source

# store 2 at 0x00401000
0x00400000  0x3c100040  lui $16,0x00000040    2    lui	$s0, 0x0040
0x00400004  0x36101000  ori $16,$16,0x000010003    ori	$s0, $s0, 0x1000
0x00400008  0x24080002  addiu $8,$0,0x000000024    addiu	$t0, $0, 2
0x0040000c  0xae080000  sw $8,0x00000000($16) 5    sw	$t0, 0($s0)

# load 2 from 0x00401000
0x00400010  0x8e090000  lw $9,0x00000000($16) 8    lw	$t1, 0($s0)
0x00400014  0x11280001  beq $9,$8,0x00000001  9    beq	$t1, $t0, NoExit

# should not visit here
0x00400018  0x200b0000  addi $11,$0,0x0000000012   addi	$t3, $0, 0

# load from 0x00400FFF (should print an error message)
# NoExit:
0x0040001c  0x3c100040  lui $16,0x00000040    16   lui	$s0, 0x0040
0x00400020  0x36100fff  ori $16,$16,0x00000fff17   ori	$s0, $s0, 0x0FFF
0x00400024  0x8e090000  lw $9,0x00000000($16) 18   lw	$t1, 0($s0)
