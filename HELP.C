/*  help.c - constant help screens
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	18-Aug-1987 bws Use keyboard queue for keystrokes
*	08-Oct-1987 bws Signal handling modifications
*	19-Oct-1987 bws Added delay time to command line
*	20-Oct-1987 bws Fix usage display
*	19-Feb-1988 bws Add explanation screens
*	27-Feb-1988 bws Improved SysSem explanation screen
*	01-Sep-1988 bws Rev number for FMT speedup
*/

#include <stddef.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#include "asrt.h"   /* assertion checking */
#include "vio.h"
#include "os2.h"
#include "queue.h"
#include "ps.h"
#include "vars.h"

#include "os2def.h"

extern qu_t    *pQu;		/* ptr to keyboard queue */

char *ExplainLibScreen[] = {
    "ษอออออออออออออออออออออออออออต Libraries ฦออออออออออออออออออออออออออออป",
    "บ                                                                    บ",
    "บ            PID - Process ID of program                             บ",
    "บ           SGID - Screen Group ID of program                        บ",
    "บ           Name - Name of program module                            บ",
    "บ   Library List - Directed, Acyclic Graph of modules imported by    บ",
    "บ                  the program module.                               บ",
    "บ                                                                    บ",
    "บ                  * = indicates a cycle in the graph                บ",
    "บ                                                                    บ",
    "ศอออออออออออออออออออออออต Press Esc to return ฦออออออออออออออออออออออผ"
};
#define     H_EXPLIB	(sizeof(ExplainLibScreen)/sizeof(char *))

char *ExplainMemScreen[] = {
    "ษอออออออออออออออออออออออออต Shared Memory ฦออออออออออออออออออออออออออป",
    "บ                                                                    บ",
    "บ     owner hndl - Internal ID of shared memory segment              บ",
    "บ            sel - LDT selector of shared memory segment             บ",
    "บ            ref - Number of references to shared memory segment     บ",
    "บ           name - Name of shared memory segment                     บ",
    "บ                                                                    บ",
    "ศอออออออออออออออออออออออต Press Esc to return ฦออออออออออออออออออออออผ"
};
#define     H_EXPMEM	(sizeof(ExplainMemScreen)/sizeof(char *))

char *ExplainProcScreen[] = {
    "ษออออออออออออออออออออออต Processes and Threads ฦอออออออออออออออออออออออป",
    "บ                                                                      บ",
    "บ      PID - Process ID                   TID - Thread ID              บ",
    "บ     SGID - Screen Group ID              Pri - Thread priority        บ",
    "บ     Name - Name of program module                                    บ",
    "บ     Nest - Parent/child nesting level.  1 is top-level process.      บ",
    "บ    State - Thread state:                                             บ",
    "บ                zombie  - Waiting for parent to finish CWAIT          บ",
    "บ                ready   - Ready for CPU                               บ",
    "บ                block   - Waiting for I/O completion or a resource    บ",
    "บ                froze   - Waiting to come foreground (usually 3xBox)  บ",
    "บ                critsec - Blocked by other thread in DosEnterCritSec  บ",
    "บ                boost   - Just received I/O boost                     บ",
    "บ  BlockID - When state is 'block', ID of event being waited upon      บ",
    "บ                                                                      บ",
    "ศออออออออออออออออออออออออต Press Esc to return ฦอออออออออออออออออออออออผ"
};
#define     H_EXPPROC	(sizeof(ExplainProcScreen)/sizeof(char *))

char *ExplainSemScreen[] = {
    "ษออออออออออออออออออออออออต System Semaphores ฦอออออออออออออออออออออออออป",
    "บ                                                                      บ",
    "บ    name,PID,TID - Name, PID, and TID of process owning semaphore     บ",
    "บ    flags/status - waiters      - thread(s) are waiting for sem       บ",
    "บ                   muxwaiters   - thread(s) are muxwaiting for sem    บ",
    "บ                   process died - process owning semaphore died       บ",
    "บ                   exclusive    - exclusive semaphore                 บ",
    "บ                   cleanup      - semaphore is being cleaned up       บ",
    "บ                   thread died  - thread owning semaphore died        บ",
    "บ                   exit owner   - exitlist thread owns semaphore      บ",
    "บ             ref - Count of processes referencing semaphore           บ",
    "บ            want - Count of processes waiting for semaphore           บ",
    "บ            hndl - Internal semaphore handle                          บ",
    "บ            name - System semaphore name                              บ",
    "บ                                                                      บ",
    "ศออออออออออออออออออออออออต Press Esc to return ฦอออออออออออออออออออออออผ"
};
#define     H_EXPSEM	(sizeof(ExplainSemScreen)/sizeof(char *))


char *helpScreen[] = {
"ษอออออออออออออหออออออออออออต Help ฦออออออออออออหออออออออออออออป",
"บ 01 Sep 1988 บ         Process Status         บ Version 1.13 บ",
"ฬอออออออออออออสออออออออออออออออออออออออออออออออสออออออออออออออน",
"บ                                                             บ",
"บ  Window Scrolling: \x18      - Scroll Window UP                บ",
"บ                    \x19      - Scroll Window DOWN              บ",
"บ                    PgUp   - Scroll Window UP one PAGE       บ",
"บ                    PgDn   - Scroll Window DOWN one PAGE     บ",
"บ                                                             บ",
"บ  Menu Selection:   \x1A      - Move Cursor RIGHT               บ",
"บ                    \x1B      - Move Cursor LEFT                บ",
"บ                    Enter  - Do cursor item                  บ",
"บ                    Space  - Do cursor item                  บ",
"บ                  <letter> - Do item starting with <letter>  บ",
"บ                                                             บ",
"บ  Use EXPLAIN to get explanation of screen data.             บ",
"บ                                                             บ",
"บ  Use -? flag to see command line flags.                     บ",
"บ                                                             บ",
"ศออออออออออออออออออออต Press Esc to return ฦออออออออออออออออออผ"
};
#define     H_HELP	(sizeof(helpScreen)/sizeof(char *))

