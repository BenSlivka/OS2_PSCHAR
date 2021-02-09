/*  asrt.h - assertion checking support
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Initial version
*	24-Aug-1988 bws Tag assertion stuff separately from DEBUG
*
*   +	Every data structure must have a signature as first member.
*   +	Signatures MUST be unique over all structures.
*   +	BAD_SIG is a reserved signature.
*   +	When freeing a structure, blast the signature field with BAD_SIG.
*   +	Put an AssertXXX prior to dereferencing any pointer.
*   +	Signatures in records and private Assert definitions should only
*	be generated if CheckAsserts is defined.
*/


#ifdef	CheckAsserts

#define BAD_SIG 	0x0000	/* invalid signature for ALL structures */

typedef unsigned int	word;
typedef unsigned int	sig_t;	/* data structure signature */

#define AssertNull(p)	 doAssertSame((word)(p),(word)NULL,__FILE__,__LINE__)
#define AssertSame(p,q)  doAssertSame((word)(p),(word)(q),__FILE__,__LINE__)
#define AssertNotNull(p) doAssertNotSame((word)(p),(word)NULL,__FILE__,__LINE__)
#define AssertNotSame(p,q) doAssertNotSame((word)(p),(word)(q),__FILE__,__LINE__)

extern void pascal doAssertPtr (char *ptr, sig_t sig, char *file, int line);
extern void pascal doAssertSame (word v1, word v2, char *file, int line);
extern void pascal doAssertNotSame (word v1, word v2, char *file, int line);

#else	/* turn off assertions */

#define AssertNull(p)
#define AssertSame(p,q)
#define AssertNotNull(p)
#define AssertNotSame(p,q)

#endif


/*  Check return code
*
*   Unlike other assertion checking, which is used to debug a program,
*   crc is used to check the return code from API calls.
*
*   Since we do not trust the system, crc is ALWAYS done.
*/
#define crc(a)	doCrc((a),__FILE__,__LINE__)
