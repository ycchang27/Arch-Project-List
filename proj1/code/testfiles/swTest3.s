 Address    Code        Basic                     Source

# store 2 at 0x00401003 (should print an error message)
0x00400000  0x3c100040  lui $16,0x00000040    2    lui	$s0, 0x0040
0x00400004  0x36101000  ori $16,$16,0x000010003    ori	$s0, $s0, 0x1000
0x00400008  0x24080002  addiu $8,$0,0x000000024    addiu	$t0, $0, 2
0x0040000c  0xae080003  sw $8,0x00000003($16) 5    sw	$t0, 3($s0)
