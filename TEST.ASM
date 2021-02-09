	page	,132

;***	test.asm - test data

.286p

include getstat.inc

data	segment
data	ends


ERROR_NOT_ENOUGH_MEMORY     equ     8	    ; error code if buffer too small

code	segment     'testcode'
    assume  cs:code

;***	DOSGETSTATUS (buffer, length)
;
;	ENTRY:	buffer - far ptr to user buffer to receive data
;		length - length of buffer in bytes
;		CLD
;
;	EXIT:	(ax) == 0, success
;		(ax) != 0, failure (buffer too small)
;
;	USES:	ax,cx,es
;
;	WARNING : This is an "extern far pascal" routine.

dgs_pBuf    equ     dword ptr [BP+8]
dgs_Len     equ     word ptr [BP+6]
ARG_BYTES   equ     6

    public  DOSGETSTATUS
DOSGETSTATUS	proc	far
	push	bp
	mov	bp,sp
	push	di
	push	si
	push	ds

	mov	cx,offset endTestData - offset TestData ; (cx) = data length
	cmp	dgs_Len,cx
	mov	ax,ERROR_NOT_ENOUGH_MEMORY  ; (ax) = error code
	jb	dgsx			; Buffer too small

	les	di,dgs_pBuf		; (es:di) = destination buffer
	mov	bx,di			; (bx) = offset of user buffer

	push	data
	pop	ds
    assume  ds:data
	mov	si,offset TestData	; (ds:si) = source buffer

	rep movsb			; Copy data to caller's buffer

;*  Relocate pointers
;
;   (ES:BX) = user buffer

	push	es
	pop	ds
    assume  ds:nothing

	mov	dx,bx			; (dx) = offset of user buffer
	sub	dx,offset TestData	; (dx) = (user) - (test)
	jmp	short dgs2		; Enter loop at test

dgs1:	shl	si,1			; (si) = offset of reloc routine
	call	cs:relocFunc[si]	; call reloc routine

	add	[bx].p_next,dx		; relocate next ptr
	mov	bx,[bx].p_next		; (bx) = next record

dgs2:	mov	si,[bx] 		; (si) = record type
	cmp	si,REC_EOF		; Check for end record
	jne	dgs1			; Not last record

;*  Done relocating
	xor	ax,ax			; (ax) = success

dgsx:	pop	ds
	pop	si
	pop	di
	pop	bp
	ret	ARG_BYTES
DOSGETSTATUS	endp


relocFunc   label   word
	dw	relocProc
	dw	relocThrd
	dw	relocLib


;***	relocProc - relocate process pointers
;
;	ENTRY:	(ds:bx) = record to relocate
;		   (dx) = relocation amount
;
;	EXIT:	pointers in record relocated
;
;	USES:	none

relocProc   proc

;*  NOTE: no shared memory segments, yet, so do nothing.

	ret
relocProc   endp


;***	relocThrd - relocate thread pointers
;
;	ENTRY:	(ds:bx) = record to relocate
;		   (dx) = relocation amount
;
;	EXIT:	pointers in record relocated
;
;	USES:	none

relocThrd   proc

;*  NOTE: no pointers in thread record

	ret
relocThrd   endp


;***	relocLib - relocate library pointers
;
;	ENTRY:	(ds:bx) = record to relocate
;		   (dx) = relocation amount
;
;	EXIT:	pointers in record relocated
;
;	USES:	none

relocLib    proc

	add	[bx].l_phMTE,dx 	; relocate ptr to MTE table
	add	[bx].l_pName,dx 	; relocate ptr to module name
	ret
relocLib    endp

code	ends




data	segment

;***	NOTE:	3xBox has 0 for MTE handle!

testData    label   byte

	       ; type	      next  pid ppid sgid mte mCnt plMT cnt sm
r01	proc_s	<REC_PROCESS, r02,  2,	0,   2,   0,  0,   0,	0,  0>
r02	proc_s	<REC_PROCESS, r03,  3,	0,   0,   2,  0,   0,	0,  0>
r03	proc_s	<REC_PROCESS, r04,  4,	0,   0,   3,  0,   0,	0,  0>
r04	proc_s	<REC_PROCESS, r05,  5,	4,   1,   4,  0,   0,	0,  0>
r05	proc_s	<REC_PROCESS, r06,  6,	4,   3,   5,  0,   0,	0,  0>
r29	proc_s	<REC_PROCESS, r30, 10,	4,   4,  10,  0,   0,	0,  0>
r30	proc_s	<REC_PROCESS, r31, 20,	4,   5,  11,  0,   0,	0,  0>
r31	proc_s	<REC_PROCESS, r32, 30,	4,   6,  12,  0,   0,	0,  0>

	       ; type	      next  pid tid pri    state    blkID
