/*  procstat.h - definition of DosGetStatus interface
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Add this header
*	19-Feb-1988 bws Add system semaphore flag definitions
*/

/* OS record types */

#define     REC_PROCESS     0
#define     REC_THREAD	    1
#define     REC_LIBRARY     2
#define     REC_SEMAPHORE   3
#define     REC_MEMORY	    4
#define     REC_EOF	    -1

/* Thread states */

#define     S_VOID	    0
#define     S_ZOMBIE	    1
#define     S_READY	    2
#define     S_BLOCK	    3
#define     S_FROZE	    4
#define     S_CRITSEC	    5
#define     S_BACKGND	    6
#define     S_BOOST	    7

/*  system semaphore flags */

#define SYSSEM_WAITING		  0x01 /* a thread is waiting on the sem */
#define SYSSEM_MUXWAITING	  0x02 /* a thread is muxwaiting on the sem */
#define SYSSEM_OWNER_DIED	  0x04 /* the thread owning the sem died */
#define SYSSEM_EXCLUSIVE	  0x08 /* indicates an exclusive syssem */
#define SYSSEM_NAME_CLEANUP	  0x10 /* NamTab entry needs to be removed */
#define SYSSEM_THREAD_OWNER_DIED  0x20 /* the thread owning the sem died */
#define SYSSEM_EXITLIST_OWNER	  0x40 /* the exitlist thread owns the sem */

/* OS records */

typedef struct proc_s {
    word    p_pid;	    /* process ID */
    word    p_ppid;	    /* parent process ID */
    word    p_sgid;	    /* screen groupd ID */
    word    p_hMTE;	    /* MTE handle for program */
    word   *p_phMTE;	    /* Ptr to list of run-time linked libraries */
    word    p_chMTE;	    /* count of run-time linked libraries */
    word   *p_phSM;	    /* Ptr to shared memory segment handle array */
    word    p_chSM;	    /* count of shared memory segments */
    word   *p_piSem;	    /* ptr to semaphore index array */
    word    p_ciSem;	    /* count of semaphores in use by the process */
} proc_t;

typedef struct thrd_s {
    word    t_slot;	    /* taskptrs slot */
    word    t_pid;	    /* process ID   */
    word    t_tid;	    /* thread ID    */
    word    t_pri;	    /* priority     */
    word    t_state;	    /* thread state */
    dword   t_blkID;	    /* block ID     */
} thrd_t;

typedef struct lib_s {
    word    l_hMTE;	    /* MTE handle of module		*/
    word    l_chMTE;	    /* number of import MTEs		*/
    word   *l_phMTE;	    /* Ptr to import MTE handle array	*/
    char   *l_pName;	    /* Ptr to ASCIIZ module name	*/
} lib_t;

typedef struct sem_s {
    byte    s_slot;	    /* slot of thread owning semaphore	*/
    byte    s_flag;	    /* system semaphore flag bit <???>	*/
    byte    s_ref;	    /* reference count			*/
    byte    s_want;	    /* request count			*/
    word    s_index;	    /* index (in semaphore table)	*/
    char    s_name;	    /* ASCIIZ semaphore name	 */
} sem_t;

typedef struct mem_s {
    word    m_hSM;	    /* Shared memory handle	    */
    word    m_sSM;	    /* Shared memory selector	    */
    word    m_ref;	    /* Reference count		    */
    char    m_name;	    /* ASCIIZ memory name    */
} mem_t;

typedef struct rec_s {
    int 	    r_type;
    struct rec_s   *r_next;
    union {
	proc_t	    pr;
	thrd_t	    th;
	lib_t	    li;
	sem_t	    se;
	mem_t	    me;
    } u;
} rec_t;


extern far pascal DosQProcStatus(FPC, word);
