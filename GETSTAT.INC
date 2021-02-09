;***	getstat.inc - DosGetStatus definitions
;
;

;*  Record types

REC_PROCESS	equ	0
REC_THREAD	equ	1
REC_LIBRARY	equ	2
REC_SEMAPHORE	equ	3
REC_MEMORY	equ	4
REC_EOF 	equ	-1

S_RUN		equ	2
S_BLOCK 	equ	3
S_FROZE 	equ	4


;*  NOTE: These structures have X_type and X_next fields in common


;*  Process and Thread records

proc_s	struc
p_type	dw	REC_PROCESS ; Indicates process record
p_next	dw	?	    ; ptr to next record
p_pid	dw	?	    ; process ID
p_ppid	dw	?	    ; parent process ID
p_sgid	dw	?	    ; screen groupd ID
p_hMTE	dw	?	    ; mte handle for program
p_hmCnt dw	?	    ; number of run-time linked libraries
p_phMTE dw	?	    ; Ptr to list of run-time linked libraries
p_smCnt dw	?	    ; number of shared memory segments
p_phSM	dw	?	    ; ptr to shared memory segment handle array
proc_s	ends

thrd_s	struc
t_type	dw	REC_THREAD  ; Indicates thread record
t_next	dw	?	    ; ptr to next record
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
l_mtCnt dw	?	    ; number of import MTEs
l_phMTE dw	?	    ; ptr to import MTE handle array
l_pName dw	?	    ; ptr to ASCIIZ module name
lib_s	ends


;*  Semaphore record

sem_s	struc
s_type	dw	REC_SEMAPHORE ; Indicates semaphore record
s_next	dw	?	    ; ptr to next record
s_state dw	?	    ; semaphore state <???>
s_tid	dw	?	    ; thread owning semaphore
s_flag	dw	?	    ; system semaphore flag bit <???>
s_ref	dw	?	    ; reference count
s_want	dw	?	    ; request count
s_pName dw	?	    ; ASCIIZ semaphore name
sem_s	ends


;*  Memory record

mem_s	struc
m_type	dw	REC_MEMORY  ; Indicates shared memory record
m_next	dw	?	    ; ptr to next record
m_hSM	dw	?	    ; shared memory handle
m_ref	dw	?	    ; reference count
m_pName dw	?	    ; ptr to ASCIIZ memory name
mem_s	ends
