/*  menu.c - display menu and handle commands
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Use keyboard queue for keystrokes
*	08-Oct-1987 bws Change quit logic to account for signals
*/

#include <stddef.h>
#include <malloc.h>

#include "asrt.h"   /* assertion checking */
#include "vio.h"
#include "ps.h"
#include "os2.h"
#include "queue.h"
#include "vars.h"

#include "os2def.h"

typedef struct {
    char  *miKeys;		/* key(s) which activate menu item */
    char  *miName;		/* menu name */
    void (*miFunc)(int);	/* menu function */
    int    miCol;		/* menu column */
    int    miLen;		/* length of menu name */
} menu_t;

extern void	mQuit (int);
extern void	mMemory (int);
extern void	mLibrary (int);
extern void	mSemaphore (int);
extern void	mProcess (int);
extern void	mExplain (int);
extern void	mOption (int);
extern void	mUpdate (int);
extern char    *PopUpStart (void);
extern void	PopUpEnd (char *);
extern void	showHelp (int);
extern void	showExplain ();

extern qu_t    *pQu;		/* ptr to keyboard queue */
extern BOOL	fKilled;	/* TRUE if killed by signal */

menu_t menu[] = {
	{"hH",	"Help",     showHelp,	0,  0},
	{"eE",	"Explain",  mExplain,	0,  0},
	{"oO",	"Options",  mOption,	0,  0},
	{"qQ",	"Quit",     mQuit,	0,  0},
	{"lL",	"Library",  mLibrary,	0,  0},
	{"mM",	"Memory",   mMemory,	0,  0},
	{"pP",	"Process",  mProcess,	0,  0},
	{"sS",	"Semaphore",mSemaphore, 0,  0},
	{"uU",	"Update",   mUpdate,	0,  0}
};
#define nMenu	 (sizeof(menu)/sizeof(menu_t))

#define     miHelp	0	/* These must match order of menu[] */
#define     miExplain	1
#define     miOption	2
#define     miQuit	3
#define     miLibrary	4
#define     miMemory	5
#define     miProcess	6
#define     miSemaphore 7


int lastStatus=-1;  /* index marked menu item */
int lastMenu;	    /* index of highlighted menu item */
int m_row;	    /* menu row */
int m_col;	    /* menu column (leftmost) */
int fAlive=TRUE;    /* TRUE while we are to continue running */

#define     MENU_SEPARATION	2   /* amount of space between menu items */


/***	menuInit - initialize menu data
*
*/
menuInit ()
{
    int i;
    int col;

    m_row = N_of_Rows - 1;
    m_col = MENU_SEPARATION;
    col = m_col;
    for (i=0; i<nMenu; i++) {
	menu[i].miCol = col;
	menu[i].miLen = strlen(menu[i].miName);
	col += menu[i].miLen + MENU_SEPARATION;
    }
}


/***	drawMenu - display menu
*
*/
drawMenu ()
{
    Attr    a;
    Cell_s  c;
    int     i;

    a = color[menuC];		/*  Clear menu area */
    c.ch = ' ';
    c.at = color[menuC];
    VioWrtNCell((FPCe)&c, N_of_Cols, m_row, 0, VioHandle);

    for (i=0; i<nMenu; i++)	/*  Draw menu items */
	drawCursor (i, a);
}


