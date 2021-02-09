/*  asrt.c - assertion checking
*
*   Author:
*	Benjamin W. Slivka
*
*   History:
*	17-Aug-1987 bws Initial version
*	24-Aug-1988 bws Tag assertion stuff separately from DEBUG
*/

#define INCL_DOS    1	/* get DOS API equates */

#include <stddef.h>
#include "asrt.h"   /* assertion checking */
#include "os2def.h"
#include "bsedos.h"


#define FailAssert()  doFailAssert()

/***	doFailAssert - common exit for assertion failure
*
*	ENTRY	none
*
*	EXIT	issue message and exit
*/
void	doFailAssert()
{
    printf ("  Assertion Failure.\n");
    DosExit (1,2);	/* kill all threads, return rc=2 */
}


/***	doCrc - return code assertion checking
*
*	Unlike other assertion checking, which is used to debug a program,
*	doCrc is used to check the return code from API calls.	Hence, it
*	is ALWAYS done, since we do not trust the system.
*/
doCrc (rc,file,line)
int	rc;
char   *file;
int	line;
{
    if (rc != 0) {
	printf ("%s:(%d) call failed; rc=%d\n", file, line, rc);
	FailAssert();
    }
}

#ifdef	CheckAsserts

/***	doAssertPtr - verify ptr to correct type by checking signature
*
*	ENTRY	ptr  - ptr to data record
*		sig  - expected signature
*		file - name of source file
*		line - source line number
*
*	EXIT	if ptr != NULL and signature present, NONE
*		if ptr invalid, print message and exit
*
*	WARNING Signature must be FIRST in data structure!
*/
void pascal doAssertPtr (ptr, sig, file, line)
char   *ptr;
sig_t	sig;
char   *file;
int	line;
{
    if (ptr == NULL) {
	printf ("%s:(%d) null ptr\n", file, line);
	FailAssert();
    }
    else if (*((sig_t *)ptr) != sig) {
	printf ("%s:(%d) signature mismatch\n", file, line);
	FailAssert();
    }
}


/***	doAssertSame - verify values match
*
*	ENTRY	v1   - first value
*		v2   - second value
*		file - name of source file
*		line - source line number
*
*	EXIT	if v1 == v2, NONE
*		if v1 != v2, print message and exit
*/
void pascal doAssertSame (v1, v2, file, line)
word	v1,v2;
char   *file;
int	line;
{
    if (v1 != v2) {
	printf ("%s:(%d) values do not match: %04x != %04x\n",
		    file, line, v1, v2);
	FailAssert();
    }
}


/***	doAssertNotSame - verify values DO NOT match
*
*	ENTRY	v1   - first value
*		v2   - second value
*		file - name of source file
*		line - source line number
*
*	EXIT	if v1 != v2, NONE
*		if v1 == v2, print message and exit
*/
void pascal doAssertNotSame (v1, v2, file, line)
word	v1,v2;
char   *file;
int	line;
{
    if (v1 == v2) {
	printf ("%s:(%d) values match: %04x == %04x\n",
		    file, line, v1, v2);
	FailAssert();
    }
}

#endif
