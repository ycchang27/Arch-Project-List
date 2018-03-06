 Address    Code        Basic                     Source

# set $t0 = 0x12345670
0x00400000  0x3c101234  lui $16,0x00001234    2    lui	$s0, 0x1234
0x00400004  0x36105670  ori $16,$16,0x000056703    ori	$s0, $s0, 0x5670

# decrease $t0 to 0x12340000
0x00400008  0x24080070  addiu $8,$0,0x000000706    addiu	$t0, $0, 0x70
0x0040000c  0x02088023  subu $16,$16,$8       7    subu	$s0, $s0, $t0
0x00400010  0x2610aa00  addiu $16,$16,0xffffaa8    addiu	$s0, $s0, -0x5600

# increment $t0 to 0x12345678
0x00400014  0x24090001  addiu $9,$0,0x0000000111   addiu	$t1, $0, 1
0x00400018  0x00094900  sll $9,$9,0x00000004  12   sll	$t1, $t1, 4
0x0040001c  0x00094842  srl $9,$9,0x00000001  13   srl	$t1, $t1, 1
0x00400020  0x02098021  addu $16,$16,$9       14   addu	$s0, $s0, $t1
0x00400024  0x24095670  addiu $9,$0,0x0000567015   addiu	$t1, $0, 0x5670
0x00400028  0x02098025  or $16,$16,$9         16   or	$s0, $s0, $t1

# reset $s0
0x0040002c  0x240a5678  addiu $10,$0,0x000056719   addiu	$t2, $0, 0x5678
0x00400030  0x020a8024  and $16,$16,$10       20   and	$s0, $s0, $t2
0x00400034  0x3c011234  lui $1,0x00001234     21   andi	$s0, $s0, 0x12340000
0x00400038  0x34210000  ori $1,$1,0x00000000       
0x0040003c  0x02018024  and $16,$16,$1             
0x00400040  0x240a0001  addiu $10,$0,0x000000022   addiu	$t2, $0, 1
0x00400044  0x0150582a  slt $11,$10,$16       23   slt	$t3, $t2, $s0

# branch test (Shouldn't visit Exit yet)
0x00400048  0x11600001  beq $11,$0,0x00000001 24   beq	$t3, $0, NoExit

# vising here would end the C MIPS Interpreter
# Exit:
0x0040004c  0x20080001  addi $8,$0,0x00000001 28   addi	$t0, $0, 1

# more branch test (Shouldn't visit Exit yet)
# NoExit:
0x00400050  0x240c0001  addiu $12,$0,0x000000031   addiu	$t4, $0, 1
0x00400054  0x1564fffd  bne $11,$4,0xfffffffd 32   bne	$t3, $4, Exit

# jump around and jump to Exit (jal->jr->j->Exit)
0x00400058  0x0c100018  jal 0x00400060        35   jal	MoreJump
0x0040005c  0x08100013  j 0x0040004c          36   j	Exit
# MoreJump:
0x00400060  0x03e00008  jr $31                39   jr	$ra