/***	doMenu - Handle menu keystrokes
*
*/
doMenu ()
{
    int 	row,col;
    word	key;
    int 	i;
    Attr	a;
    int 	iMenu;
    int 	changeData;	/* TRUE if data display has changed */

    drawMenu();

    iMenu = 6;	 /* Start with PROCESS display */
    lastMenu = iMenu;
    (*(menu[iMenu].miFunc))(iMenu); /* do Process function */

    while (fAlive) {
#ifdef	DEBUG
	showFreeSpace();
#endif
	updateCursor (iMenu);
	changeData = FALSE;
	do {
	    if (!fKilled)   /* catch kill from help/option screens */
		key = (word)QueueReadTO(pQu,refreshPeriod);
	    if (fKilled)
		key = quitKey;	/* Change kill into quit command */
	    else if (key == NULL)
		mUpdate(0);	/* update screen */
	    else
		;		/* exit loop */
	} while (key == NULL);
	switch (key) {
	    case rightKey:
		if (++iMenu > (nMenu - 1))
		    iMenu = 0;
		break;
	    case leftKey:
		if (--iMenu < 0)
		    iMenu = nMenu - 1;
		break;
	    case upKey:
		doScroll (-1);
		break;
	    case downKey:
		doScroll (1);
		break;
	    case pgUpKey:
		doScroll (-dataRows);
		break;
	    case pgDownKey:
		doScroll (dataRows);
		break;
	    case helpKey:
		showHelp (miHelp);
		break;
	    default:
		if ((i = findKey(key)) != -1) {
		    iMenu = i;
		    changeData = TRUE;
		}
		break;
	    case enterKey:
	    case spaceKey:
		changeData = TRUE;
		break;
	}   /* switch */
	if (changeData) {
	    updateCursor (iMenu);
	    (*(menu[iMenu].miFunc))(iMenu);
	}
    }	/* while */
}


/***	updateCursor - Move cursor to "i"th menu item
*
*/
updateCursor (i)
int i;
{
    if (i != lastMenu)		    /* user moved to different field */
	drawCursor (lastMenu, color[menuC]); /* revert to normal color */
    drawCursor (i, color[cursorC]); /* highlight current field */
    lastMenu = i;
}


/***	markMenu - Move mark to "i"th menu item
*
*/
markMenu (i)
int i;
{
    Attr    a;

    a=color[menuC];

    if (lastStatus != -1)
	VioWrtCharStrAtt ((FPC)" ", 1, m_row, menu[lastStatus].miCol-1,
			  (FPAt)&a, VioHandle);
    VioWrtCharStrAtt ((FPC)"*", 1, m_row, menu[i].miCol-1,(FPAt)&a, VioHandle);
    lastStatus=i;
}


/***	findKey - Map key to menu index, if any
*
*/
findKey (key)
unsigned key;
{
    int     i;
    int     found=-1;
    char    k=key>>8;

    if (k != 0) /* Not IBM extended code, so scan */
	for (i=0; i<nMenu && found==-1; i++)
	    if (strchr(menu[i].miKeys,k))
		found=i;
    return found;
}


/***	drawCursor - draw individual menu item
*
*/
drawCursor (i, color)
int	i;
Attr	color;
{
    VioWrtCharStrAtt ((FPC)menu[i].miName, menu[i].miLen, m_row,
		      menu[i].miCol, (FPAt)&color, VioHandle);
}


/***	mQuit - do QUIT menu option
*
*/
void mQuit (i)
int i;
{
    fAlive=FALSE;
}


/***	mProcess - do PROCESS menu option
*
*/
void mProcess (i)
int i;
{
    markMenu(i);
    doScreen(iProcess);
}


/***	mLibrary - do LIBRARY menu option
*
*/
void mLibrary (i)
int i;
{
    markMenu(i);
    doScreen(iLibrary);
}


/***	mSemaphore - do SEMAPHORE menu option
*
*/
void mSemaphore (i)
int i;
{
    markMenu(i);
    doScreen(iSemaphore);
}


/***	mMemory - do MEMORY menu option
*
*/
void mMemory (i)
int i;
{
    markMenu(i);
    doScreen(iMemory);
}


/***	mExplain - do EXPLAIN menu option
*
*/
void mExplain (i)
int i;
{
    switch (lastStatus) {   /* map menu index to table index */
	case miLibrary:     i = iLibrary;   break;
	case miMemory:	    i = iMemory;    break;
	case miProcess:     i = iProcess;   break;
	case miSemaphore:   i = iSemaphore; break;
	default:
	    Fatal ("invalid menu index");
    }
    showExplain(i); /* show explanation screen */
}


/***	mOption - do OPTION menu option
*
*/
void mOption (i)
int i;
{
    char    *buf;

    if (buf=PopUpStart()) {
	doOption();
	PopUpEnd(buf);
    }
}


/***	mUpdate - do UPDATE menu option
*
*/
void mUpdate (i)
int i;
{
    freeImages();
    buildDatabase();
    updateScreen();
}
