/*  data.c  - Call DosQProcstatus and build internal database
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Modified assertion usage
*	18-Aug-1987 bws Added more assertions, combined init with alloc.
*	10-Oct-1987 bws Record time of last DosQProcstatus call.
*	24-Aug-1988 bws Tag assertion stuff separately from DEBUG
*	01-Sep-1988 bws Use fmt instead of sprintf
*/

#define INCL_DOS    1	/* get DOS API equates */

#include <malloc.h>
#include <stdio.h>

#include "asrt.h"   /* assertion checking */
#include "vio.h"
#include "ps.h"
#include "vars.h"
#include "procstat.h"
#include "data.h"
#include "os2.h"

#include "os2def.h"
#include "bsedos.h"

#define     RECBUF_SIZE     65500   /* Size of OS Record buffer */
#define     HMTE_3XBOX	    -1	    /* "reserved" hMTE for 3xBox "module" */

extern	char	achTime[];	    /* time of last DosQProcstatus call */


/*  Global and Local info seg pointers */

struct _GINFOSEG far *fpisg;
struct _LINFOSEG far *fpisl;

/*  Static data for names for header nodes.  3xBox and DOSCALLS are treated
*   specially.
*/
char	str3xBox[]     = "3xBox";
char	strDosCalls[]  = "DOSCALLS";
char	strLibHdr[]    = "lib data";
char	strSemHdr[]    = "sem data";
char	strMemHdr[]    = "mem data";


/*  Head/Tail pointers database lists */

dproc_t     *pProcHead = (dproc_t *)NULL;
dproc_t     *pProcTail = (dproc_t *)NULL;
dlib_t	    *pLibHead  = (dlib_t *)NULL;
dlib_t	    *pLibTail  = (dlib_t *)NULL;
dsem_t	    *pSemHead  = (dsem_t *)NULL;
dsem_t	    *pSemTail  = (dsem_t *)NULL;
dmem_t	    *pMemHead  = (dmem_t *)NULL;
dmem_t	    *pMemTail  = (dmem_t *)NULL;

rec_t far   *record = (rec_t far *)NULL;    /*	Segment with records from OS */


/***	fstrlen - strlen of char far *
*
*/
fstrlen (s)
char far *s;
{
    int i;

    for (i=0; *s++; i++)
	;
    return i;
}


/***	strcpyFtoN - Copy far string to near string
*
*/
strcpyFtoN (dst,src)
char	 *dst;
char far *src;
{
    while (*dst++ = *src++)
	;
}


/***	allocProc - allocate dprot_t record
*
*/
dproc_t *allocProc (pid, sgid)
int	 pid;
int	 sgid;
{
    dproc_t *p;

    if ((p = (dproc_t *)malloc(sizeof(dproc_t))) == NULL)
	Fatal("allocProc: malloc failed");

    p->dp_pid	    = pid;
    p->dp_sgid	    = sgid;
    p->dp_fppRec    = NULL;
    p->dp_pChild    = NULL;
    p->dp_pSibl     = NULL;
    p->dp_pThrd     = NULL;
    p->dp_pLib	    = NULL;
    p->dp_phSM	    = NULL;
    p->dp_chSM	    = 0;
    p->dp_phMTE     = NULL;
    p->dp_chMTE     = 0;
    p->dp_piSem     = NULL;
    p->dp_ciSem     = 0;
    p->dp_link	    = NULL;
#ifdef CheckAsserts
    p->dp_dbgSig = DP_SIG;
#endif
    return p;
}


/***	allocThrd - allocate dthrd_t record
*
*/
dthrd_t *allocThrd ()
{
    dthrd_t *t;

    if ((t = (dthrd_t *)malloc(sizeof(dthrd_t))) == NULL)
	Fatal("allocThrd: malloc failed");
#ifdef CheckAsserts
    t->dt_dbgSig = DT_SIG;
#endif
    return t;
}


