/*  ps.h  - definitions for process status
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Add this header
*	24-Aug-1988 bws Tag assertion stuff separately from DEBUG
*/

#define     FALSE   0
#define     TRUE    1

#define     VioHandle	0

#define     MAX_SCREEN_WIDTH	  90

#define     enterKey	0x0D00
#define     upKey	0x0048
#define     pgUpKey	0x0049
#define     downKey	0x0050
#define     pgDownKey	0x0051
#define     leftKey	0x004B
#define     rightKey	0x004D
#define     quitKey	0x7100	    /* q */
#define     escKey	0x1B00
#define     helpKey	0x003B	    /* F1 */
#define     spaceKey	0x2000
#define     updateKey	0x7500	    /* u */

#define     delKey	0x0053
#define     bsKey	0x0800
#define     homeKey	0x0047
#define     endKey	0x004F

#define     borderC	    0	    /* index in color array */
#define     titleC	    1
#define     headerC	    2
#define     menuC	    3
#define     cursorC	    4
#define     helpC	    5
#define     dataC	    6
#define     optionC	    7
#define     ofieldC	    8
#define     ocursorC	    9
#define     debugC	   10

#define iLibrary    0
#define iMemory     1
#define iProcess    2
#define iSemaphore  3
#define N_IMAGES    4	    /* number of image screens */

typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long dword;


/***	farp(fp,off) - construct far pointer from far pointer and offset */

#define     farp(fp,off)   ( ((dword)(fp) & 0xFFFF0000) | (word)(off) )


/*  record to link together lines of text */
typedef struct line_s {
#ifdef	CheckAsserts
    sig_t	    li_dbgSig; /* signature for debugging */
#endif
    char	   *li_str;    /* line image */
    struct line_s  *li_next;   /* ptr to next line */
} line_t;

#ifdef	CheckAsserts

#define LI_SIG	    0x494C  /* "LI" */
#define AssertLI(p) doAssertPtr ((char *)(p),LI_SIG,__FILE__,__LINE__)

#else

#define AssertLI(p)	    /* turn off assertion check */

#endif
