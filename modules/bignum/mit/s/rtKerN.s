	.globl	.oVncs
	.globl	_BnnSetToZero
	.globl	_.BnnSetToZero
	.data
	.align 2
_BnnSetToZero:
	.long	_.BnnSetToZero
	.text
	.align 2
_.BnnSetToZero:
	get	r0,$_bzero
	get	r4,$_.bzero
	brx	r4		# call bzero
	sli	r3,2		# convert words to bytes
	.long	0xdf02df00

	.globl	_BnnAssign
	.globl	_.BnnAssign

	.data
	.align	2

_BnnAssign:
	.long	_BnnAssign

	.text
	.align	2

_.BnnAssign:
	get	r5,$_.blt
	get	r0,$_blt
	brx	r5
	sli	r4,2		# convert bytes to words
	.long	0xdf02df00

	.globl	_BnnSetDigit
	.globl	_.BnnSetDigit

	.data
	.align	2
_BnnSetDigit:
	.long	_.BnnSetDigit

	.text
_.BnnSetDigit:
	brx	r15
	sts	r3,0(r2)
	.long	0xdf02df00

	.globl	_BnnGetDigit
	.globl	_.BnnGetDigit

	.data
	.align	2
_BnnGetDigit:
	.long	_.BnnGetDigit

	.text
_.BnnGetDigit:
	brx	r15
	ls	r2,0(r2)
	.long	0xdf02df00

	.globl	_BnnNumDigits
	.globl	_.BnnNumDigits

	.data
	.align	2
_BnnNumDigits:
	.long	_.BnnNumDigits

	.text
	.align	2
	.byte	0,0		# word align loop
_.BnnNumDigits:
	mr	r5,r3
	sli	r5,2
	a	r2,r5		# point past end of number
0:
	sis	r3,1
	jm	1f		# test nl-- > 0
	l	r0,-4(r2)	# test *--nn
	dec	r2,4
	cis	r0,0
	jeq	0b
	inc	r3,1		# undo decrement
	brx	r15
	mr	r2,r3
1:
	brx	r15
	lis	r2,1
	.long	0xdf02df00

# BnnNumLeadingZeroBitsInDigit (BigNumDigit) 

	.globl	_BnnNumLeadingZeroBitsInDigit
	.globl	_.BnnNumLeadingZeroBitsInDigit

	.data
	.align	2
_BnnNumLeadingZeroBitsInDigit:
	.long	_.BnnNumLeadingZeroBitsInDigit
	.text
_.BnnNumLeadingZeroBitsInDigit:
	srpi16	r2,0		# r3:16 contains r2:0
	clz	r4,r3
	ci	r4,16
	jeq	0f
	brx	r15
	mr	r2,r4
0:
	clz	r2,r2
	brx	r15
	ai	r2,r2,16

	.long	0xdf02df00


	.globl	_BnnDoesDigitFitInWord
	.globl	_.BnnDoesDigitFitInWord
	.data
	.align	2
_BnnDoesDigitFitInWord:
	.long	_.BnnDoesDigitFitInWord
	.text
_.BnnDoesDigitFitInWord:
	brx	r15
	lis	r2,1
	.long	0xdf02df00

	.globl	_BnnIsDigitZero
	.globl	_.BnnIsDigitZero
	.data
	.align	2
_BnnIsDigitZero:
	.long	_.BnnIsDigitZero
	.text
_.BnnIsDigitZero:
	cis	r2,0
	beqrx	r15
	cal	r2,1(r0)
	brx	r15
	cal	r2,0(r0)

	.long	0xdf02df00

# Boolean BnnIsDigitNormalized (BigNumDigit) 

	.globl	_BnnIsDigitNormalized
	.globl	_.BnnIsDigitNormalized
	.data
	.align	2
_BnnIsDigitNormalized:
	.long	_.BnnIsDigitNormalized
	.text
_.BnnIsDigitNormalized:
	mttbiu	r2,0
	lis	r2,0
	brx	r15
	mftbil	r2,15
	.long	0xdf02df00

	.globl	_BnnIsDigitOdd
	.globl	_.BnnIsDigitOdd
	.data
	.align	2
_BnnIsDigitOdd:
	.long	_.BnnIsDigitOdd
	.text
_.BnnIsDigitOdd:
	brx	r15
	nilz	r2,r2,1
	.long	0xdf02df00

# BigNumCmp BnnCompareDigits(BigNumDigit, BigNumDigit)
	.globl	_BnnCompareDigits
	.globl	_.BnnCompareDigits
	.data
	.align	2