/***	allocLib - allocate dlib_t record
*
*/
dlib_t *allocLib (buf, hMTE, chMTE)
char   *buf;
word	hMTE;
word	chMTE;
{
    dlib_t *l;

    if ((l = (dlib_t *)malloc(sizeof(dlib_t))) == NULL)
	Fatal("allocLib: malloc failed");
    l->dl_pName  = buf;
    l->dl_hMTE	 = hMTE;
    l->dl_cMTE	 = chMTE;
    l->dl_pMTE	 = NULL;
    l->dl_link	 = NULL;
#ifdef CheckAsserts
    l->dl_dbgSig = DL_SIG;
#endif
    return l;
}


/***	allocSem - allocate dsem_t record
*
*/
dsem_t *allocSem (buf)
char   *buf;
{
    dsem_t *s;

    if ((s = (dsem_t *)malloc(sizeof(dsem_t))) == NULL)
	Fatal("allocSem: malloc failed");
    s->ds_pName = buf;
    s->ds_link = (dsem_t *)NULL;
#ifdef CheckAsserts
    s->ds_dbgSig = DS_SIG;
#endif
    return s;
}


/***	allocMem - allocate dmem_t record
*
*/
dmem_t *allocMem (buf)
char   *buf;
{
    dmem_t *m;

    if ((m = (dmem_t *)malloc(sizeof(dmem_t))) == NULL)
	Fatal("allocMem: malloc failed");
    m->dm_link = (dmem_t *)NULL;
    m->dm_pName = buf;
#ifdef CheckAsserts
    m->dm_dbgSig = DM_SIG;
#endif
    return m;
}


/***	allocMTE - allocate dmte_t record
*
*/
dmte_t *allocMTE ()
{
    dmte_t *mt;

    if ((mt = (dmte_t *)malloc(sizeof(dmte_t))) == NULL)
	Fatal("allocMTE: malloc failed");
#ifdef CheckAsserts
    mt->dmt_dbgSig = DE_SIG;
#endif
    return mt;
}


/***	freeProc - free all process and thread database nodes
*
*/
freeProc ()
{
    dproc_t *p=pProcHead;
    dproc_t *p1;
    dthrd_t *t;
    dthrd_t *t1;

    while (p != NULL) {
	AssertDP(p);
	t=p->dp_pThrd;	    /* first thread */
	while (t != NULL) {
	    AssertDT(t);
	    t1=t->dt_next;  /* next thread */
	    free((NPC)t);   /* free this thread */
	    t=t1;	    /* consider next thread */
	}
	p1=p->dp_link;	    /* next process */
	free((NPC)p);	    /* free this process */
	p=p1;		    /* consider next process */
    }

    pProcHead = (dproc_t *)NULL;
    pProcTail = (dproc_t *)NULL;
}


/***	freeLib - free library and MTE database nodes
*
*/
freeLib ()
{
    dlib_t *l=pLibHead;
    dlib_t *l1;
    dmte_t *mt;
    dmte_t *mt1;

    while (l != NULL) {
	AssertDL(l);
	AssertNotNull(l->dl_pName);
	free(l->dl_pName);  /* free library name */
	mt=l->dl_pMTE;	    /* first mte */
	while (mt != NULL) {
	    AssertDE(mt);
	    mt1=mt->dmt_next; /* next mte */
	    free((NPC)mt);  /* free this mte */
	    mt=mt1;	    /* consider next mte */
	}
	l1=l->dl_link;	    /* next library */
	free((NPC)l);	    /* free this library */
	l=l1;		    /* consider next library */
    }

    pLibHead  = (dlib_t *)NULL;
    pLibTail  = (dlib_t *)NULL;
}


/***	freeSem - free semaphore names and database nodes
*
*/
freeSem ()
{
    dsem_t *s=pSemHead;
    dsem_t *s1;

    while (s != NULL) {
	AssertDS(s);
	AssertNotNull(s->ds_pName);
	free(s->ds_pName);  /* free name */
	s1=s->ds_link;	    /* next semaphore */
	free((NPC)s);	    /* free this semaphore */
	s=s1;		    /* consider next semaphore */
    }

    pSemHead  = (dsem_t *)NULL;
    pSemTail  = (dsem_t *)NULL;
}


