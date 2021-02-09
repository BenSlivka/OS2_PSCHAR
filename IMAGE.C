/*  image.c  - build screen images from internal database
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Modified assertion usage
*	18-Aug-1987 bws Added more assertions
*	27-Feb-1988 bws Decode system semaphore flags
*	24-Aug-1988 bws Tag assertion stuff separately from DEBUG
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
#include "data.h"
#include "procstat.h"

#define     NEST_WIDTH	 4  /* Width of nesting column in process display */
#define     LIB_INDENT	 4  /* Library indentation tab width */
#define     SEM_INDENT	21  /* Width of module,pid,tid area in Sem display */

extern	dproc_t *findProc (word);
extern	dproc_t *findSlot (word, dthrd_t **);

extern	dsem_t	*pSemHead;
extern	dmem_t	*pMemHead;


/***	allocLine - allocate a line_t and copy of string
*
*	Create copy of string, allocate a line_t, and make line_t
*	point to copy of string.
*/
line_t	*allocLine (buf)
char *buf;
{
    line_t *li;
    char   *s;

    if ((li = (line_t *)malloc(sizeof(line_t))) == NULL)
	Fatal("allocLine: could not allocate line node");

    if ((s = malloc(strlen(buf)+1)) == NULL)
	Fatal("allocLine: could not allocate string");
    strcpy (s,buf);	/* copy string */

    li->li_next = NULL; /* assume end of list */
    li->li_str = s;	/* point to string */
#ifdef	CheckAsserts
    li->li_dbgSig = LI_SIG;
#endif
    return li;
}


/***	appendLine - append a line to a linked list of lines
*
*/
appendLine (tail, cnt, buf)
line_t	  **tail;
int	   *cnt;
char	   *buf;
{
    line_t *p;

    AssertNotNull(tail);
    AssertLI(*tail);
    AssertNotNull(cnt);
    AssertNotNull(buf);
    p = allocLine(buf);
    (*tail)->li_next = p;
    *tail = p;
    (*cnt)++;
}


/***	stateToString - map a thread state number to state name
*
*/
char *stateToString(state)
int state;
{
    switch (state) {
	case S_VOID:	return "void   ";
	case S_ZOMBIE:	return "zombie ";
	case S_READY:	return "ready  ";
	case S_BLOCK:	return "block  ";
	case S_FROZE:	return "froze  ";
	case S_CRITSEC: return "critsec";
	case S_BACKGND: return "backgnd";
	case S_BOOST:	return "boost  ";
	default:	return "?????  ";
    }
}


/***	traverseProcs - Traverse process tree depth-first to produce image
*
*/
traverseProcs (p,nest,tail,cnt,func)
dproc_t    *p;
int	    nest;
line_t	  **tail;	/* Screen image */
int	   *cnt;	/* Number of rows in image */
int	   (*func)(dproc_t *, int, line_t **, int *);  /* Report function */
{
    dproc_t *x;

    AssertDP(p);
    AssertNotNull(cnt);
    AssertNotNull(tail);
    AssertLI(*tail);
    for (x=p->dp_pChild; x != NULL; x=x->dp_pSibl) {
	AssertDP(x);
	(*func)(x,nest,tail,cnt);   /* Do report */
	traverseProcs(x,nest+1,tail,cnt,func);	/* do child processes */
    }
}


/***	imageProcSub - recursively image child processes of given process
*
*/
imageProcSub (p,nest,tail,cnt)
dproc_t    *p;		/* Process to image */
int	    nest;	/* nesting count */
line_t	  **tail;	/* Screen image */
int	   *cnt;	/* Number of rows in image */
{
    dthrd_t    *t;
    char	buf[NEST_WIDTH+1];
    char	lbuf[MAX_SCREEN_WIDTH];

    AssertDP(p);
    AssertNotNull(tail);
    AssertLI(*tail);
    AssertNotNull(cnt);
    if (nest <= NEST_WIDTH) {
	memset(buf,' ',NEST_WIDTH); /* blank buffer */
	buf[NEST_WIDTH] = '\0';     /* set terminator */
	buf[nest-1] = nest + '0';   /* set nest level */
    }
    else
	fmt (buf, "%*d", NEST_WIDTH, nest);

	       /*  "Nest  pid   sgid    Name" */
    fmt (lbuf, "%*s  %04x   %02x     %s\0",
	NEST_WIDTH, buf, p->dp_pid, p->dp_sgid, p->dp_pLib->dl_pName);
    appendLine (tail, cnt, lbuf);
    for (t=p->dp_pThrd; t != NULL; t=t->dt_next) {  /* image threads */
	AssertDT(t);
		  /*  "Nest  pid   sgid    Name" */
		  /*  "                tid   pri   state    name" */
	fmt (lbuf,"                 %02x   %04x  %7s  %08lx\0",
	    t->dt_tid, t->dt_pri, stateToString(t->dt_state), t->dt_blkID);
	appendLine (tail, cnt, lbuf);
    }
}


