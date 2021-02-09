/*  screen.c - screen stuff
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Modified assertion usage (more still needed!)
*	08-Oct-1987 bws Removed extra lines from screen image
*	09-Oct-1987 bws Display time
*	01-Sep-1988 bws Use fmt instead of sprintf
*/

#include <malloc.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>

#include "asrt.h"   /* assertion checking */
#include "vio.h"
#include "ps.h"
#include "vars.h"

#define     COL_MARGIN	    0	    /* Left and right data margin */
#define     ROW_TITLE	    0	    /* Row of title line starts (base 0) */
#define     ROW_HEADER	    1	    /* Row at which headers start (base 0) */
#define     ROW_DATA	    4	    /* Row at which data starts (base 0) */
#define     N_HDR_ROWS	    2	    /* Number of header rows */
#define     ROW_BOT_MARGIN  2	    /* Number of reserved rows at bottom */
#define     ROW_TIME	    (ROW_HEADER+N_HDR_ROWS-1) /* Row for time */

int	CurScreen;	/* Index of currently displayed screen */
char	achTime[MAX_SCREEN_WIDTH];  /* Time of last DosQProcStatus call */

char PrcTitle[] = "Process and Thread Status";
char *PrcHdr[] = {
"Nest  PID   SGID    Name",
"                TID   Pri   State    BlockID"
};
#define     N_HDR_PRC	(sizeof(PrcHdr)/sizeof(char *))


char LibTitle[] = "Library Status";
char *LibHdr[] = {
"PID   SGID    Name",
"                  Library List"
};
#define     N_HDR_LIB	(sizeof(LibHdr)/sizeof(char *))


char SemTitle[] = "Semaphore Status";
char *SemHdr[] = {
"ฺฤฤฤฤฤ owner ฤฤฤฤฤฤฟ",
"    name  PID   TID     status  ref   want  hndl  name"
		    /*123456789012*/
};
#define     N_HDR_SEM	(sizeof(SemHdr)/sizeof(char *))


char MemTitle[] = "Shared Memory Status";
char *MemHdr[] = {
"owner",
"hndl  sel   ref   name"
};
#define     N_HDR_MEM	(sizeof(MemHdr)/sizeof(char *))


typedef struct {
    char   *imTitle;	    /* Title of image */
    char  **imHeader;	    /* Column headers */
    int     imNHeader;	    /* Number of header rows */
    line_t *imImage;	    /* Screen image */
    int     imNImage;	    /* Number of rows in image */
    int     imCurRow;	    /* Image row at top of data region */
} image_t;

image_t image[N_IMAGES] = {
    {LibTitle, LibHdr, N_HDR_LIB, NULL, 0, 0},
    {MemTitle, MemHdr, N_HDR_MEM, NULL, 0, 0},
    {PrcTitle, PrcHdr, N_HDR_PRC, NULL, 0, 0},
    {SemTitle, SemHdr, N_HDR_SEM, NULL, 0, 0},
};
#define nImage	(sizeof(image)/sizeof(image_t))

char	szBlank[]=
"                                                                                ";
char	szBar[]=
"ออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ";


/***	paintScreen - Draw constant borders
*
*/
paintScreen()
{
    int     cchBlank;
    int     cchBar;
    int     row;
    Attr    a;

    hideCursor();
    clearScreen();

    a = color[borderC];
    cchBlank = strlen(szBlank);
    cchBar = strlen(szBar);

    dataRows = N_of_Rows-ROW_BOT_MARGIN-ROW_DATA;

    doLine (0, szBlank, cchBlank, a);
    doLine (1, szBlank, cchBlank, a);
    doLine (2, szBlank, cchBlank, a);
    doLine (3, szBar,	cchBar,   a);

    for (row=0; row<dataRows; row++)
	doLine (row+ROW_DATA, szBlank, cchBlank, a);

    doLine (N_of_Rows-ROW_BOT_MARGIN, szBar, cchBar, a);
}