/***	freeMem - free shared memory names and database nodes
*
*/
freeMem ()
{
    dmem_t *m=pMemHead;
    dmem_t *m1;

    while (m != NULL) {
	AssertDM(m);
	AssertNotNull(m->dm_pName);
	free(m->dm_pName);  /* free name */
	m1=m->dm_link;	    /* next shared memory */
	free((NPC)m);	    /* free this shared memory */
	m=m1;		    /* consider next shared memory */
    }

    pMemHead  = (dmem_t *)NULL;
    pMemTail  = (dmem_t *)NULL;
}


/***	freeDatabase - Deallocate database memory
*
*/
freeDatabase()
{
    freeProc ();
    freeLib ();
    freeSem ();
    freeMem ();
}


/***	initDatabase - initialized data structures for database
*
*/
initDatabase ()
{
    dproc_t    *p;
    dproc_t    *p0;
    dlib_t     *l;
    dlib_t     *l0;
    dlib_t     *l1;
    dsem_t     *s;
    dmem_t     *m;
    char       *buf;
    unsigned	selGIS;
    unsigned	selLIS;

    freeDatabase();

    p = allocProc(0,0); 	/* allocate new header node */
    p0 = allocProc(0,-1);	/* allocate process 0 */
    p->dp_link = p0;
    pProcHead = p;
    pProcTail = p0;

    if ((buf = malloc(sizeof(strLibHdr))) == NULL)
	Fatal("initDatabase: cannot allocate lib hdr name");
    strcpy(buf,strLibHdr);
    l = allocLib(buf,NULL,NULL);	/* allocate header node */

    if ((buf = malloc(sizeof(str3xBox))) == NULL)
	Fatal("initDatabase: cannot allocate 3xBox name");
    strcpy(buf,str3xBox);		/* Copy name */
    l0 = allocLib(buf,HMTE_3XBOX,NULL); /* allocate 3xBox library node */
    l0->dl_fplRec = NULL;		/* No OS record */

    if ((buf = malloc(sizeof(strDosCalls))) == NULL)
	Fatal("initDatabase: cannot allocate DosCalls name");
    strcpy(buf,strDosCalls);		/* Copy name */
    l1 = allocLib(buf,NULL,NULL);	/* allocate DosCalls library node */
    l1->dl_fplRec = NULL;		/* No OS record */

    l->dl_link = l0;
    l0->dl_link = l1;
    pLibHead  = l;
    pLibTail  = l1;

    if ((buf = malloc(sizeof(strSemHdr))) == NULL)
	Fatal("initDatabase: cannot allocate sem hdr name");
    strcpy(buf,strSemHdr);
    s = allocSem(buf);
    pSemHead = s;
    pSemTail = s;

    if ((buf = malloc(sizeof(strMemHdr))) == NULL)
	Fatal("initDatabase: cannot allocate mem hdr name");
    strcpy(buf,strMemHdr);
    m = allocMem(buf);
    pMemHead = m;
    pMemTail = m;

    crc( DosGetInfoSeg((PSEL)&selGIS,(PSEL)&selLIS) );

    /* convert selectors into far pointers */
    fpisg = (struct _GINFOSEG far *)((long)selGIS << 16);
    fpisl = (struct _LINFOSEG far *)((long)selLIS << 16);
}


/***	findProc - map pid to process node
*
*/
dproc_t *findProc (pid)
word	pid;
{
    dproc_t *p;

    AssertDP(pProcHead);
    p=pProcHead->dp_link;	/* p = dummy process 0 */
    AssertDP(p);
    while ( (p != NULL) && (p->dp_pid != pid) ) { /* Scan process list */
	AssertDP(p);
	p = p->dp_link;
    }
    if (p != NULL)
	AssertDP(p);
    return p;
}


/***	findSlot - map slot to process, thread nodes
*
*/
dproc_t *findSlot (slot,pThrd)
word	slot;
dthrd_t **pThrd;
{
    dproc_t *p;
    dthrd_t *t;

    AssertDP(pProcHead);
    p=pProcHead->dp_link;	/* p = dummy process 0 */
    AssertDP(p);
    p=p->dp_link;		/* p = first real process */
    AssertDP(p);
    while (p != NULL) { /* Scan process list */
	t = p->dp_pThrd;
	AssertDT(t);
	while (t != NULL) { /* Scan thread list */
	    if (t->dt_slot == slot) {
		*pThrd = t; /* return thread node */
		return p;   /* return process node */
	    }
	    AssertDT(t);
	    t = t->dt_next; /* next thread */
	}
	AssertDP(p);
	p = p->dp_link; /* next process */
    }
    return p;
}