/***	imageProc - Produce screen image of process and thread data
*
*/
imageProc (head, cnt)
line_t	  **head;	/* Start of screen image */
int	   *cnt;	/* Number of rows in image */
{
    dproc_t *p;
    line_t  *tail;

    AssertNotNull(cnt);
    AssertNotNull(head);
    if ((p = findProc(0)) == NULL)
	Fatal("imageProcs: could not find process 0");

    tail = allocLine("proc head");   /* header node */
    *head = tail;
    *cnt = 0;	    /* do not count header line */
    traverseProcs (p,1,&tail,cnt,imageProcSub);
}


/***	imageLibSub - image library data for given process
*
*/
imageLibSub (p,nest,tail,cnt)
dproc_t    *p;
int	    nest;
line_t	  **tail;	/* Screen image */
int	   *cnt;	/* Number of rows in image */
{
    char    lbuf[MAX_SCREEN_WIDTH];

    AssertDP(p);
    AssertNotNull(tail);
    AssertLI(*tail);
    AssertNotNull(cnt);
	       /*  "PID   SGID    Name" */
    fmt (lbuf, "%04x   %02x   %s\0",
	p->dp_pid,
	p->dp_sgid,
	p->dp_pLib->dl_pName
    );
    appendLine (tail, cnt, lbuf);
    imageMTE(p->dp_pLib,0,tail,cnt);	/* Image MTEs recursively */
}


/***	imageMTE - recusively image library modules
*
*/
imageMTE (l,nest,tail,cnt)
dlib_t	   *l;
int	    nest;
line_t	  **tail;	/* Screen image */
int	   *cnt;	/* Number of rows in image */
{
    dlib_t  *pL;
    dmte_t  *m;
    int     fSeen;
    char    indent[MAX_SCREEN_WIDTH];
    char    lbuf[MAX_SCREEN_WIDTH];
    char    mark[2];	/* room for asterisk (*) plus 0-byte terminator */

    AssertDL(l);
    AssertNotNull(tail);
    AssertLI(*tail);
    AssertNotNull(cnt);
    memset (indent, ' ', nest*LIB_INDENT);
    indent[nest*LIB_INDENT] = '\0';	/* terminate indent */
    for (m=l->dl_pMTE; m != NULL; m=m->dmt_next) {
	AssertDE(m);
	pL = m->dmt_pLib;
	AssertDL(pL);
	fSeen = pL->dl_fSeen;

	if (fSeen)
	    strcpy (mark,"*");
	else
	    mark[0] = '\0';
		   /*  "PID   SGID    Name" */
		   /*  "                  Library List" */
	fmt (lbuf, "                  %s%s%s",
		 indent,
		 pL->dl_pName,
		 mark
	);
	appendLine (tail, cnt, lbuf);
	if (!fSeen) {
	    pL->dl_fSeen = TRUE;	    /* mark library as seen */
	    imageMTE(pL,nest+1,tail,cnt);   /* Image MTEs recursively */
	}
    }
}


/***	imageLib - Produce screen image of library data
*
*/
imageLib (head, cnt)
line_t	  **head;	/* Start of screen image */
int	   *cnt;	/* Number of rows in image */
{
    dproc_t *p;
    line_t  *tail;

    AssertNotNull(cnt);
    AssertNotNull(head);
    if ((p = findProc(0)) == NULL)
	Fatal("imageLibs: could not find process 0");

    tail = allocLine("lib head");    /* header node */
    *head = tail;
    *cnt = 0;	    /* do not count header line */

    resetLibs();    /* Clear traversal flags */
    traverseProcs (p,1,&tail,cnt,imageLibSub);
}


