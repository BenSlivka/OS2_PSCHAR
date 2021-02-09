/*  data.h - definition of database
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Add this header
*	18-Aug-1987 bws Define assertion macros
*	24-Aug-1988 bws Tag assertion stuff separately from DEBUG
*/

typedef struct dlib_s {
#ifdef	CheckAsserts
    sig_t		dl_dbgSig;  /* signature for debugging */
#endif
    word		dl_hMTE;    /* MTE handle of module	    */
    word		dl_cMTE;    /* number of import MTEs	    */
    struct lib_s far   *dl_fplRec;  /* Ptr to lib portion of OS rec */
    struct dmte_s      *dl_pMTE;    /* Ptr to import MTE list	    */
    char	       *dl_pName;   /* Ptr to ASCIIZ module name    */
    struct dlib_s      *dl_link;    /* Link for traversal/cleanup   */
    int 		dl_fSeen;   /* Used for graph traversal     */
} dlib_t;

typedef struct dmte_s {
#ifdef	CheckAsserts
    sig_t		dmt_dbgSig; /* signature for debugging */
#endif
    struct dlib_s      *dmt_pLib;   /* ptr to library data */
    struct dmte_s      *dmt_next;   /* ptr to next library */
} dmte_t;


typedef struct dproc_s {
#ifdef	CheckAsserts
    sig_t		dp_dbgSig;  /* signature for debugging */
#endif
    word		dp_pid;     /* process ID */
    word		dp_sgid;    /* screen group ID */
    struct proc_s far  *dp_fppRec;  /* Ptr to proc portion of OS rec */
    struct dproc_s     *dp_pChild;  /* first child process */
    struct dproc_s     *dp_pSibl;   /* next sibling */
    struct dthrd_s     *dp_pThrd;   /* ptr to first thread */
    struct dlib_s      *dp_pLib;    /* ptr to dlib_t of program */
    word		dp_chSM;    /* count of shared memory segments */
    word	       *dp_phSM;    /* Ptr to shared memory segment handles */
    word		dp_chMTE;   /* count of RTL modules */
    word	       *dp_phMTE;   /* Ptr to RTL modules */
    word		dp_ciSem;   /* count of system semaphores */
    word	       *dp_piSem;   /* Ptr to system semaphores */
    struct dproc_s     *dp_link;    /* Link for traversal/cleanup */
} dproc_t;


typedef struct dthrd_s {
#ifdef	CheckAsserts
    sig_t		dt_dbgSig;  /* signature for debugging */
#endif
    word		dt_slot;    /* thread slot  */
    word		dt_tid;     /* thread ID    */
    word		dt_pri;     /* priority     */
    word		dt_state;   /* thread state */
    dword		dt_blkID;   /* block ID     */
    struct dthrd_s     *dt_next;    /* ptr to next thread */
} dthrd_t;


typedef struct dsem_s {
#ifdef	CheckAsserts
    sig_t		ds_dbgSig;  /* signature for debugging */
#endif
    byte		ds_slot;    /* slot of thread owning semaphore	*/
    byte		ds_flag;    /* system semaphore flag bit <???>	*/
    byte		ds_ref;     /* reference count			*/
    byte		ds_want;    /* request count			*/
    word		ds_index;   /* index (in semaphore table)	*/
    char	       *ds_pName;   /* ASCIIZ semaphore name	 */
    struct dsem_s      *ds_link;    /* link for traversal/cleanup */
} dsem_t;


typedef struct dmem_s {
#ifdef	CheckAsserts
    sig_t		dm_dbgSig;  /* signature for debugging */
#endif
    word		dm_hSM;     /* Shared memory handle	    */
    word		dm_sSM;     /* Shared memory selector	    */
    word		dm_ref;     /* Reference count		    */
    char	       *dm_pName;   /* ASCIIZ memory name    */
    struct dmem_s      *dm_link;    /* link for traversal/cleanup */
} dmem_t;


#ifdef	CheckAsserts

#define DE_SIG	    0x4544  /* "DE" */
#define DL_SIG	    0x4C44  /* "DL" */
#define DM_SIG	    0x4D44  /* "DM" */
#define DP_SIG	    0x5044  /* "DP" */
#define DS_SIG	    0x5344  /* "DS" */
#define DT_SIG	    0x5444  /* "DT" */
#define AssertDE(p) doAssertPtr ((char *)(p),DE_SIG,__FILE__,__LINE__)
#define AssertDL(p) doAssertPtr ((char *)(p),DL_SIG,__FILE__,__LINE__)
#define AssertDM(p) doAssertPtr ((char *)(p),DM_SIG,__FILE__,__LINE__)
#define AssertDP(p) doAssertPtr ((char *)(p),DP_SIG,__FILE__,__LINE__)
#define AssertDS(p) doAssertPtr ((char *)(p),DS_SIG,__FILE__,__LINE__)
#define AssertDT(p) doAssertPtr ((char *)(p),DT_SIG,__FILE__,__LINE__)

#else

#define AssertDE(p)
#define AssertDL(p)
#define AssertDM(p)
#define AssertDP(p)
#define AssertDS(p)
#define AssertDT(p)

#endif