/***	doLine - display one full-width line on screen
*
*/
doLine (row, line, len, a)
int row;
char *line;
int len;
Attr a;
{
    VioWrtCharStrAtt ((FPC)line, len, row, 0, (FPAt)&a, VioHandle);
}


/***	putLine - Draw line within borders
*
*/
putLine (row, line, a)
int row;
char *line;
Attr a;
{
    char	buf[MAX_SCREEN_WIDTH];
    int 	n;
    unsigned	width;
    char       *p;

    width = N_of_Cols - 2*COL_MARGIN;

    p = memccpy (buf, line, '\0', width);   /* p points after null */
    if (p)
	memset (p-1,' ',width - (p-buf-1)); /* blank-fill line */
    VioWrtCharStrAtt ((FPC)buf, width, row, COL_MARGIN, (FPAt)&a, VioHandle);
}


/***	DrawCenteredLine - Draw line centered within borders
*
*/
DrawCenteredLine (row, sz, a)
int row;
char *sz;
Attr a;
{
    char	ach[MAX_SCREEN_WIDTH];
    int 	n;
    unsigned	width;
    char       *p;
    int 	cch;

    width = N_of_Cols - 2*COL_MARGIN;
    cch = strlen(sz);	    /* length of string */
    memset (ach,' ',width); /* blank-fill line */
    memcpy (&ach[(width-cch)/2],sz,cch);   /* center string */
    VioWrtCharStrAtt ((FPC)ach, width, row, COL_MARGIN, (FPAt)&a, VioHandle);
}


/***	rowToPtr - map "row" to line ptr in data group "i"
*
*/
line_t	*rowToPtr (i, row)
int	i;
int	row;
{
    line_t  *t;

#ifdef DEBUG
    if (fDebug)
	printf ("rowToPtr: i=%d, row=%d\n", i, row);
#endif
    AssertLI(image[i].imImage);
    t = (image[i].imImage)->li_next;
    while (row--) {
	AssertLI(t);
	t = t->li_next;
    }
    AssertLI(t);
    return t;
}


/***	doScreen - display active screen data
*
*	Draw title, headers, and data.
*
*/
doScreen (i)
int i;
{
    int     row;
    Attr    a;

    a = color[titleC];
    DrawCenteredLine (ROW_TITLE, image[i].imTitle, a);
    a = color[headerC];
    for (row=0; row<N_HDR_ROWS; row++)	/* Draw header */
	putLine (row+ROW_HEADER, image[i].imHeader[row], a);
    doData(i);	/* Display data */
    CurScreen = i;	/* Remember index of current screen */
}


/***	doData - Draw data in data region
*
*/
doData (i)
int i;
{
    int     row;
    int     col;
    int     nRows;
    int     dRow;   /* beginning row for data */
    int     bRow;   /* beginning row for blanking */
    int     width;
    char    buf[MAX_SCREEN_WIDTH];
    Attr    a;
    Cell_s ce;
    line_t *li;

    ensureImage(i);	/* make sure image is generated */

    ce.ch = ' ';
    ce.at = color[dataC];

    width = N_of_Cols - 2*COL_MARGIN;

    dRow = image[i].imCurRow;	/* current DESIRED top image row */
    nRows = image[i].imNImage;	/* number of image rows */
    if (dRow >= nRows) {	/* image has shrunk below last current row */
	dRow = nRows-1; 	/* Set current row to last row */
	image[i].imCurRow = dRow; /* Remember new current row */
    }
    nRows -= dRow;		/* number of rows following current row */
    if (nRows > dataRows)	/* Only display one screenfull */
	nRows = dataRows;
    bRow = dRow+nRows;		/* bottom row of data */

    a = color[dataC];

    li = rowToPtr(i, dRow);
    for (row=dRow; row<(nRows+dRow); row++) {	/* Draw data */
	AssertLI(li);
	putLine ((row-dRow)+ROW_DATA, li->li_str, a);
	li = li->li_next;
    }
    for (row=bRow; row<(dataRows+dRow); row++)	/* Blank lines */
	VioWrtNCell((FPCe)&ce, width,
		    (row-dRow)+ROW_DATA, COL_MARGIN, VioHandle);

    fmt (buf, "Line %3d of %3d", dRow+1, image[i].imNImage);
    width = strlen(buf);
    col = N_of_Cols - width - COL_MARGIN;
    a = color[headerC];
    VioWrtCharStrAtt ((FPC)buf, width, ROW_TITLE, col, (FPAt)&a, VioHandle);
#ifdef	DEBUG
    showFreeSpace();
#endif
    ShowTime();
}