_BnnCompareDigits:	.long	_.BnnCompareDigits
	.text
_.BnnCompareDigits:
	cl	r2,r3
	jh	1f
	jeq	2f
	brx	r15
	cal	r2,-1(r0)
1:
	brx	r15
	cal	r2,1(r0)
2:
	brx	r15
	cal	r2,0(r0)

	.long	0xdf02df00
	

# void BnnComplement (BigNum, int)

	.globl	_BnnComplement
	.globl	_.BnnComplement
	.data
	.align	2
_BnnComplement:
	.long	_.BnnComplement
	.text
_.BnnComplement:
	j	1f
0:
	ls	r4,0(r2)
	inc	r2,4
	onec	r4,r4
	st	r4,-4(r2)
1:
	sis	r3,1
	jnm	0b
	br	r15
	.long	0xdf02df00

# void BnnAndDigits(BigNum, BigNumDigits)

	.globl	_BnnAndDigits
	.globl	_.BnnAndDigits
	.data
	.align	2
_BnnAndDigits:
	.long	_.BnnAndDigits
	.text
_.BnnAndDigits:
	ls	r4,0(r2)
	n	r4,r3
	brx	r15
	sts	r4,0(r2)
	.long	0xdf02df00

	.globl	_BnnOrDigits
	.globl	_.BnnOrDigits
	.data
_BnnOrDigits:
	.long	_.BnnOrDigits
	.text
_.BnnOrDigits:
	ls	r4,0(r2)
	o	r4,r3
	brx	r15
	sts	r4,0(r2)
	.long	0xdf02df00

	.globl	_BnnXorDigits
	.globl	_.BnnXorDigits
	.data
	.align	2
_BnnXorDigits:
	.long	_.BnnXorDigits
	.text
_.BnnXorDigits:
	ls	r4,0(r2)
	x	r4,r3
	brx	r15
	sts	r4,0(r2)
	.long	0xdf02df00

# Shift left (r4) bits, with carry over as needed

	.globl	_BnnShiftLeft
	.globl	_.BnnShiftLeft
	.data
	.align	2
_BnnShiftLeft:
	.long	_.BnnShiftLeft
	.text
_.BnnShiftLeft:
	stm	r14,-8(r1)
	lis	r0,0
	cis	r4,0
	jeq	2f
	cis	r3,0
	jeq	2f
	twoc	r5,r4
	ai	r5,32		# 32 - shift count

0:
	ls	r14,0(r2)
	inc	r2,4		# be useful during load delay
	slp	r14,r4		# *mm << nbits
	o	r15,r0		# (*mm << nbits) | res
	st	r15,-4(r2)	# r2 already incremented
	srp	r14,r5		# (*mm >> lnbits)
	sis	r3,1		# --ml
	bhx	0b		# > 0
	mr	r0,r15		# res = (*mm >> rnbits)

2:
	lm	r14,-8(r1)
	brx	r15
	mr	r2,r0
	.long	0xdf02df00

# Shift right (r4) bits, with carry over as needed

	.globl	_BnnShiftRight
	.globl	_.BnnShiftRight
	.data
	.align	2
_BnnShiftRight:
	.long	_.BnnShiftRight
	.text
_.BnnShiftRight:
	stm	r14,-8(r1)
	lis	r0,0
	cis	r4,0
	jeq	2f
	cis	r3,0
	jeq	2f
	twoc	r5,r4
	ai	r5,32		# 32 - shift count
	mr	r15,r3
	sli	r15,2
	a	r2,r15		# point mm past end of list

0:
	dec	r2,4
	ls	r14,0(r2)
	srp	r14,r4
	o	r15,r0
	sts	r15,0(r2)
	slp	r14,r5
	sis	r3,1
	bhx	0b
	mr	r0,r15

2:
	lm	r14,-8(r1)
	brx	r15
	mr	r2,r0
	.long	0xdf02df00


# BigNumCarry BnnAddCarry (BigNum, int, BigNumCarry) 

	.globl	_BnnAddCarry
	.globl	_.BnnAddCarry
	.data
	.align	2
_BnnAddCarry:	.long	_.BnnAddCarry
	.text
_.BnnAddCarry:
	cis	r4,0
	jeq	3f
0:				# only come here as long as carry set
	sis	r3,1
	jm	4f
	ls	r0,0(r2)
	inc	r2,4
	ais	r0,1
	bc0x	0b
	st	r0,-4(r2)
