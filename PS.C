/***	ps.c - Process Status
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	21-Apr-1987 bws Initial version.
*	29-Apr-1987 bws Converted to OS/2, use real API.
*	12-May-1987 bws Rewrote getKey to use KBDCALLS.
*	13-May-1987 bws Fixed null ptr bugs; added profile support
*	19-May-1987 bws Renamed to PS, support -f flag
*	26-May-1987 bws Add semaphore, shared memory reports
*	28-May-1987 bws Add slot to thread record
*	28-Jun-1987 bws Adjust priorities for better response
*	06-Aug-1987 bws Fiddle priorities again
*	17-Aug-1987 bws Add better assertion checking, use queue to pass
*			keystrokes from keyboard thread to main thread
*	08-Oct-1987 bws Add signal handlers.
*	19-Oct-1987 bws Added delay time to command line
*	13-Feb-1988 bws Add fColor flag
*	01-Sep-1988 bws Replace sprintf with fmt for speed
*
*   Overview:
*
*	PS consists of two threads:
*
*	    thread  responsibilities
*	    ------  -------------------------------------------------
*	       1    main thread; initialize, handle menus and pop-ups,
*		    get data from OS, image data, display data
*	       2    keyboard thread; pass keystrokes to main thread
*
*/

#define INCL_DOS    1	/* get DOS API equates */
#define INCL_SUB    1	/* get SUB API equates */

#include <stdio.h>
#include <math.h>

#include "asrt.h"
#include "ps.h"
#include "vars.h"
#include "os2.h"
#include "profile.h"
#include "queue.h"

#include "os2def.h"
#include "bsedos.h"
#include "bsesub.h"

#define SIGINTR 1
#define SIGTERM 3
#define SIGBREAK 4
#define CATCH_SIG 2
#define SIG_ACK 4

#define     STACK_SIZE	    1000	/* keyboard thread stack size */
#define     OUR_DUMP_FILE   "ps.pro"	/* profiling dump file name */

#define     MAX_PENDING_CMDS	10	/* max size of command queue */


int argc;	/* argc parm from main(,) */
char **argv;	/* argv parm from main(,) */

BOOL fKBDAlive=TRUE;	    /* TRUE ==> KBD thread should stay alive */
BOOL fKilled=FALSE;	    /* TRUE => main thread should kill process */
qu_t *pQu;		    /* ptr to keyboard command queue */


/***	cmdDestroy - destroy a queue data object
*
*	ENTRY	ptr - data value
*
*	EXIT	none
*
*	NOTE	our command passes data instead of a pointer, so this
*		routine does nothing.
*/
void	cmdDestroy (ptr)
char *ptr;
{
}


/***	main - main program
*
*/
main (c,v)
int	c;
char   *v[];
{
    char    stack[STACK_SIZE];	 /* Thread 2 stack; must be here for chkstk */

    parseCommandLine(c, v);
#ifdef PROFILE
    if (fProfile) {
	crc( PROFINIT(PT_USER) );
	crc( PROFON(PT_USER) );
    }
#endif
    buildDatabase();
    if (fFile) {
	buildImages();
	printImages();
    }
    else {
	SigInit();
	screenInit();		    /* Init the physical screen */
	optionInit ();
	menuInit();
	helpInit();
	paintScreen();
	pQu = QueueCreate(MAX_PENDING_CMDS,cmdDestroy);
	CreateKBDThread(stack, STACK_SIZE);
	doMenu();
	fKBDAlive = FALSE;	    /* tell KBD thread we are dying */
	QueueDestroy(pQu);
	screenReset();
    }
#ifdef PROFILE
    if (fProfile) {
	crc( PROFOFF(PT_USER) );
	crc( PROFDUMP(PT_USER, (FPC)OUR_DUMP_FILE) );
	crc( PROFFREE(PT_USER) );
    }
#endif
    flushall(); 	/* flush C run-time buffers */
    DosExit (1,0);	/* kill all threads, return success */
}   /* main */


/***	Usage - Issue message, display usage, then exit
*
*/
Usage (msg, parm)
char *msg;
char *parm;
{
    char *sz;

    if (strlen(parm) > 0)
	printf ("%s: %s -> %s\n", argv[0], msg, parm);
    else
	printf ("%s: %s\n", argv[0], msg);

#ifdef	PROFILE
#ifdef	DEBUG
    sz = "bdprx";
#else
    sz = "p";
#endif
#else
#ifdef	DEBUG
    sz = "bdrx";
#else
    sz = "";
#endif
#endif

    printf ("USAGE: %s [-f%s24] [delay]\n",argv[0],sz);
#ifdef DEBUG
    printf ("   -b : debug - use black & white\n");
    printf ("   -d : debug - trace certain events\n");
#endif
    printf ("   -f : print data on stdout\n");
#ifdef PROFILE
    printf ("   -p : profile this program\n");
#endif
#ifdef DEBUG
    printf ("   -r : debug - print OS data records on stdout\n");
    printf ("   -x : debug - dump internal database");
#endif
    printf ("   -2 : set screen to 25-line mode\n");
    printf ("   -4 : set screen to 43-line mode (default)\n");
    printf ("delay = refresh interval in seconds (0.3 is default)\n");
    exit(1);
}