/***	imageMem - Produce screen image of Shared Memory Data
*
*/
imageMem  (head, cnt)
line_t	  **head;	/* Start of screen image */
int	   *cnt;	/* Number of rows in image */
{
    line_t *tail;
    char    lbuf[MAX_SCREEN_WIDTH];
    dmem_t *m;

    AssertNotNull(cnt);
    AssertNotNull(head);
    tail = allocLine("mem head");    /* header node */
    *head = tail;
    *cnt = 0;	    /* do not count header line */

    AssertDM(pMemHead);
    AssertDM(pMemHead->dm_link);
    for (m=pMemHead->dm_link; m != NULL; m=m->dm_link) {
	AssertDM(m);
		    /* "hndl  sel   ref   name"*/
	fmt (lbuf, "%04x  %04x  %04x  %s",
	    m->dm_hSM, m->dm_sSM, m->dm_ref, m->dm_pName);
	appendLine (&tail, cnt, lbuf);
    }
}


typedef struct MPWPSZ {
    int     w;
    char *  psz;
} MPWPSZ;

MPWPSZ	mpwpsz[] = {	      /*123456789*/
    {SYSSEM_WAITING,	       "waiters"  },
    {SYSSEM_MUXWAITING,        "muxwait"  },
    {SYSSEM_OWNER_DIED,        "proc died"},
    {SYSSEM_EXCLUSIVE,	       "exclusive"},
    {SYSSEM_NAME_CLEANUP,      "cleanup"  },
    {SYSSEM_THREAD_OWNER_DIED, "thrd died"},
    {SYSSEM_EXITLIST_OWNER,    "exit ownr"}
};

/***	MapSSFlagToName - return name for SysSem flag
*
*/
char *MapSSFlagToName (pw)
int    *pw;
{
    int     i;
    char   *psz = (char *)NULL;

    for (i=0; i<(sizeof(mpwpsz)/sizeof(MPWPSZ)); i++)
	if (*pw & mpwpsz[i].w) {
	    *pw &= !mpwpsz[i].w;    /* clear flag */
	    psz = mpwpsz[i].psz;    /* flag name */
	    break;
	}
    if (psz == NULL)
	psz = " ";
    return psz;
}


/***	imageSem - Produce screen image of Semaphore Data
*
*/
imageSem  (head, cnt)
line_t	  **head;	/* Start of screen image */
int	   *cnt;	/* Number of rows in image */
{
    line_t     *tail;
    char	lbuf[MAX_SCREEN_WIDTH];
    dsem_t     *s;
    dproc_t    *p;
    dthrd_t    *t;
    word	w;
    char       *psz;

    AssertNotNull(cnt);
    AssertNotNull(head);
    tail = allocLine("sem head");    /* header node */
    *head = tail;
    *cnt = 0;	    /* do not count header line */

    AssertDS(pSemHead);
    AssertDS(pSemHead->ds_link);
    for (s=pSemHead->ds_link; s != NULL; s=s->ds_link) {
	AssertDS(s);
	if (s->ds_slot == 0) /* no owner */
	    memset (lbuf,' ', SEM_INDENT);
	else {
	    if ((p = findSlot(s->ds_slot, &t)) == NULL)
		Fatal("imageSem: unknown thread slot");
			/*  123456789 123456789 12 */
			/* "module    pid   tid  " */
	    fmt (lbuf, "%8s  %04x  %02x   ",
		p->dp_pLib->dl_pName, p->dp_pid, t->dt_tid);
	}
			   /* "   status  ref   want  hndl  name" */
	w = s->ds_flag;
	psz = MapSSFlagToName(&w);
	fmt (&(lbuf[SEM_INDENT]), "%9s  %04x  %04x  %04x  %s",
	    psz, s->ds_ref, s->ds_want, s->ds_index, s->ds_pName);
	appendLine (&tail, cnt, lbuf);
	if (w != NULL) {
	    memset(lbuf,' ',SEM_INDENT);    /* blank front of buffer */
	    do {
		psz = MapSSFlagToName(&w);  /* get next flag name */
		fmt(&(lbuf[SEM_INDENT]), "%9s", psz);
		appendLine(&tail, cnt, lbuf);
	    } while (w != 0);
	}
    }
}