char *optHelpScreen[] = {
    "ษออออออออออออออออออต Options - HELP ฦอออออออออออออออออป",
    "บ                                                     บ",
    "บ  Moving Between Fields:                             บ",
    "บ      \x18       - UP to previous field                 บ",
    "บ      \x19       - DOWN to next field                   บ",
    "บ                                                     บ",
    "บ  Field WITH cursor:                                 บ",
    "บ      \x1A       - Move cursor RIGHT                    บ",
    "บ      \x1B       - Move cursor LEFT                     บ",
    "บ      Del     - Delete character at cursor           บ",
    "บ      Home    - Move cursor to LEFT edge of field    บ",
    "บ      BS      - Delecte character to left of cursor  บ",
    "บ      <other> - Insert character, if valid           บ",
    "บ                                                     บ",
    "บ  Field WITHOUT cursor:                              บ",
    "บ      \x1A       - Change field to NEXT value           บ",
    "บ      \x1B       - Change field to PREVIOUS value       บ",
    "บ                                                     บ",
    "ศออออออออออออออออต Press Esc to return ฦออออออออออออออผ"
};
#define     H_OPTION	(sizeof(optHelpScreen)/sizeof(char *))


typedef struct {
    int     scNScreen;	    /* Number of rows in screen */
    int     scWidth;	    /* Width of screen in columns */
    char  **scScreen;	    /* Screen screen */
} screen_t;

screen_t screen[] = {
    {H_EXPLIB,	    0,	ExplainLibScreen},
    {H_EXPMEM,	    0,	ExplainMemScreen},
    {H_EXPPROC,     0,	ExplainProcScreen},
    {H_EXPSEM,	    0,	ExplainSemScreen},
    {H_HELP,	    0,	helpScreen},
    {H_OPTION,	    0,	optHelpScreen}
};
#define nScreen  (sizeof(screen)/sizeof(screen_t))

#define iExpLib     0	    /* index of library explanation screen */
#define iExpMem     1	    /* index of memory explanation screen */
#define iExpProc    2	    /* index of process explanation screen */
#define iExpSem     3	    /* index of semaphore explanation screen */
#define iHelp	    4	    /* index of main help screen */
#define iOption     5	    /* index of option help screen */


/***	helpInit - initialize help control structures
*
*/
helpInit ()
{
    int i;

    for (i=0; i<nScreen; i++)
	screen[i].scWidth = strlen(screen[i].scScreen[0]);
}


/***	PopUpStart - Prepare for "pop-up"
*
*	We allocate buffer large enough for screen memory, then copy
*	screen memory to buffer.  This allows for the "appearance" of
*	a "pop-up", since we can write to the screen, then later copy
*	this saved image back, thus "bringing down" the "pop-up".  This
*	is not to be confused with VioPopUp.  PopUpEnd does the restore
*	and frees the buffer.
*/
char *PopUpStart ()
{
    unsigned	size;
    char       *buf;

    size = N_of_Rows*N_of_Cols*2;
    if (buf = malloc(size))
	VioReadCellStr ((FPCe)buf, (FPI)&size, 0, 0, VioHandle);
    return buf;
}


/***	PopUpEnd - Restore screen after pop-up
*
*/
PopUpEnd (buf)
char *buf;
{
    int     size;

    size = N_of_Rows*N_of_Cols*2;
    VioWrtCellStr ((FPCe)buf, size, 0, 0, VioHandle);
    free(buf);
}


/***	doPopUp - Do pop-up processing for specified text "window"
*
*	EXIT	FALSE, normal exit
*		TRUE, kill requested
*/
BOOL doPopUp (i)
int i;
{
    int     height,width;
    int     row,col;
    int     hRow,hCol;
    Attr    a;
    char    *buf;
    word    key;
    BOOL    fDie=FALSE;

    if (buf=PopUpStart()) {
	height = screen[i].scNScreen;
	width = screen[i].scWidth;

	hRow = (N_of_Rows - height)/2;
	hCol = (N_of_Cols - width)/2;

	a = color[helpC];
	for (row=0; row<height; row++)
	    VioWrtCharStrAtt ((FPC)screen[i].scScreen[row], width,
			      row+hRow,hCol, (FPAt)&a, VioHandle);
	do {
	    key = (word)QueueRead(pQu);
	    if (key == NULL) {
		key = escKey;	/* Change kill into exit command */
		fDie = TRUE;
	    }
	} while (key != escKey);    /* wait for Esc key */

	PopUpEnd(buf);
    }
    return (fDie);
}


/***	showHelp - pop-up help window
*
*/
void showHelp (i)
int i;
{
    doPopUp(iHelp);
}


/***	showExplain - pop-up an explanation window
*
*/
void showExplain (i)
int i;
{
    doPopUp(i);
}


/***	helpOption - pop-up help window for Option window
*
*	EXIT	FALSE, normal exit
*		TRUE, kill requested
*/
BOOL helpOption (i)
int i;
{
    return ( doPopUp(iOption) );
}
