 Address    Code        Basic                     Source

0x00400000  0x3c101234  lui $16,0x00001234    1    lui $s0, 0x1234		# $s0 = 0x12340000
0x00400004  0x36105678  ori $16,$16,0x000056782    ori $s0, $s0, 0x5678	# $s0 = 0x12345678
0x00400008  0x20000000  addi $0,$0,0x00000000 4    addi $zero, $zero, 0	# exits (this function isn't supported)
