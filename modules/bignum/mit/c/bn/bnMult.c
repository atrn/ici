/* Copyright     Digital Equipment Corporation & INRIA     1988, 1989 */
/* Last modified_on Fri Mar 30  4:13:47 GMT+2:00 1990 by shand */
/*      modified_on Mon Mar 26 18:09:25 GMT+2:00 1990 by herve */


/* bnMult.c: a piece of the bignum kernel written in C */


		/***************************************/

#define BNNMACROS_OFF
#include "BigNum.h"

                        /*** copyright ***/

static char copyright[]="@(#)bnMult.c: copyright Digital Equipment Corporation & INRIA 1988, 1989, 1990\n";

BigNumCarry (BnnMultiply) (BigNum pp, BigNumLength pl, BigNum mm, BigNumLength ml, BigNum nn, BigNumLength nl)

/*
 * Performs the product:
 *    Q = P + M * N
 *    BB = BBase(P)
 *    Q mod BB => P
 *    Q div BB => CarryOut
 *
 * Returns the CarryOut.  
 *
 * Assumes: 
 *    Size(P) >= Size(M) + Size(N), 
 *    Size(M) >= Size(N).
 */

{
   BigNumCarry c;

   /* Multiply one digit at a time */

   /* the following code give higher performance squaring.
   ** Unfortunately for small nl, procedure call overheads kills it
   */
#ifndef mips
    /* Squaring code provoke a mips optimizer bug in V1.31 */
   if (mm == nn && ml == nl && nl > 6)
   {
       int n_prev = 0;
       /* special case of squaring */
       for (c = 0; nl > 0; )
       {
           register BigNumDigit n = *nn;
           c += BnnMultiplyDigit(pp, pl, nn, 1, n);
           if (n_prev)
               c += BnnAdd(pp, pl, nn, 1, 0);
           nl--, nn++;
           pp += 2, pl -= 2;
           c += BnnMultiplyDigit(pp-1, pl+1, nn, nl, n+n+n_prev);
           n_prev = ((long) n) < 0;
       }
   }
   else
#endif /* mips */
       for (c = 0; nl-- > 0; pp++, nn++, pl--)
          c += BnnMultiplyDigit (pp, pl, mm, ml, *nn);

   return c;
}
