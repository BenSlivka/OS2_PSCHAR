/*  queue.c - inter-thread communication module
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Initial version
*	18-Aug-1987 bws Fix bugs in RemoveEvent and QueueWrite
*	24-Aug-1988 bws Tag assertion stuff separately from DEBUG
*
*
*   WARNING Except for those routines called at queue create/destroy time,
*	    no routine may use the C run-times library, as these are NOT
*	    reentrant.	The only exception to this is for assertion failures.
*
*
*   OS/2 provides a general purpose queue facility, which supports passing
*   data in an ordered fashion from "writing" processes to "reading" process.
*
*   The queues in this module ONLY support passing data between threads of
*   a SINGLE process.  This is especially useful for passing keyboard data
*   from a "keyboard" thread to a "worker" thread.
*
*   The following picture shows how a "queue" is represented.
*
*
*   +----------+		     "data 1"	  "data 2"     "data 3"
*   | semCtrl  |			A	     A		  A
*   +----------+	"header"	|	     |		  |
*   | semRead  |			|	     |		  |
*   +----------+	+-------+    +-------+	  +-------+    +-------+
*   | semWrite |	| NULL	|    | pData |	  | pData |    | pData |
*   +----------+	+-------+    +-------+	  +-------+    +-------+
*   | pevHead  |------->| next	|--->| next  |--->| next  |--->| next  |---||
*   +----------+	+-------+    +-------+	  +-------+    +-------+
*   | pevTail  |-----+						   A
*   +----------+     +---------------------------------------------+
*   | pevFree  |--+
*   +----------+  |	+-------+    +-------+	  +-------+
*		  |	| NULL	|    | NULL  |	  | NULL  |
*		  |	+-------+    +-------+	  +-------+
*		  +---->| next	|--->| next  |--->| next  |---||
*			+-------+    +-------+	  +-------+
*
*/

#define INCL_DOS    1	/* get DOS API equates */

#include <stddef.h>

#include "asrt.h"   /* assertion checking */
#include "vio.h"
#include "os2.h"
#include "queue.h"

#include "os2def.h"
#include "bsedos.h"


/***	LockQueue - gain exclusive access to queue data
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*
*	EXIT	qu_semCtrl semaphore grabbed
*
*	NOTE	This routine may block!
*		Also, queue may be destroyed while we are blocked, so
*		we check the pointer again after waking up.
*/
void	LockQueue(pQu)
qu_t *pQu;
{
    AssertQU(pQu);  /* check for valid parameter from caller */
    crc( DosSemRequest ((HSEM)&(pQu->qu_semCtrl), SEM_INFINITE) );
    AssertQU(pQu);  /* check for queue destruction */
}


/***	UnlockQueue - release exclusive access to queue data
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*
*	EXIT	qu_semCtrl semaphore released
*/
void	UnlockQueue(pQu)
qu_t *pQu;
{
    AssertQU(pQu);
    crc( DosSemClear ((HSEM)&(pQu->qu_semCtrl)) );
}


/***	RemoveEvent - remove event from queue
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*		queue has been LOCKED via LockQueue
*		queue must have at least one event
*
*	EXIT	returns ptr to data
*		puts event on free list
*
*	WARNING LockQueue MUST be called prior to calling this routine!
*/
char *RemoveEvent (pQu)
qu_t *pQu;
{
    ev_t   *pEv;
    ev_t   *pEvHead;
    char   *ptr;

    AssertQU(pQu);
    pEvHead = pQu->qu_pevHead;		/* header event */
    AssertEV(pEvHead);
    AssertNotSame(pEvHead, pQu->qu_pevTail); /* head != tail */

    pEv = pEvHead->ev_pev;		/* event to remove */
    AssertEV(pEv);
    pEvHead->ev_pev = pEv->ev_pev;	/* take event off queue */
    if (pEv == pQu->qu_pevTail) {	/* we just removed last event */
	pQu->qu_pevTail = pEvHead;	/* tail = head */
    }
    else {				/* not last event */
	AssertEV(pEv->ev_pev);
    }

    ptr = pEv->ev_pData;		/* ptr to data */
    AssertNotNull(ptr);

    pEv->ev_pData = NULL;		/* no longer points to valid data */
    pEv->ev_pev = pQu->qu_pevFree;	/* put on free list */
    pQu->qu_pevFree = pEv;		/* new head of free list */

    return (ptr);
}