3:
	brx	r15
	lis	r2,0
4:	brx	r15
	lis	r2,1
	
	.long	0xdf02df00	


# BigNumCarry BnnAdd (BigNum, int, BigNum, int, BigNumCarry) 

	.globl	_BnnAdd
	.globl	_.BnnAdd
	.data
	.align	2
_BnnAdd:	.long	_.BnnAdd
	.text
_.BnnAdd:
	stm	r14,-8(r1)
	s	r3,r5		# sizeof(M) - sizeof(N)
	ls	r0,0(r1)	# r0 = carry
# in loop:
# r5 = count, r0 = carry, r14 = *M, r15 = *N
# r2 = M, r4 = M

0:
	sis	r5,1
	jm	4f
	ls	r15,0(r4)
	ls	r14,0(r2)
	inc	r4,4
	a	r14,r15
	bc0x	2f
	a	r14,r0
	jc0	2f
	lis	r0,0		# no carry
1:
	sts	r14,0(r2)
	bx	0b
	inc	r2,4
2:
	bx	1b
	lis	r0,1		# carry next iteration
4:	# add carry (r0) to rest of M [BnnAddCarry inlined]
	cis	r0,0
	jeq	6f
5:
	sis	r3,1
	jm	7f
	ls	r14,0(r2)
	inc	r2,4
	ais	r14,1
	bc0x	5b
	st	r14,-4(r2)
6:
	lm	r14,-8(r1)
	brx	r15
	lis	r2,0
7:
	lm	r14,-8(r1)
	brx	r15
	lis	r2,1

	.long	0xdf02df00


# BigNumCarry BnnSubtractBorrow (BigNum, int, BigNumCarry) 

	.globl	_BnnSubtractBorrow
	.globl	_.BnnSubtractBorrow
	.data
	.align	2
_BnnSubtractBorrow:	.long	_.BnnSubtractBorrow
	.text
_.BnnSubtractBorrow:
	cis	r4,1
	jeq	4f
0:
	sis	r3,1
	jm	3f
	ls	r0,0(r2)
	inc	r2,4
	sis	r0,1
	bnc0x	0b		# c0 clear -> borrow
	st	r0,-4(r2)
4:
	brx	r15
	lis	r2,1
3:
	brx	r15
	lis	r2,0

	.long	0xdf02df00


# BigNumCarry BnnSubtract (BigNum, int, BigNum, int, BigNumCarry)
	.globl	_BnnSubtract
	.globl	_.BnnSubtract
	.data
	.align	2
_BnnSubtract:	.long	_.BnnSubtract
	.text
_.BnnSubtract:
	stm	r14,-8(r1)
	ls	r0,0(r1)	# r0 = carry
	s	r3,r5		# sizeof(M) - sizeof(N)
	xil	r0,r0,1		# 0 -> 1, 1 -> 0
0:
	sis	r5,1
	jm	4f
	ls	r15,0(r4)
	ls	r14,0(r2)
	inc	r4,4
	s	r14,r15
	bnc0x	1f		# subtract sets c0 if no borrow
	s	r14,r0
	jnc0	1f
	sts	r14,0(r2)
	lis	r0,0
	bx	0b
	inc	r2,4
1:
	sts	r14,0(r2)
	inc	r2,4
	bx	0b
	lis	r0,1

4:
	lm	r14,-8(r1)
	bx	_.BnnSubtractBorrow
	xil	r4,r0,1

	.long	0xdf02df00

# BigNumCarry BnnMultiplyDigit (BigNum, int, BigNum, int, BigNumDigit)
# (P, M, d)
# P = P + M * d
# sizeof(P) > sizeof(M)

	.globl	_BnnMultiplyDigit
	.globl	_.BnnMultiplyDigit
	.data
	.align	2
_BnnMultiplyDigit:	.long	_.BnnMultiplyDigit
	.text
_.BnnMultiplyDigit:

# register allocation:
# r0 = mulitplier (d)
# r2,r3 = P
# r4,r5 = M
# r14 = digit of M
# r15 = digit of P
# r12 = carry
# r13 = scratch

# step 1: handle simple cases (d == 0 || d == 1)
	ls	r0,0(r1)
	cis	r0,0
	jeq	0f	# return 0
	cis	r0,1
	jne	1f
	lis	r0,0
	bx	_.BnnAdd
	sts	r0,0(r1)
0:
	brx	r15
	lis	r2,0
