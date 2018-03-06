 Address    Code        Basic                     Source

# store 2 at 0x00404000 (should print an error message)
0x00400000  0x3c100040  lui $16,0x00000040    2    lui	$s0, 0x0040
0x00400004  0x36104000  ori $16,$16,0x000040003    ori	$s0, $s0, 0x4000
0x00400008  0x2408000a  addiu $8,$0,0x0000000a4    addiu	$t0, $0, 10
0x0040000c  0xae080000  sw $8,0x00000000($16) 5    sw	$t0, 0($s0)
