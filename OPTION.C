/*  option.c - Display option screen
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Get keystrokes from keyboard queue
*	19-Oct-1987 bws Added delay time to command line
*	13-Feb-1988 bws Change acceptance key to Enter from Esc
*/

#include <stdio.h>
#include <math.h>

#include "asrt.h"   /* assertion checking */
#include "ps.h"
#include "os2.h"
#include "queue.h"
#include "vio.h"
#include "vars.h"

#define Z_ALPHA     0x01
#define Z_DIGIT     0x02
#define Z_WHITE     0x04
#define Z_PATH	    0x08
#define Z_PHONE     0x10
#define Z_FLOAT     0x20

extern qu_t    *pQu;		/* ptr to keyboard queue */

/*  ต = 181, ฦ = 198 */
char *optScreen[] = {
   /* 123456789 123456789 123456789 123456789 123456789 123456789 1 */
    "ษอออออออออออออออออออออออออต Options ฦออออออออออออออออออออออออป", /*  0 */
    "บ                                                            บ", /*  1 */
    "บ  Auto Refresh:      yes                                    บ", /*  2 */
    "บ  Refresh Interval:  999.999 seconds                        บ", /*  3 */
    "บ                                                            บ", /*  4 */
    "บ                                                            บ",
    "บ                                                            บ",
    "บ                      Press F1 for Help                     บ",
    "บ                                                            บ",
    "บ                                                            บ",
    "ศออออออออออออออออออต Press Enter to return ฦอออออออออออออออออผ"
};
#define     O_HEIGHT	(sizeof(optScreen)/sizeof(char *))

#define     rowRefr	2
#define     colRefr    22

#define     rowIntv	3
#define     colIntv    22
#define     wIntv	7
#define     sIntv      (Z_FLOAT)

char strIntv[wIntv+1] = "       ";
#define     lenIntv   wIntv


int O_ROW;	/* Coordinates of top left corner of option menu */
int O_COL;
int O_WIDTH;	/* Width of help screen */

char *csRefr[] = {
    "no ",
    "yes"
};
#define nRefr	(sizeof(csRefr)/sizeof(char*))
#define wRefr	3


#define OT_SET	    0		/* Option types */
#define OT_ASCIIZ   1

unsigned char cMap[256];	/* map character to character type */

typedef struct {
    int sc_n;			/* number of different values */
    char **sc_name;		/* array of value names */
} setControl_t;

typedef struct {
    unsigned char az_Set;	/* flags of valid characters */
} azControl_t;

typedef struct {
    int ctl_type;		/* Type of control */
    int ctl_row;		/* row to display value name */
    int ctl_col;		/* column to display value name */
    int ctl_width;		/* width of value names */
    union {
	setControl_t  sc;
	azControl_t   az;
    } ctl_union;
} control_t;

typedef union {
    struct {
	int   oaz_len;
	char *oaz_str;
	int   oaz_cur;		/* cursor position */
    } oaz;
    int   osc;
} option_t;

control_t  control[] = {
    {OT_SET   ,rowRefr	,colRefr  ,wRefr  ,{nRefr  ,csRefr  }},
    {OT_ASCIIZ,rowIntv	,colIntv  ,wIntv  ,{sIntv	   }},
};

option_t option[] = {		/* Current option settings */
/* refr      */    {1}, 	/* Refresh ON by default */
/* intv      */    {lenIntv,strIntv,0},
};
#define     nOption    (sizeof(option)/sizeof(option_t))

#define     io_fRefresh     0
#define     io_interval     1


/***	SetRefreshPeriod - set refresh interval
*
*/
SetRefreshPeriod()
{
    if (option[io_fRefresh].osc)    /* refresh desired */
	refreshPeriod = atof(strIntv)*1000.0;  /* interval in milliseconds */
    else
	refreshPeriod = SEM_INFINITE;	/* refresh period is forever */
}


/***	optionInit - initialize option data
*
*/
optionInit ()
{
    int i;

    O_WIDTH = strlen(optScreen[0]);
    O_ROW = (N_of_Rows - O_HEIGHT)/2;
    O_COL = (N_of_Cols - O_WIDTH)/2;
    for (i=0; i<nOption; i++) {        /* Adjust to real screen coordinates */
	control[i].ctl_row += O_ROW;
	control[i].ctl_col += O_COL;
    }
    initCMap();
    sprintf(strIntv,"%7.3f",((float)refreshPeriod)/1000.0);
    SetRefreshPeriod();
}   /* optionInit */