/***	findLib - map MTE handle to library node
*
*/
dlib_t	*findLib (hMTE)
word	hMTE;
{
    dlib_t  *l;

    AssertDL(pLibHead);
    l=pLibHead->dl_link;
    AssertDL(l);
    while ( (l != NULL) && (l->dl_hMTE != hMTE) ) { /* Scan library list */
	AssertDL(l);
	l = l->dl_link;
    }
    return l;
}


/***	scanRecords - scan data from OS and build database
*
*/
scanRecords (record, recType, recFunc)
rec_t far *record;
int	   recType;
int	  (*recFunc)(rec_t far *);
{
    rec_t far *p=record;

    while (p->r_type != REC_EOF) {
	if (p->r_type == recType)
	    (*recFunc)(p);
	p = (rec_t far *)farp(p,p->r_next);
    }
}


/***	parseProc - first pass at building process nodes
*
*/
parseProc (r)
rec_t far *r;
{
    proc_t far *pR = &(r->u.pr);
    dproc_t    *p;

    p = allocProc(pR->p_pid, pR->p_sgid);   /* allocate process node */
    p->dp_fppRec = pR;

    if (pR->p_hMTE == NULL)	    /* Only 3xBox has null hMTE! */
	pR->p_hMTE = HMTE_3XBOX;    /* Set special hMTE for 3xBox name */

    AssertDP(pProcTail);
    pProcTail->dp_link = p;	/* point last process to this one */
    pProcTail = p;		/* this one is new tail of list */
}


/***	parseThrd - build thread nodes
*
*/
parseThrd (r)
rec_t far *r;
{
    thrd_t far *pR = &(r->u.th);
    dthrd_t    *t;
    dproc_t    *p;

    t = allocThrd();		/* allocate new thread node */
    if ((p = findProc(pR->t_pid)) == NULL)
	Fatal("parseThrd: thread without process");

    t->dt_slot	  = pR->t_slot;
    t->dt_tid	  = pR->t_tid;
    t->dt_pri	  = pR->t_pri;
    t->dt_state   = pR->t_state;
    t->dt_blkID   = pR->t_blkID;
    t->dt_next	  = NULL;
    insertThrd(&(p->dp_pThrd), t);  /* Link thread to process */
}


/***	insertThrd - Insert thread into process thread list
*
*	Threads are maintained in ascending order by thread id
*/
insertThrd (q, e)
dthrd_t **q;
dthrd_t  *e;
{
    dthrd_t *c=*q;

    AssertDT(e);
    if (c == NULL)			/* Empty list */
	*q = e;
    else if (e->dt_tid < c->dt_tid) {	/* Insert at head of list */
	e->dt_next = c;
	*q = e;
    }
    else {
	while ((c->dt_next) && (e->dt_tid > c->dt_next->dt_tid)) {
	    AssertDT(c);
	    c = c->dt_next;
	}
	e->dt_next = c->dt_next;
	c->dt_next = e;
    }
}


/***	parseLib - first pass at building library nodes
*
*/
parseLib (r)
rec_t far *r;
{
    lib_t far  *pR = &(r->u.li);
    dlib_t     *l;
    char  far  *pN;
    char       *buf;
    int 	lenN;

    pN = (char far *)farp(pR,pR->l_pName);
    lenN = fstrlen(pN);

    if ((buf = malloc(lenN+1)) == NULL)
	Fatal("parseLib: cannot allocate library name");
    strcpyFtoN (buf,pN);	/* Copy name to near buffer */
    l = allocLib(buf,pR->l_hMTE, pR->l_chMTE);
    l->dl_fplRec = pR;

    AssertDL(pLibTail);
    pLibTail->dl_link = l;	/* point last process to this one */
    pLibTail = l;		/* this one is new tail of list */
}


