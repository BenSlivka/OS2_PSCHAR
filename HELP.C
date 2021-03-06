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
    "浜様様様様様様様様様様様様様� Libraries 突様様様様様様様様様様様様様融",
    "�                                                                    �",
    "�            PID - Process ID of program                             �",
    "�           SGID - Screen Group ID of program                        �",
    "�           Name - Name of program module                            �",
    "�   Library List - Directed, Acyclic Graph of modules imported by    �",
    "�                  the program module.                               �",
    "�                                                                    �",
    "�                  * = indicates a cycle in the graph                �",
    "�                                                                    �",
    "藩様様様様様様様様様様様� Press Esc to return 突様様様様様様様様様様夕"
};
#define     H_EXPLIB	(sizeof(ExplainLibScreen)/sizeof(char *))

char *ExplainMemScreen[] = {
    "浜様様様様様様様様様様様様� Shared Memory 突様様様様様様様様様様様様融",
    "�                                                                    �",
    "�     owner hndl - Internal ID of shared memory segment              �",
    "�            sel - LDT selector of shared memory segment             �",
    "�            ref - Number of references to shared memory segment     �",
    "�           name - Name of shared memory segment                     �",
    "�                                                                    �",
    "藩様様様様様様様様様様様� Press Esc to return 突様様様様様様様様様様夕"
};
#define     H_EXPMEM	(sizeof(ExplainMemScreen)/sizeof(char *))

char *ExplainProcScreen[] = {
    "浜様様様様様様様様様様裕 Processes and Threads 突様様様様様様様様様様様�",
    "�                                                                      �",
    "�      PID - Process ID                   TID - Thread ID              �",
    "�     SGID - Screen Group ID              Pri - Thread priority        �",
    "�     Name - Name of program module                                    �",
    "�     Nest - Parent/child nesting level.  1 is top-level process.      �",
    "�    State - Thread state:                                             �",
    "�                zombie  - Waiting for parent to finish CWAIT          �",
    "�                ready   - Ready for CPU                               �",
    "�                block   - Waiting for I/O completion or a resource    �",
    "�                froze   - Waiting to come foreground (usually 3xBox)  �",
    "�                critsec - Blocked by other thread in DosEnterCritSec  �",
    "�                boost   - Just received I/O boost                     �",
    "�  BlockID - When state is 'block', ID of event being waited upon      �",
    "�                                                                      �",
    "藩様様様様様様様様様様様裕 Press Esc to return 突様様様様様様様様様様様�"
};
#define     H_EXPPROC	(sizeof(ExplainProcScreen)/sizeof(char *))

char *ExplainSemScreen[] = {
    "浜様様様様様様様様様様様裕 System Semaphores 突様様様様様様様様様様様様�",
    "�                                                                      �",
    "�    name,PID,TID - Name, PID, and TID of process owning semaphore     �",
    "�    flags/status - waiters      - thread(s) are waiting for sem       �",
    "�                   muxwaiters   - thread(s) are muxwaiting for sem    �",
    "�                   process died - process owning semaphore died       �",
    "�                   exclusive    - exclusive semaphore                 �",
    "�                   cleanup      - semaphore is being cleaned up       �",
    "�                   thread died  - thread owning semaphore died        �",
    "�                   exit owner   - exitlist thread owns semaphore      �",
    "�             ref - Count of processes referencing semaphore           �",
    "�            want - Count of processes waiting for semaphore           �",
    "�            hndl - Internal semaphore handle                          �",
    "�            name - System semaphore name                              �",
    "�                                                                      �",
    "藩様様様様様様様様様様様裕 Press Esc to return 突様様様様様様様様様様様�"
};
#define     H_EXPSEM	(sizeof(ExplainSemScreen)/sizeof(char *))


char *helpScreen[] = {
"浜様様様様様様僕様様様様様裕 Help 突様様様様様曜様様様様様様様�",
"� 01 Sep 1988 �         Process Status         � Version 1.13 �",
"麺様様様様様様瞥様様様様様様様様様様様様様様様擁様様様様様様様�",
"�                                                             �",
"�  Window Scrolling: \x18      - Scroll Window UP                �",
"�                    \x19      - Scroll Window DOWN              �",
"�                    PgUp   - Scroll Window UP one PAGE       �",
"�                    PgDn   - Scroll Window DOWN one PAGE     �",
"�                                                             �",
"�  Menu Selection:   \x1A      - Move Cursor RIGHT               �",
"�                    \x1B      - Move Cursor LEFT                �",
"�                    Enter  - Do cursor item                  �",
"�                    Space  - Do cursor item                  �",
"�                  <letter> - Do item starting with <letter>  �",
"�                                                             �",
"�  Use EXPLAIN to get explanation of screen data.             �",
"�                                                             �",
"�  Use -? flag to see command line flags.                     �",
"�                                                             �",
"藩様様様様様様様様様裕 Press Esc to return 突様様様様様様様様夕"
};
#define     H_HELP	(sizeof(helpScreen)/sizeof(char *))

char *optHelpScreen[] = {
    "浜様様様様様様様様裕 Options - HELP 突様様様様様様様様�",
    "�                                                     �",
    "�  Moving Between Fields:                             �",
    "�      \x18       - UP to previous field                 �",
    "�      \x19       - DOWN to next field                   �",
    "�                                                     �",
    "�  Field WITH cursor:                                 �",
    "�      \x1A       - Move cursor RIGHT                    �",
    "�      \x1B       - Move cursor LEFT                     �",
    "�      Del     - Delete character at cursor           �",
    "�      Home    - Move cursor to LEFT edge of field    �",
    "�      BS      - Delecte character to left of cursor  �",
    "�      <other> - Insert character, if valid           �",
    "�                                                     �",
    "�  Field WITHOUT cursor:                              �",
    "�      \x1A       - Change field to NEXT value           �",
    "�      \x1B       - Change field to PREVIOUS value       �",
    "�                                                     �",
    "藩様様様様様様様裕 Press Esc to return 突様様様様様様夕"
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