/***	doFlush - Flush all events from queue
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*		queue locked by LockQueue
*
*	EXIT	existing entries in queue are removed
*
*	NOTE	queue MUST be locked by LockQueue!
*/
char *doFlush (pQu)
qu_t *pQu;
{
    char   *ptr;

    AssertQU(pQu);
    AssertEV(pQu->qu_pevHead);
    AssertEV(pQu->qu_pevTail);
    while (pQu->qu_pevHead != pQu->qu_pevTail) {
	ptr = RemoveEvent (pQu);	/* get data item */
	AssertEV(pQu->qu_pevHead);
	AssertEV(pQu->qu_pevTail);
	(*(pQu->qu_funcFree))(ptr);	/* free data item */
    }
    AssertSame(pQu->qu_pevHead, pQu->qu_pevTail); /* head == tail */
}


/***	QueueFlush - Flush all events from queue
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*
*	EXIT	existing entries in queue are removed
*
*	NOTE	This routine may block!
*/
void QueueFlush (pQu)
qu_t *pQu;
{
    LockQueue (pQu);
    doFlush (pQu);
    crc( DosSemClear ((HSEM)&(pQu->qu_semWrite)) ); /* wakeup writers */
    UnlockQueue (pQu);
}


/***	QueuePeek - Peek next event from queue
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*
*	EXIT	if no event in queue
*		    returns NULL
*		else
*		    returns ptr to data
*		    Queue is NOT altered
*
*	NOTE	This routine may block!
*/
char *QueuePeek (pQu)
qu_t *pQu;
{
    ev_t   *pEv;
    char   *ptr;

    LockQueue (pQu);
    AssertEV(pQu->qu_pevHead);
    AssertEV(pQu->qu_pevTail);
    if (pQu->qu_pevHead != pQu->qu_pevTail) {
	ptr = pQu->qu_pevHead->ev_pev->ev_pData;
	AssertNotNull(ptr);
    }
    else
	ptr = NULL;
    UnlockQueue (pQu);
    return (ptr);
}


/***	QueueReadTO - Read next event from queue, with timeout
*
*	This version of ReadQueue allows the caller to do some work
*	if a keystroke doesn't come back within a specified time period.
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*		timeout - duration, in milliseconds, to wait for event
*
*	EXIT	if event available,
*		    return ptr to event;
*		    event is removed from queue
*		if timeout or interrupted,
*		    return NULL ptr
*
*	NOTE	This routine may block!
*/
char *QueueReadTO (pQu,timeout)
qu_t *pQu;
long timeout;
{
    ev_t	   *pEv;
    char	   *ptr;
    unsigned int    rc=0;

    LockQueue (pQu);
    AssertEV(pQu->qu_pevHead);
    AssertEV(pQu->qu_pevTail);
    while ((rc == 0) && (pQu->qu_pevHead == pQu->qu_pevTail)) {
	crc( DosSemSet((HSEM)&(pQu->qu_semRead)) );
	UnlockQueue (pQu);  /* let queue change */
	rc = DosSemWait((HSEM)&(pQu->qu_semRead),timeout);
	if ((rc == OS2_ERROR_SEM_TIMEOUT) ||
	    (rc == OS2_ERROR_INTERRUPT))
	    ptr = (char *)NULL; /* timed out, or interrupted */
	else if (rc != 0)   /* some unexpected error */
	    crc(rc);	/* kill program */
	else {	/* queue might be non-empty, try again */
	    LockQueue (pQu);
	    AssertEV(pQu->qu_pevHead);
	    AssertEV(pQu->qu_pevTail);
	}
    }
    if (rc == 0) {
	ptr = RemoveEvent (pQu);	/* get data item */
	crc( DosSemClear ((HSEM)&(pQu->qu_semWrite)) ); /* wakeup writers */
    }
    UnlockQueue (pQu);
    return (ptr);
}


/***	QueueRead - Read next event from queue
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*
*	EXIT	if data in queue,
*		    returns ptr to data;
*		    event is removed from queue;
*		if read interrupted,
*		    returns NULL ptr;
*
*	NOTE	This routine may block!
*/
char *QueueRead (pQu)
qu_t *pQu;
{
    char   *ptr;

    ptr = QueueReadTO (pQu, SEM_INFINITE); /* wait forever for event */
    return (ptr);
}


/***	QueueWrite - Write event to queue
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*		ptr - ptr to data object to add to queue
*
*	EXIT	data added to queue
*
*	NOTE	This routine may block!
*/
void QueueWrite (pQu, ptr)
qu_t *pQu;
char *ptr;
{
    ev_t   *pEv;

    AssertNotNull(ptr);
    LockQueue (pQu);
    while ((pEv = pQu->qu_pevFree) == NULL) {	/* queue full */
	crc( DosSemSet((HSEM)&(pQu->qu_semWrite)) );
	UnlockQueue (pQu);  /* let queue change */
	crc( DosSemWait((HSEM)&(pQu->qu_semWrite),SEM_INFINITE) );
	LockQueue (pQu);
    }
    AssertEV(pEv);
    pQu->qu_pevFree = pEv->ev_pev;	/* take event from free list */
    pEv->ev_pData = ptr;		/* point event at data */
    pEv->ev_pev = NULL; 		/* tail of chain */

    AssertEV(pQu->qu_pevTail);
    pQu->qu_pevTail->ev_pev = pEv;	/* add to tail of queue */
    pQu->qu_pevTail = pEv;		/* new queue tail */
    crc( DosSemClear ((HSEM)&(pQu->qu_semRead)) ); /* wakeup readers */
    UnlockQueue (pQu);
}