/***	linkLibs - second pass over libarary nodes
*
*	Finish links to produce module graph.
*/
linkLibs ()
{
    lib_t far  *pR;	/* ptr to OS library record */
    dlib_t     *l;	/* library node */
    dlib_t     *pL;	/* successor library node */
    dmte_t     *pM;	/* MTE node */
    int 	i;
    word  far  *m;	/* ptr to list of MTE handles */
    int 	cnt;	/* count of MTE handles */

    for (l=pLibHead->dl_link; l != NULL; l=l->dl_link) {
	AssertDL(l);
	if ((cnt = l->dl_cMTE) > 0) {
	    pR = l->dl_fplRec;			/* OS library record */
	    m = (word far *)farp(pR,pR->l_phMTE);    /* list of MTEs */
	    for (i=0; i<cnt; m++,i++) { /* Create mte links */
		if ((pL = findLib(*m)) == NULL)
		    Fatal("linkLibs: invalid mte handle");
		pM = allocMTE();
		pM->dmt_pLib = pL;
		pM->dmt_next = l->dl_pMTE;
		l->dl_pMTE   = pM;
	    }
	}
    }
}


/***	parseSem - build semaphore node
*
*/
parseSem (r)
rec_t far *r;
{
    sem_t far *pR = &(r->u.se);
    dsem_t     *s;
    char far   *pN;
    char       *buf;
    int 	lenN;

    pN = (char far *)&(pR->s_name);
    lenN = fstrlen(pN);
    if ((buf = malloc(lenN+1)) == NULL)
	Fatal("parseSem: malloc failed");
    strcpyFtoN (buf,pN);	/* copy far data to near buffer */
    s = allocSem(buf);		/* create semaphore node */

    s->ds_slot	= pR->s_slot;	/* copy rest of fields */
    s->ds_flag	= pR->s_flag;
    s->ds_ref	= pR->s_ref;
    s->ds_want	= pR->s_want;
    s->ds_index = pR->s_index;

    AssertDS(pSemTail);
    pSemTail->ds_link = s;	/* point last node to this one */
    pSemTail = s;		/* this one is new tail of list */
}


/***	parseMem - build shared memory node
*
*/
parseMem (r)
rec_t far *r;
{
    mem_t far  *pR = &(r->u.me);
    dmem_t     *m;
    char far   *pN;
    char       *buf;
    int 	lenN;

    pN = (char far *)&(pR->m_name);
    lenN = fstrlen(pN);
    if ((buf = malloc(lenN+1)) == NULL)
	Fatal("parseMem: malloc failed");
    strcpyFtoN (buf,pN);	/* copy far data to near buffer */
    m = allocMem(buf);		/* create shared memory node */

    m->dm_hSM = pR->m_hSM;	/* copy rest of fields */
    m->dm_sSM = pR->m_sSM;
    m->dm_ref = pR->m_ref;

    AssertDM(pMemTail);
    pMemTail->dm_link = m;	/* point last node to this one */
    pMemTail = m;		/* this one is new tail of list */
}


/***	linkProcs - second pass over process nodes
*
*	Finish links to produce process tree.
*/
linkProcs ()
{
    proc_t far *pR;	/* ptr to OS process record */
    dproc_t    *p;	/* process node */
    dproc_t    *pP;	/* parent process node */
    dlib_t     *l;	/* library node */

    /* NOTE: skip process 0, it doesn't have a parent */

    for (p=pProcHead->dp_link->dp_link; p != NULL; p=p->dp_link) {
	AssertDP(p);
	pR = p->dp_fppRec;		    /* OS process record */
	if ((pP = findProc(pR->p_ppid)) == NULL)
	    Fatal("linkProcs: process without parent");

	insertProc (&(pP->dp_pChild),p);

	if ((l = findLib(pR->p_hMTE)) == NULL)
	    Fatal("linkProcs: invalid library handle");
	p->dp_pLib = l; 		    /* Point to program */
    }
}