/***	doScroll - Scroll current data by "cScroll" rows
*
*/
doScroll (cScroll)
int cScroll;
{
    int     cRow;   /* current top row */
    int     lRow;   /* last row */
    int     i;	    /* screen index */

    i = CurScreen;  /* index of current screen */

    cRow = image[i].imCurRow;
    lRow = image[i].imNImage-1;

    cRow += cScroll;
    if (cRow < 0)
	cRow = 0;
    else if (cRow > lRow)
	cRow = lRow;

    image[i].imCurRow = cRow;
    doData(i);
}


/***	updateScreen - Redraw current data
*
*/
updateScreen()
{
    doData(CurScreen);
}


/***	printImages - Print all images to stdout
*
*/
printImages()
{
    int     i;
    int     j;
    line_t *l;

    for (i=0; i<N_IMAGES; i++) {
	if (i>0)    /* separate images with blank lines */
	    printf ("\n\n");
	for (j=0; j<N_HDR_ROWS; j++)	/* print header */
	    printf ("%s\n", image[i].imHeader[j]);

	printf ("\n");	/* separate data from header */
	AssertLI(image[i].imImage);
	l = (image[i].imImage)->li_next;
	while (l != NULL) {		/* print data */
	    AssertLI(l);
	    printf ("%s\n", l->li_str);
	    l = l->li_next;
	}
    }
}


/***	buildImages - build all images
*
*/
buildImages()
{
    int i;

    for (i=0; i<N_IMAGES; i++)
	ensureImage(i);
}


/***	ensureImage - generate image, if it does not exist
*
*/
ensureImage(i)
int i;
{
    if (image[i].imImage == NULL) { /* image not generated */
	switch (i) {
	    case iLibrary:
		imageLib  (&(image[i].imImage),&(image[i].imNImage));
		break;
	    case iMemory:
		imageMem  (&(image[i].imImage),&(image[i].imNImage));
		break;
	    case iProcess:
		imageProc (&(image[i].imImage),&(image[i].imNImage));
		break;
	    case iSemaphore:
		imageSem  (&(image[i].imImage),&(image[i].imNImage));
		break;
	    default:
		Fatal ("ensureImage: unknown image id");
	}
    }
}


/***	freeImages - Free all images
*
*/
freeImages()
{
    int i;

    for (i=0; i<N_IMAGES; i++)
	deallocateImage (&(image[i].imImage));
}


/***	deallocateImage - deallocate all space for an image
*
*/
deallocateImage (head)
line_t	  **head;	/* Start of screen image */
{
    line_t  *l=*head;
    line_t  *x;
    while (l != NULL) {
	AssertLI(l);
	AssertNotNull(l->li_str);
	free(l->li_str);    /* free string */
	x=l->li_next;
	free((NPC)l);	    /* free node */
	l=x;		    /* next node */
    }
    *head = NULL;	    /* mark list empty */
}


/***	HideCursor - hide hardware cursor
*
*/
HideCursor ()
{
    setCurType (-1);	/* hidden */
}


/***	ShowCursor - make hardware cursor visible
*
*/
ShowCursor ()
{
    setCurType (0);	/* visible */
}

