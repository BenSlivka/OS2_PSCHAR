;***	procstat.inc - DosQProcStatus definitions
;
;

;*  Record types

REC_PROCESS	equ	0
REC_THREAD	equ	1
REC_LIBRARY	equ	2
REC_SEMAPHORE	equ	3
REC_MEMORY	equ	4
REC_EOF 	equ	-1

;*  Thread states

S_VOID		equ	0
S_ZOMBIE	equ	1
S_READY 	equ	2
S_BLOCK 	equ	3
S_FROZE 	equ	4
S_CRITSEC	equ	5
S_BACKGND	equ	6
S_BOOST 	equ	7


;*  NOTE: These structures MUST have X_type and X_next fields in common.


;*  Process and Thread records

proc_s	struc
p_type	dw	REC_PROCESS ; Indicates process record
p_next	dw	?	    ; ptr to next record
p_pid	dw	?	    ; process ID
p_ppid	dw	?	    ; parent process ID
p_sgid	dw	?	    ; screen group ID
p_hMTE	dw	?	    ; mte handle for program
p_phMTE dw	?	    ; Ptr to list of run-time linked libraries
p_chMTE dw	?	    ; count of run-time linked libraries
p_phSM	dw	?	    ; ptr to shared memory segment handle array
p_chSM	dw	?	    ; number of shared memory segments
p_piSem dw	?	    ; ptr to semaphore index array
p_ciSem dw	?	    ; number of semaphores in use by the process
proc_s	ends

thrd_s	struc
t_type	dw	REC_THREAD  ; Indicates thread record
t_next	dw	?	    ; ptr to next record
t_slot	dw	?	    ; thread slot
t_pid	dw	?	    ; process ID
t_tid	dw	?	    ; thread ID
t_pri	dw	?	    ; priority
t_state dw	?	    ; thread state
t_blkID dd	?	    ; block ID
thrd_s	ends


;*  Library record

lib_s	struc
l_type	dw	REC_LIBRARY ; Indicates module record
l_next	dw	?	    ; ptr to next record
l_hMTE	dw	?	    ; MTE handle of module
l_chMTE dw	?	    ; count of import MTEs
l_phMTE dw	?	    ; ptr to import MTE handle array
l_pName dw	?	    ; ptr to ASCIIZ module name
lib_s	ends


;*  Semaphore record

sem_s	struc
s_type	dw	REC_SEMAPHORE ; Indicates semaphore record
s_next	dw	?	    ; ptr to next record
s_slot	db	?	    ; slot of thread owning semaphore
s_flag	db	?	    ; system semaphore flag bit <???>
s_ref	db	?	    ; reference count
s_want	db	?	    ; request count
s_index dw	?	    ; index (in semaphore table) for this semaphore
s_name	db	?	    ; ASCIIZ semaphore name
sem_s	ends


;*  Memory record

mem_s	struc
m_type	dw	REC_MEMORY  ; Indicates shared memory record
m_next	dw	?	    ; ptr to next record
m_hSM	dw	?	    ; shared memory handle
m_sSM   dw	?	    ; shared memory selector
m_ref	dw	?	    ; reference count
m_name	db	?	    ; ASCIIZ memory name
mem_s	ends