r06	thrd_s	<REC_THREAD,  r07,  2,	1,  0200h, S_FROZE, 000000000h>
r07	thrd_s	<REC_THREAD,  r08,  3,	1,  0200h, S_BLOCK, 0f0000000h>
r08	thrd_s	<REC_THREAD,  r09,  3,	2,  0200h, S_BLOCK, 000000000h>
r09	thrd_s	<REC_THREAD,  r10,  3,	3,  0200h, S_BLOCK, 000000000h>
r10	thrd_s	<REC_THREAD,  r11,  3,	4,  021fh, S_BLOCK, 0fffe0002h>
r11	thrd_s	<REC_THREAD,  r12,  4,	1,  0d00h, S_BLOCK, 008f0051eh>
r12	thrd_s	<REC_THREAD,  r13,  4,	2,  0d00h, S_BLOCK, 008f00630h>
r13	thrd_s	<REC_THREAD,  r14,  4,	3,  0d00h, S_BLOCK, 00338002fh>
r14	thrd_s	<REC_THREAD,  r15,  4,	4,  0d00h, S_BLOCK, 0fffd0007h>
r15	thrd_s	<REC_THREAD,  r16,  5,	1,  0d00h, S_BLOCK, 0fffe0002h>
r16	thrd_s	<REC_THREAD,  r17,  6,	1,  0400h, S_BLOCK, 0fffe0002h>
r32	thrd_s	<REC_THREAD,  r33, 10,	1,  0200h, S_BLOCK, 00027abcdh>
r33	thrd_s	<REC_THREAD,  r34, 20,	1,  0200h, S_BLOCK, 0f0000000h>
r34	thrd_s	<REC_THREAD,  r35, 30,	1,  0200h, S_RUN,   000000000h>

	       ; type	      next  mte cnt    pMTE pName
r17	lib_s	<REC_LIBRARY, r18,  1,	m01C,  m01L, m01N>
r18	lib_s	<REC_LIBRARY, r19,  2,	m02C,  m02L, m02N>
r19	lib_s	<REC_LIBRARY, r20,  3,	m03C,  m03L, m03N>
r20	lib_s	<REC_LIBRARY, r21,  4,	m04C,  m04L, m04N>
r21	lib_s	<REC_LIBRARY, r22,  5,	m05C,  m05L, m05N>
r22	lib_s	<REC_LIBRARY, r23,  6,	m06C,  m06L, m06N>
r23	lib_s	<REC_LIBRARY, r24,  7,	m07C,  m07L, m07N>
r24	lib_s	<REC_LIBRARY, r25,  8,	m08C,  m08L, m08N>
r25	lib_s	<REC_LIBRARY, r26,  9,	m09C,  m09L, m09N>
r26	lib_s	<REC_LIBRARY, r27, 10,	m10C,  m10L, m10N>
r27	lib_s	<REC_LIBRARY, r28, 11,	m11C,  m11L, m11N>
r28	lib_s	<REC_LIBRARY, r29, 12,	m12C,  m12L, m12N>
r35	lib_s	<REC_LIBRARY, r36, 13,	m13C,  m13L, m13N>

r36	dw	 REC_EOF	; mark end of list



m01N	db	'bvscalls',0
m02N	db	'swapper.exe',0
m03N	db	'shell.exe',0
m04N	db	'harderr.exe',0
m05N	db	'cmd.exe',0
m06N	db	'doscalls.dll',0
m07N	db	'doscall1.dll',0
m08N	db	'viocalls.dll',0
m09N	db	'sesmgr.dll',0
m10N	db	'z.exe',0
m11N	db	'word.exe',0
m12N	db	'excel.exe',0
m13N	db	'bvscalls.dll',0

m01L	dw	6
m02L	dw	6,7
m03L	dw	6,7,9
m04L	dw	6,7
m05L	dw	6,7
m06L	dw	8
m07L	dw	6
m08L	dw	6,13
m09L	dw	6
m10L	dw	6
m11L	dw	6
m12L	dw	6
m13L	dw	6,8

m01C	equ	1
m02C	equ	2
m03C	equ	3
m04C	equ	2
m05C	equ	2
m06C	equ	1
m07C	equ	1
m08C	equ	2
m09C	equ	1
m10C	equ	1
m11C	equ	1
m12C	equ	1
m13C	equ	2

endTestData label   byte

data	ends

	end