/***	initCMap - initialize character class mapping
*
*/
initCMap ()
{
    register int i;

    for (i=0; i<256; i++)
	cMap[i] = 0;
    for (i='a'; i<='z'; i++)
	cMap[i] |= Z_ALPHA;
    for (i='A'; i<='Z'; i++)
	cMap[i] |= Z_ALPHA;
    for (i='0'; i<='9'; i++)
	cMap[i] |= Z_DIGIT | Z_FLOAT;
    cMap[':']  |= Z_PATH;
    cMap['\\'] |= Z_PATH;
    cMap['/']  |= Z_PATH;
    cMap['.']  |= Z_PATH | Z_FLOAT;
    cMap['-']  |= Z_PATH | Z_PHONE;
    cMap[' ']  |= Z_WHITE;
    cMap['\t'] |= Z_WHITE;
    cMap[',']  |= Z_PHONE;
}   /* initCMap */


/***	doOption - display and manage option window
*
*/
doOption ()
{
    int 	row,col;
    unsigned	key;
    int 	i;
    Attr	a;
    int 	iOpt,lastOpt;

    a = color[optionC];
    for (row=0; row<O_HEIGHT; row++)	    /* Display option panel */
	VioWrtCharStrAtt ((FPC)optScreen[row], O_WIDTH, row+O_ROW,O_COL,
			  (FPAt)&a, VioHandle);

    for (i=0; i<nOption; i++)	   /* Fill in options */
	showOpt (i,color[ofieldC]);

    key = 0;
    iOpt = 0;	/* Start with first option */
    lastOpt = iOpt;
    while (key != escKey) {
	if (iOpt != lastOpt)	    /* user moved to different field */
	    showOpt (lastOpt, color[ofieldC]); /* revert to normal color */
	showOpt (iOpt, color[ocursorC]); /* highlight current field */
	lastOpt = iOpt;
	if (key != escKey) {
	    key = (word)QueueRead(pQu);     /* get keystroke */
	    if (key == enterKey)    /* Treat Enter same as Esc */
		key = escKey;
	    if (key == NULL)
		key = escKey;	    /* Change kill into exit command */
	}
	switch (key) {
	    case upKey:
		if (--iOpt < 0)
		    iOpt = nOption - 1;
		break;
	    case downKey:
		if (++iOpt > (nOption - 1))
		    iOpt = 0;
		break;
	    case rightKey:
		modOption (iOpt,  1);
		break;
	    case leftKey:
		modOption (iOpt, -1);
		break;
	    case delKey:
		if (control[iOpt].ctl_type == OT_ASCIIZ)
		    doRightDel (iOpt);
		break;
	    case homeKey:
		if (control[iOpt].ctl_type == OT_ASCIIZ)
		    option[iOpt].oaz.oaz_cur = 0;
		break;
	    case bsKey:
		if (control[iOpt].ctl_type == OT_ASCIIZ)
		    doLeftDel (iOpt);
		break;
	    case helpKey:
		HideCursor();
		if (helpOption())
		    key = escKey;   /* dying */
		break;
	    default:
		switch (control[iOpt].ctl_type) {
		    case OT_SET:    /* Ignore other key strokes */
			break;
		    case OT_ASCIIZ:
			doInsert (iOpt,key);
			break;
		    default:
			Fatal ("doOption: invalid option type");
		}
		break;
	}   /* switch */
    }	/* while */
    HideCursor();
    SetRefreshPeriod();
}   /* doOption */


/***	doInsert - handle a charater insertion
*
*/
doInsert (iOpt,key)
int iOpt;
unsigned key;
{
    int   ch;
    int   cur;
    int   i,n;
    int   width;
    char *s,*d,*p;

    ch = key >> 8;
    if (ch > 0x7F)	/* Ignore */
	return;
    if ((cMap[ch] & control[iOpt].ctl_union.az.az_Set) == 0)
	return; 	/* Not valid char */

    /*	oaz_cur - points to insertion point
	We throw away characters at end of string.

	before: aaaaaaabbbbbbbbbbbbc
		       ^
	after:	aaaaaaaxbbbbbbbbbbbb
			^
    */
    cur = option[iOpt].oaz.oaz_cur;
    width = control[iOpt].ctl_width;
    p = option[iOpt].oaz.oaz_str;   /* value buffer */

    d = p+width-1;	/* End of string */
    s = d-1;		/* End of string - 1 */
    n = (width-cur)-1;	/* Number of chars to shift */
    for (i=n; i>0; i--)
	*d-- = *s--;	/* Shift string */
    p[cur] = ch;	/* Insert character */
    cur++;		/* Advance insertion point */
    if (cur >= width)	/* cursor wrapped */
	cur = 0;
    option[iOpt].oaz.oaz_cur = cur;
}


