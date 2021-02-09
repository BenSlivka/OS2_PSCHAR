/***	vio.h - VIO definitions
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Add this header
*/

typedef struct {
    int 	    md_length;	/* length of mode data structure */
    unsigned char   md_type;	/* 0 => text mode,  1 => graphics mode	  */
    unsigned char   md_color;	/* # of bits of color			  */
    int 	    md_cols;	/* Number of text columns/graphics pixels */
    int 	    md_rows;	/* Number of text rows/graphics pixels	  */
    unsigned int    md_gcols;	/* horizontal graphics pels */
    unsigned int    md_grows;	/* vertical graphics pels */
    unsigned int    md_modepels; /* char columns + 100h * rows */
} ModeData_s;

#define     MODE_TEXT	    0
#define     MODE_GRAPHICS   1

#define     LEN_MODE_TEXT_DATA	(sizeof(ModeData_s))


typedef struct {
    int 	    cd_start;	/* Starting scan line			  */
    int 	    cd_end;	/* Ending scan line			  */
    int 	    cd_width;	/* Width (always 1, ignored)		  */
    int 	    cd_attr;	/* Attribute (-1 => hidden, else ignored) */
} CursorData_s;

typedef struct {
    unsigned a_fore   : 3;
    unsigned a_bright : 1;
    unsigned a_back   : 3;
    unsigned a_blink  : 1;
} Attr_s;

typedef unsigned char Attr;

typedef struct {
    unsigned char   ch; 	/* character */
    Attr	    at; 	/* attribute */
} Cell_s;

typedef char *		    NPC;
typedef int  *		    NPI;
typedef long *		    NPL;
typedef unsigned *	    NPU;
typedef unsigned long *     NPUL;

typedef char far *	    FPC;
typedef int  far *	    FPI;
typedef long far *	    FPL;
typedef unsigned far *	    FPU;
typedef unsigned long far * FPUL;
typedef unsigned char far * FPB;

typedef Cell_s far *	    FPCe;
typedef Attr far *	    FPAt;


/*
**  VioGetCurPos     (&Row, &Column, Handle)
**  VioGetMode	     (&ModeData, Handle)
**  VioReadCellStr   (&Buffer, &Length, Row, Column, Handle)
**  VioScrollDown    (TRow, LCol, BRow, RCol, Lines, &Cell, Handle)
**  VioScrollLeft    (TRow, LCol, BRow, RCol, Columns, &Cell, Handle)
**  VioScrollRight   (TRow, LCol, BRow, RCol, Columns, &Cell, Handle)
**  VioScrollUp      (TRow, LCol, BRow, RCol, Lines, &Cell, Handle)
**  VioSetCurPos     (Row, Column, Handle)
**  VioSetCurType    (&CursorData, Handle)
**  VioSetMode	     (&ModeData, Handle)
**  VioWrtCellStr    (&Buffer, Length, Row, Column, Handle)
**  VioWrtCharStr    (&Buffer, Length, Row, Column, Handle)
**  VioWrtCharStrAtt (&Buffer, Length, Row, Column, &Attrib, Handle)
**  VioWrtNAttr      (&Attrib, Times, Row, Column, Handle)
**  VioWrtNCell      (&Cell, Times, Row, Column, Handle)
**  VioWrtNChar      (&Char, Times, Row, Column, Handle)
*/

short far pascal VioGetCurPos	  (FPI,FPI,int);
short far pascal VioGetMode	  (ModeData_s far *, int);
short far pascal VioReadCellStr   (FPCe,FPI,int,int,int);
short far pascal VioScrollDn	  (int,int,int,int,int,FPCe,int);
short far pascal VioScrollLf	  (int,int,int,int,int,FPCe,int);
short far pascal VioScrollRt	  (int,int,int,int,int,FPCe,int);
short far pascal VioScrollUp	  (int,int,int,int,int,FPCe,int);
short far pascal VioSetCurPos	  (int,int,int);
short far pascal VioSetCurType	  (CursorData_s far *, int);
short far pascal VioSetMode	  (ModeData_s far *, int);
short far pascal VioWrtCellStr	  (FPCe,int,int,int,int);
short far pascal VioWrtCharStr	  (FPC,int,int,int,int);
short far pascal VioWrtCharStrAtt (FPC,int,int,int,FPAt,int);
short far pascal VioWrtNAttr	  (FPAt,int,int,int,int);
short far pascal VioWrtNCell	  (FPCe,int,int,int,int);
short far pascal VioWrtNChar	  (FPC,int,int,int,int);


/***	Attribute definitions	*/

#define     black	0
#define     blue	1
#define     green	2
#define     cyan	3
#define     red 	4
#define     magenta	5
#define     yellow	6
#define     white	7
#define     bright	8
#define     blinking	8

/***	attr - compute a character attribute
*
*	ENTRY:	f = foreground attribute
*		b = backround attribute
*
*	EXIT:	returns combined attribute
*/

#define     attr(f,b)  (((b)<<4)+(f))