/***	allocEvent - allocate an event record
*
*	ENTRY	ptr - ptr to data object, may be NULL
*
*	EXIT	return ptr to new event record
*
*	WARNING Caller must set ev_pev!
*/
ev_t *allocEvent (ptr)
char *ptr;
{
    ev_t    *pEv;

    if ((pEv = (ev_t *)malloc(sizeof(ev_t))) == NULL)
	Fatal("allocEvent: malloc failed");

    pEv->ev_pData = ptr;
#ifdef	CheckAsserts
    pEv->ev_dbgSig = EV_SIG;
#endif
    return (pEv);
}


/***	deallocEvent - deallocate an event record
*
*	ENTRY	pEv - ptr to event record to free
*
*	EXIT	event record freed
*/
void deallocEvent (pEv)
ev_t   *pEv;
{
    AssertEV(pEv);
    AssertNull(pEv->ev_pData);	    /* Caller must free data object */
#ifdef	CheckAsserts
    pEv->ev_dbgSig = BAD_SIG;	    /* Smash signature */
#endif
    free (pEv); 		    /* free memory */
}


/***	QueueCreate - Create an event queue
*
*	ENTRY	maxEvents - max number of events on queue
*		funcFree  - function which can free data objects
*
*	EXIT	returns ptr to event queue
*
*	NOTE	1)  if qu_pevHead == qu_pevTail, queue is empty
*		2)  if qu_pevFree == NUL, queue is full
*
*		This routine may block!
*/
qu_t *QueueCreate (maxEvents,func)
int maxEvents;
void (*func)(char *);
{
    qu_t   *pQu;
    ev_t   *pEv;
    ev_t   *pEvLast;
    int     i;

    if ((pQu = (qu_t *)malloc(sizeof(qu_t))) == NULL)
	Fatal("QueueCreate: malloc failed");

#ifdef	CheckAsserts
    pQu->qu_dbgSig = QU_SIG;	/* set signature */
#endif
    crc( DosSemClear ((HSEM)&(pQu->qu_semCtrl)) );
    crc( DosSemClear ((HSEM)&(pQu->qu_semWrite)) );
    crc( DosSemClear ((HSEM)&(pQu->qu_semRead)) );

    pQu->qu_funcFree = func;

    pEv = allocEvent("queue head");
    pQu->qu_pevHead = pEv;
    pQu->qu_pevTail = pEv;
    pEv->ev_pev = NULL; 	    /* header points to nothing */

    pEvLast = NULL;		    /* last free event */
    while (maxEvents--) {
	pEv = allocEvent(NULL);     /* allocate event */
	pEv->ev_pev = pEvLast;	    /* link to previous event */
	pEvLast = pEv;		    /* remember head of list */
    }
    pQu->qu_pevFree = pEv;	    /* set head of free list */

    return (pQu);
}

/***	QueueDestroy - Destroy an event queue
*
*	ENTRY	pQu - ptr to queue created by QueueCreate
*
*	EXIT	space for queue is freed
*
*	NOTE	This routine may block!
*/
void QueueDestroy (pQu)
qu_t *pQu;
{
    ev_t   *pEv, *pEvNext;

    LockQueue (pQu);
    doFlush (pQu);		    /* move all events to free list */
    AssertSame(pQu->qu_pevHead, pQu->qu_pevTail); /* head == tail */
    pEv = pQu->qu_pevFree;	    /* head of free list */

    do {			    /* free event records */
	AssertEV(pEv);
	pEvNext = pEv->ev_pev;	    /* next free event */
	deallocEvent (pEv);
	pEv = pEvNext;
    } while (pEv != NULL);

    pQu->qu_pevHead->ev_pData = NULL; /* clear static ptr so deallocEvent */
				    /* will not complain */
    deallocEvent (pQu->qu_pevHead); /* free header event */

#ifdef	CheckAsserts
    pQu->qu_dbgSig = BAD_SIG;	    /* smash signature */
#endif

    /*	At this point, there should be no waiters.  We unlock AFTER having
    *	smashed the signature, so that we can catch these bad threads, if
    *	they exist.
    *
    *	We "unlock" the queue ourselves, because UnlockQueue checks the
    *	signature, which we have just smashed!
    */
    crc( DosSemClear ((HSEM)&(pQu->qu_semCtrl)) );
    free (pQu); 		    /* free queue itself */
}
