/*  queue.h - inter-thread communication definitions
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Initial version
*	24-Aug-1988 bws Tag assertion stuff separately from DEBUG
*/

/*  WARNING
*	ev_t and qu_t are defined here to satisfy the extern
*	definitions.  There only use outside of queue.c is to
*	declare a pointer to a queue, e.g., "qu_t *pquMyQueue".
*/

/*  ev_t - Event record */
typedef struct ev_s {
#ifdef	CheckAsserts
    sig_t	    ev_dbgSig;	/* signature for debugging */
#endif
    struct ev_s    *ev_pev;	/* ptr to next event */
    void	   *ev_pData;	/* ptr to event data */
} ev_t;


/*  qu_t - Queue record */
typedef struct chan_s {
#ifdef	CheckAsserts
    sig_t   qu_dbgSig;	    /* signature for debugging */
#endif
    RAMSEM  qu_semCtrl;     /* semaphore for control of queue */
    RAMSEM  qu_semRead;     /* semaphore for readers to block on */
    RAMSEM  qu_semWrite;    /* semaphore for writers to block on */
    ev_t   *qu_pevHead;     /* ptr to read side of ev_t queue */
    ev_t   *qu_pevTail;     /* ptr to write side of ev_t queue */
    ev_t   *qu_pevFree;     /* pool of free ev_t's */
    void   (*qu_funcFree)(char *); /* function to free data items */
} qu_t;

extern qu_t *QueueCreate  (int, void (*)(char *)); /* create a queue */
extern void  QueueDestroy (qu_t *);		   /* destroy a queue */
extern void  QueueFlush   (qu_t *);		   /* flush a queue */
extern char *QueuePeek	  (qu_t *);		   /* peek at queue */
extern char *QueueRead	  (qu_t *);		   /* read item from queue */
extern char *QueueReadTO  (qu_t *, long);	   /* read, with timeout */
extern void  QueueWrite   (qu_t *, char *);	   /* write item to queue */


#ifdef	CheckAsserts

#define EV_SIG	    0x5645  /* "EV" without null trail byte */
#define QU_SIG	    0x5551  /* "QU" without null trail byte */
#define AssertEV(p) doAssertPtr ((char *)(p),EV_SIG,__FILE__,__LINE__)
#define AssertQU(p) doAssertPtr ((char *)(p),QU_SIG,__FILE__,__LINE__)

#else

#define AssertEV(p)	    /* turn off assertion check */
#define AssertQU(p)	    /* turn off assertion check */

#endif