/***	insertProc - insert process into child list
*
*	Insert process in ascending <sgid,pid> order on child list
*/
insertProc (q, e)
dproc_t **q;
dproc_t  *e;
{
    dproc_t *c=*q;

    AssertDP(e);
    if (c == NULL)			/* Empty list */
	*q = e;
    else if (compareProc(e,c) < 0) {	/* Insert at head of list */
	e->dp_pSibl = c;
	*q = e;
    }
    else {				/* Scan for correct position */
	while ((c->dp_pSibl) && (compareProc(e,c->dp_pSibl) > 0)) {
	    AssertDP(c);
	    c = c->dp_pSibl;
	}
	e->dp_pSibl = c->dp_pSibl;
	c->dp_pSibl = e;
    }
}

/***	compareProc - compute lexicographic order on two processes
*
*	Compare processes based on <sgid,pid>
*
*	EXIT	<0 ==> a <  b
*		 0 ==> a == b
*		>0 ==> a >  b
*/
compareProc (a,b)
dproc_t  *a;
dproc_t  *b;
{
    int     i;

    AssertDP(a);
    AssertDP(b);
    i = a->dp_sgid - b->dp_sgid;
    return (i != 0 ? i : a->dp_pid - b->dp_pid);
}


/***	buildDatabase - Get data from OS and build database structure
*
*/
buildDatabase ()
{
    if (record == NULL)     /* Need to allocate buffer */
	if ((record = (rec_t far *)_fmalloc(RECBUF_SIZE)) == NULL)
	    Fatal ("buildDatabase: could not get buffer");
    if (DosQProcStatus((FPC)record, RECBUF_SIZE))
	Fatal ("buildDatabase: DosQProcstatus failed");

    initDatabase();
#ifdef DEBUG
    if (fRecord)
	printRecords(record);
#endif

    scanRecords(record, REC_PROCESS, parseProc);
    scanRecords(record, REC_THREAD , parseThrd);
    scanRecords(record, REC_LIBRARY, parseLib);
    scanRecords(record, REC_SEMAPHORE, parseSem);
    scanRecords(record, REC_MEMORY, parseMem);
#ifdef DEBUG
    if (fXDebug) {
	printf ("BEFORE linking\n");
	printf ("\n");
	dumpProcs();
	printf ("\n");
	dumpLibs();
    }
#endif
    linkLibs();
    linkProcs();
#ifdef DEBUG
    if (fXDebug) {
	printf ("\n");
	printf ("AFTER linking\n");
	printf ("\n");
	printf ("\n");
	dumpProcs();
	printf ("\n");
	dumpLibs();
    }
#endif
    FormatTime ();	/* Record time of last database build */
}


/***	FormatTime - Format current time
*
*/
FormatTime ()
{
    int     s;

    s = fpisg->hundredths/10;	/* tenths of seconds */
    fmt (achTime, "%2d:%02d:%02d.%01d",
	     fpisg->hour,fpisg->minutes,fpisg->seconds,s);
}


/***	resetLibs - reset "seen" flag in library nodes
*
*	When imaging the module graph, these flags are used to
*	detect cycles.
*/
resetLibs ()
{
    dlib_t  *l=pLibHead->dl_link;

    AssertDL(pLibHead);
    for (l=pLibHead->dl_link; l; l = l->dl_link) {
	AssertDL(l);
	l->dl_fSeen = FALSE;
    }
}


#ifdef DEBUG
/***	printProc - Print an OS process record on stdout
*
*/
printProc (r)
rec_t far *r;
{
    proc_t far *p = &(r->u.pr);
    word  far  *w;
    int 	i;

    printf ("Process   pid=%04x  ppid=%04x  sgid=%02x  hMTE=%04x\n",
	p->p_pid, p->p_ppid, p->p_sgid, p->p_hMTE);

    printf ("    Run-Time Linked MTE handles:\n");
    w = (word far *)farp(p,p->p_phMTE);
    for (i=0; i<p->p_chMTE; w++, i++)	/* print RTL MTE handles */
	printf ("        %04x\n", *w);

    printf ("    Shared Memory Handles:\n");
    w = (word far *)farp(p,p->p_phSM);
    for (i=0; i<p->p_chSM; w++, i++)   /* print shared mem handles */
	printf ("        %04x\n", *w);

    printf ("    System Semaphore Indicies:\n");
    w = (word far *)farp(p,p->p_piSem);
    for (i=0; i<p->p_ciSem; w++, i++)	/* print semaphore indicies */
	printf ("        %04x\n", *w);
}


