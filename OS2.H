/*  os2.h - OS/2-specific equates
*
*   Author:
*	Benjamin W. Slivka
*	(c) 1987,1988
*	Microsoft Corporation
*
*   History:
*	17-Aug-1987 bws Initial version
*/

#define     PRI_SCOPE_PROCESS	0	/* DosSetPrty scopes */
#define     PRI_SCOPE_TREE	1
#define     PRI_SCOPE_THREAD	2

#define     PRI_CLASS_UNCHANGED     0	/* DosSetPrty priority classes */
#define     PRI_CLASS_IDLE	    1
#define     PRI_CLASS_REGULAR	    2
#define     PRI_CLASS_TIME_CRITICAL 3


#define     OS2_ERROR_INTERRUPT 	     95
#define     OS2_ERROR_SEM_TIMEOUT	    121
#define     OS2_ERROR_MAX_THREADS_REACHED   164


/*  OS/2 semaphore equates */

#define     OS2_ERROR_SEM_TIMEOUT   121 /* error code for semaphore timout */

#define     SEM_INFINITE	 -1L	/* inifinite timeout */

typedef unsigned long	RAMSEM; 	/* RAM semaphore structure */