/***	doLeftDel - delete char to left of cursor
*
*/
doLeftDel (iOpt)
int iOpt;
{
    if (option[iOpt].oaz.oaz_cur == 0)	/* Ignore at start of field */
	return;
    /*
	We delete the character to the left of the cursor and shift
	rest of characters left 1.  Ignore at beginning of field.

	before: aaaaaaaxbbbbbbbbbbbb
			^
	after:	aaaaaaabbbbbbbbbbbb
		       ^
    */
    doDel (iOpt,0,-1);
}


/***	doRightDel - delete char under cursor
*
*/
doRightDel (iOpt)
int iOpt;
{
    /*
	We delete the character at the cursor and shift
	rest of characters left 1.

	before: aaaaaaaxbbbbbbbbbbbb
		       ^
	after:	aaaaaaabbbbbbbbbbbb
		       ^
    */
    doDel (iOpt,1,0);
}


/***	doDel - common character deletion code
*
*/
doDel (iOpt,iShift,iCur)
int iOpt;
int iShift;
int iCur;
{
    int cur,width,i,n;
    char *s,*d,*p;

    cur = option[iOpt].oaz.oaz_cur;
    width = control[iOpt].ctl_width;
    p = option[iOpt].oaz.oaz_str;

    s = p+cur+iShift;
    d = s-1;
    n = (width-(cur+iCur))-1;	/* Number of chars to shift */
    for (i=n; i>0; i--)
	*d++ = *s++;	/* Shift string */
    cur += iCur;	/* Adjust cursor */
    p[width-1] = ' ';	/* Put blank at end */
    option[iOpt].oaz.oaz_cur = cur;
}


/***	modOption - modify a "list" field
*
*/
modOption (iOpt, inc)
int iOpt,inc;
{
    int col;
    int width;

    switch (control[iOpt].ctl_type) {
	case OT_SET:	/* Ignore other key strokes */
	    if (inc > 0) {  /* Increment value */
		if (++option[iOpt].osc > (control[iOpt].ctl_union.sc.sc_n - 1))
		    option[iOpt].osc = 0;
	    }
	    else	    /* Decrement value */
		if (--option[iOpt].osc < 0 )
		    option[iOpt].osc = control[iOpt].ctl_union.sc.sc_n - 1;
	    break;
	case OT_ASCIIZ:
	    col = option[iOpt].oaz.oaz_cur;
	    width = control[iOpt].ctl_width;
	    if (inc > 0) {  /* move cursor right */
		if (++col >= width)
		    col = 0;
	    }
	    else	    /* move cursor left */
		if (--col < 0 )
		    col = width-1;
	    option[iOpt].oaz.oaz_cur = col;
	    break;
	default:
	    Fatal ("modOption: invalid option type");
    }
}


/***	showOpt - display value of a field
*
*/
showOpt (iOpt, color)
int	iOpt;
Attr	color;
{
    int  row,col,width;
    int  curCol;
    char *name;

    row = control[iOpt].ctl_row;
    col = control[iOpt].ctl_col;
    width = control[iOpt].ctl_width;
    switch (control[iOpt].ctl_type) {
	case OT_SET:
	    name = control[iOpt].ctl_union.sc.sc_name[option[iOpt].osc];
	    HideCursor();
	    break;
	case OT_ASCIIZ:
	    name = option[iOpt].oaz.oaz_str;
	    curCol = col+option[iOpt].oaz.oaz_cur;
	    VioSetCurPos (row, curCol, VioHandle);
	    ShowCursor();
	    break;
	default:
	    Fatal ("showOpt: invalid option type");
    };
    VioWrtCharStrAtt ((FPC)name, width, row, col, (FPAt)&color, VioHandle);
}