/***	parseCommandLine - parse command line arguments
*
*/
parseCommandLine (c, v)
int c;
char *v[];
{
    char   *p;
    int     i;
    float   flt;

    argc=c;
    argv=v;
    for (i=1; i<c; i++) {
	if (argv[i][0] == '-') {    /* process options */
	    p = &(v[i][1]);
	    while (*p) {
		switch (*p) {
#ifdef DEBUG
		    case 'b':	fColor	 = FALSE; break;
		    case 'd':	fDebug	 = TRUE; break;
		    case 'r':	fRecord  = TRUE; break;
		    case 'x':	fXDebug  = TRUE; break;
#endif
#ifdef PROFILE
		    case 'p':	fProfile = TRUE; break;
#endif
		    case 'f':	fFile	 = TRUE; break;

		    case '2':
		    case '5':	f43mode  = FALSE; break;

		    case '4':
		    case '3':	f43mode = TRUE; break;

		    case '?':	Usage("options",""); break;
		    default:	Usage("unknown flag", argv[i]);
		}
		p++;
	    }
	}
	else if ((flt = atof(argv[i])) != 0.0)
	    refreshPeriod = (long)(flt*1000.0); /* interval in milliseconds */
	else
	    Usage("invalid parameter", argv[i]);
    }
}

#define     KBD_WAIT	0   /* wait for keystroke */
#define     KBD_HANDLE	0   /* keyboard handle */

/***	getKey - return two-byte key stroke; includes IBM extended ASCII codes
*
*	key = 0xCC00	if normal ASCII character
*	key = 0x00SS	if IBM extended character
*
*	NOTE: IBM 101-key keyboard returns:
*		00ss - special keys (cursor, home, end, ins, del, ...)
*		E0ss - special keys
*		cc00 - normal keys
*
*	      We return 00ss for BOTH 00ss AND E0ss.  This works for all
*	      keys tested.  Some future keyboard may not exhibit this
*	      behavior, but at that point PS will be a Presentation
*	      Manager application, so PM will hide this stuff!
*/
unsigned int getKey ()
{
    KBDKEYINFO	kd;

    crc( KbdCharIn (&kd, KBD_WAIT, KBD_HANDLE) );

    if ( (kd.chChar == 0) || (kd.chChar == 0xE0) )
	return	kd.chScan;
    else
	return	kd.chChar << 8;
}

#define     STD_OUT	1   /* handle of stdout */

/***	errOut - Put string on stdout without C runtime
*
*/
errOut (msg)
char *msg;
{
    word    len=0;
    char   *p=msg;

    while (*p++)
	len++;
    crc( DosWrite (STD_OUT, msg, len, &len) );
}


/***	Fatal - issue message for fatal error and exit program
*
*/
Fatal (msg)
char *msg;
{
    errOut ("FATAL ERROR: ");
    errOut (msg);
    errOut ("\r\n");
    DosExit (1,2);	/* kill all threads, return rc=2 */
}


/***	KBDThread - body of keyboard thread
*
*	The keyboard thread takes keystrokes and places them into the
*	keyboard queue for the main thread.
*
*	The main thread will kill us when the time comes, but it is possible
*	for the user to press a key during this time.  So, the main thread
*	resets fKBDAlive.  If we ever get control with this flag clear,
*	we block forever on a private RAM semaphore.  This allows the main
*	thread to do the DosExit and kill us.
*
*	WARNING Any and all code called by this thread MUST NOT use the
*		C run-time, as that code is NOT reentrant.
*/
void far KBDThread ()
{
    unsigned int    key;
    RAMSEM	    semWaitToDie;

    while (fKBDAlive) {
	key = getKey();
	AssertNotSame(key,0);	/* cannot pass 0 to QueueWrite */
	if (fKBDAlive) {    /* User may press key while we are dying */
	    QueueWrite (pQu, (char *)key); /* put key in queue */
	}
    }
    crc( DosSemSetWait((HSEM)&semWaitToDie,SEM_INFINITE) );
}


/***	CreateKBDThread - Create Keyboard thread
*
*/
CreateKBDThread(stack,size)
char	   *stack;
unsigned    size;
{
    word    tid;
    word    rc;

    rc = DosCreateThread((PFNTHREAD)KBDThread, (PTID)&tid,
			 (PBYTE)&stack[size-(1+2)]);

    if (rc == OS2_ERROR_MAX_THREADS_REACHED)
	Fatal("Couldn't create second thread");
}


/***	SigHandler - Signal Handler
*
*/
void pascal far SigHandler (usArg, usNum)
USHORT usArg, usNum;
{
    long address;
    word rc;

    fKilled = TRUE;	    /* Tell main thread to kill process */
    crc( DosSetSigHandler( (PFNSIGHANDLER)0L,
			   (PFNSIGHANDLER FAR *)&address, &usArg,
			   SIG_ACK, usNum));
}


/***	SigInit - Initialize signal handling
*
*/
SigInit ()
{
    unsigned long far  *address;
    unsigned		oldaction;

    crc( DosSetSigHandler((PFNSIGHANDLER)SigHandler,
	    (PFNSIGHANDLER FAR *)&address, &oldaction,
	    CATCH_SIG, SIGINTR));

    crc( DosSetSigHandler((PFNSIGHANDLER)SigHandler,
	    (PFNSIGHANDLER FAR *)&address, &oldaction,
	    CATCH_SIG, SIGTERM));

    crc( DosSetSigHandler((PFNSIGHANDLER)SigHandler,
	    (PFNSIGHANDLER FAR *)&address, &oldaction,
	    CATCH_SIG, SIGBREAK));
}