/***	setCurType - set hardware cursor type
*
*/
setCurType (at)
int	at;
{
    CursorData_s CursorData;

    if (N_of_Rows > 25) {
	CursorData.cd_start = 6;
	CursorData.cd_end   = 7;
    }
    else {
	CursorData.cd_start = 0x0C;
	CursorData.cd_end   = 0x0D;
    }
    CursorData.cd_width = 0x01;
    CursorData.cd_attr	= at;
    VioSetCurType ((CursorData_s far *)&CursorData, VioHandle);
}


ModeData_s  OldModeData;    /* VIO mode data at start-up */

#define     oldCols	OldModeData.md_cols
#define     oldRows	OldModeData.md_rows


/***	screenInit - initialize the screen mode
*
*	We attempt to set 43 line mode, if asked.  If that fails, we
*	revert to current settings.
*/
screenInit ()
{
    ModeData_s	    md; /* Mode return data from VioGetMode */
    unsigned int    rc; /* return code */
    int 	    i;	/* index */

    OldModeData.md_length = sizeof(OldModeData);    /* Set size */
    crc( VioGetMode ((ModeData_s far *)&OldModeData, VioHandle) );

    memcpy ((NPC)&md,(NPC)&OldModeData,sizeof(ModeData_s));
    md.md_cols = 80;
    md.md_rows = (f43mode ? 43 : 25);
    rc = VioSetMode ((ModeData_s far *)&md, VioHandle);   /* set mode */
    crc( VioGetMode ((ModeData_s far *)&md, VioHandle) ); /* get result */
    N_of_Cols = md.md_cols;
    N_of_Rows = md.md_rows;

    /* If less than 3 bits of color, or B&W forced, use black and white */

    if ( (md.md_color < 3) || !fColor) {
	for (i=0; i<cColors; i++)
	    color[i] = attr(white,black);
	color[cursorC] = attr(black,white);
	color[ocursorC] = attr(black,white);
    }
}


/***	screenReset - Reset screen to original mode
*
*/
screenReset()
{
    int fClearBefore;

    fClearBefore = (oldRows <= N_of_Rows);

    if (fClearBefore)
	clearScreen();
    VioSetMode ((ModeData_s far *)&OldModeData, VioHandle);
    N_of_Rows = oldRows;	/* Reset global data */
    N_of_Cols = oldCols;
    if (!fClearBefore)
	clearScreen();
    showCursor();
}


/***	clearScreen - clear entire screen
*
*/
clearScreen ()
{
    Cell_s c;

    c.ch = ' ';
    c.at = attr(white,black);
    VioWrtNCell((FPCe)&c, N_of_Rows*N_of_Cols, 0, 0, VioHandle);

}


/***	ShowTime - Show time of last DosQProcStatus call
*
*/
ShowTime ()
{
    int     cch;
    Attr    a;
    int     col;

    cch = strlen(achTime);
    col = N_of_Cols - cch - COL_MARGIN;
    a = color[titleC];
    VioWrtCharStrAtt ((FPC)achTime, cch, ROW_TIME, col, (FPAt)&a, VioHandle);
}


#ifdef	DEBUG

#define N_BITS	15		/* Cannot have 64K allocation */
#define MAX_BLOCK   32768	/* 2**N_BITS */

/***	showFreeSpace - display size of free heap space
*
*/
showFreeSpace()
{
    char    *mem[N_BITS];
    int     i;
    word    size;
    word    avail;
    char    buf[MAX_SCREEN_WIDTH];
    int     width;
    int     col;
    Attr    a;

    size = MAX_BLOCK;
    avail = 0;
    for (i=0; i<N_BITS; i++) {
	if ((mem[i] = malloc(size)) != NULL)
	    avail += size;
	size >>= 1;	/* divide size by 2 */
    }

    for (i=0; i<N_BITS; i++)
	if (mem[i] != NULL)
	    free(mem[i]);

    fmt (buf, "free heap: %05u", avail);
    width = strlen(buf);
    col = N_of_Cols - width - COL_MARGIN;
    a = color[debugC];
    VioWrtCharStrAtt ((FPC)buf, width, ROW_HEADER, col, (FPAt)&a, VioHandle);
}
#endif