1:
	stm	r12,-16(r1)
	lis	r12,0		# initialize carry to 0
	s	r3,r5		# amount by which P is longer...
	dec	r3,1		# ...minus the minimum difference of 1
2:		
	sis	r5,1
	jm	6f
	ls	r14,0(r4)
	inc	r4,4
	s	r15,r15		# faster than setsb cs,c0
	mts	r10,r0

	m	r15,r14
	m	r15,r14
	m	r15,r14
	m	r15,r14

	m	r15,r14
	m	r15,r14
	m	r15,r14
	m	r15,r14

	m	r15,r14
	m	r15,r14
	m	r15,r14
	m	r15,r14

	m	r15,r14
	m	r15,r14
	m	r15,r14
	m	r15,r14

	bc0x	3f
	mfs	r10,r13
	a	r15,r14
3:
	cis	r14,0
	bnmx	4f
	ls	r14,0(r2)

	a	r15,r0

4:	# now, r15:r13 is the product, and (r12+r14) is to be added
	a	r14,r12		# set up r12:r14 as dest + carry
	lis	r12,0
	aei	r12,r12,0
	# now, do a standard 64 bit add r12:r14 += r15:r13
	a	r14,r13
	ae	r12,r15
	# r12 should be saved as next carry, and r14 stored back
	# ASSUME CARRY CLEAR HERE
	sts	r14,0(r2)
	bx	2b
	inc	r2,4
5:
	sis	r3,1
	jm	7f
6:				# go here after last digit of M
	ls	r15,0(r2)
	lis	r14,0
	a	r15,r12
	sts	r15,0(r2)	# sizeof(P) > sizeof(M), so this store is
	aei	r12,r14,0	# executed at least one
	bnex	5b
	inc	r2,4
				# should return carry = 0
	lm	r12,-16(r1)
	brx	r15
	lis	r2,0

7:				# should return carry = 1
	lm	r12,-16(r1)
	brx	r15
	lis	r2,1

	.long	0xdf02df00

abort:	tgte	r15,r15

# BigNumDigit BnnDivideDigit (BigNum, BigNum, int, BigNumDigit)

	.globl	_BnnDivideDigit
	.globl	_.BnnDivideDigit
	.data
	.align	2
_BnnDivideDigit:	.long	_.BnnDivideDigit
	.text
	.align	2
_.BnnDivideDigit:


################################################################
# assembly version
#
# (Q, N, d)
# Q = N / d
# returns N % d
# assumes d > (first word on N)


	stm	r13,-12(r1)
	mr	r0,r4
	dec	r0,1
	sli	r0,2		# byte size of N - sizeof(int)
	a	r2,r0		# &q[l-1]
	a	r3,r0		# &n[l-1]
	ls	r14,0(r3)
	cis	r5,0
	jm	slow

2:
	sis	r4,1
	jnh	ex
1:
	dec	r3,4
	ls	r0,0(r3)
	# divide r14:r0 [X] by r5 [d], r14 = rem, r0 = quot
	mts	r10,r0

	d	r14,r5
#	jo	6f
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5

	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5

	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5

	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5
	d	r14,r5

	bc0x	3f
	mfs	r10,r0		# r0 is quotient
	a	r14,r5
3:				# r14 is remainder
	dec	r2,4
	bx	2b
	sts	r0,0(r2)
ex:
	mr	r2,r14
	lm	r13,-12(r1)
	br	r15

slow:
2:	sis	r4,1
	jnh	ex
1:	dec	r3,4
	ls	r0,0(r3)
				# divide r14:r0 / r5, divisor high bit set
	cal	r15,32(r0)	# r0 = count
s0:	mr	r13,r14		# r13 sign bit is sign bit of previous value
	sli	r14,1		# r14:r0 <<= 1
	mttbiu	r0,0
	mftbil	r14,15
	sli	r0,1
				# compare high 33 bits of old r14:0 to r5
	cl	r14,r5		# high 32 bits of r14:r0 > divisor?
	jhe	s1		# if so, subtract
	cis	r13,0		# was MSB of dividend set?
	jm	s1		# subtract
	sis	r15,1
	jh	s0
	dec	r2,4
	bx	2b
	sts	r0,0(r2)
s1:
	s	r14,r5
	setbl	r0,15		# set LSB of quotient
	sis	r15,1
	jh	s0

s2:
	dec	r2,4
	bx	2b
	sts	r0,0(r2)

#6:	tgte	r15,r15

	.long	0xdf02df00