/***	printThrd - Print an OS thread record on stdout
*
*/
printThrd (r)
rec_t far *r;
{
    thrd_t far *p = &(r->u.th);

    printf ("Thread    tid=%02x  pid=%04x  pri=%04x  state=%02x  blkID=%08lx\n",
	p->t_tid, p->t_pid, p->t_pri, p->t_state, p->t_blkID);
}


/***	printLib - Print an OS library record on stdout
*
*/
printLib (r)
rec_t far *r;
{
    lib_t far *p = &(r->u.li);
    int        i;
    word  far *m;
    char       buf[100];

    strcpyFtoN (buf,(char far *)farp(p,p->l_pName));
    printf ("Library   hMTE=%04x  %s\n", p->l_hMTE, buf);
    m = (word far *)farp(p,p->l_phMTE);
    for (i=0; i<p->l_chMTE; m++,i++)
	printf ("       %04x\n",*m);
}


/***	printSem - Print an OS semaphore record on stdout
*
*/
printSem (r)
rec_t far *r;
{
    sem_t far *p = &(r->u.se);
    char       buf[100];

    strcpyFtoN (buf,(char far *)&(p->s_name));
    printf ("Semaphore tid=%02x flag=%04x ref=%04x want=%04x ind=%04x name=%s\n",
	p->s_slot, p->s_flag, p->s_ref, p->s_want, p->s_index, buf);
}


/***	printMem - Print an OS shared memory record on stdout
*
*/
printMem (r)
rec_t far *r;
{
    mem_t far *p = &(r->u.me);
    char       buf[100];

    strcpyFtoN (buf,(char far *)&(p->m_name));
    printf ("ShareMem  hndl=%04x sel=%04x ref=%04x name=%s\n",
	p->m_hSM, p->m_sSM, p->m_ref, buf);
}


/***	printRecords - Print OS records on stdout
*
*/
printRecords (record)
rec_t far *record;
{
    rec_t far *p=record;

    while (p->r_type != REC_EOF) {
	switch (p->r_type) {
	    case REC_PROCESS:
		printProc (p);
		break;
	    case REC_THREAD:
		printThrd (p);
		break;
	    case REC_LIBRARY:
		printLib (p);
		break;
	    case REC_SEMAPHORE:
		printSem (p);
		break;
	    case REC_MEMORY:
		printMem (p);
		break;
	    default:
		Fatal("printRecords: invalid record type");
		break;
	}
	p = (rec_t far *)farp(p,p->r_next);
    }
}


/***	dumpProcs - Dump database process nodes to stdout
*
*/
dumpProcs ()
{
    dproc_t *p;

    printf     ("Process\n");
    printf     ("addr  pid  sgid fppRec   pChild pSibl pThrd pLib link\n");
    printf     ("----  ---  ---- -------- ------ ----- ----- ---- ----\n");
    for (p=pProcHead->dp_link; p; p=p->dp_link) {
	printf ("%04x  %04x %02x   %08lx %04x   %04x  %04x  %04x %04x\n",
	    p,
	    p->dp_pid,
	    p->dp_sgid,
	    p->dp_fppRec,
	    p->dp_pChild,
	    p->dp_pSibl,
	    p->dp_pThrd,
	    p->dp_pLib,
	    p->dp_link
	);
    }
}


/***	dumpLibs - Dump database library nodes to stdout
*
*/
dumpLibs ()
{
    dlib_t  *l;

    printf     ("Library\n");
    printf     ("addr  hMTE mCnt fplRec   pMTE pName link\n");
    printf     ("----  ---- ---- -------- ---- ----- ----\n");
    for (l=pLibHead->dl_link; l; l=l->dl_link) {
	printf ("%04x  %04x %04x %08lx %04x %04x  %04x\n",
	    l,
	    l->dl_hMTE,
	    l->dl_cMTE,
	    l->dl_fplRec,
	    l->dl_pMTE,
	    l->dl_pName,
	    l->dl_link
	);
    }
}
#endif
