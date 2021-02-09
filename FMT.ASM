	page	,132
title	fmt - Fast replacement for C run-time sprintf() function

;***	fmt.asm - Fast replacement for C run-time sprintf() function
;
;	This is a near-model (DS==SS) replacement for sprintf().
;
;	Author:
;	    Benjamin W. Slivka
;	    Microsoft Corporation
;	    Copyright (c)1987,1988
;
;	History:
;	    23-Sep-1987 Initial version.
;	    17-Oct-1987 Added %lx support.
;	    01-Sep-1988 Added %ld/u support!

.286p

include dosmac.inc
include dispatch.inc

fourbytes   struc
    loword  dw	?
    hiword  dw	?
fourbytes   ends


_TEXT	segment  byte public 'CODE'
_TEXT	ends


_TEXT	segment
    assume  cs:_TEXT

FALSE		    equ     0
NIBBLE_PER_LONG     equ     8		; Nibbles per long (dword)
DIGIT_PER_SHORT     equ     5		; Decimal digits per short (word)
DIGIT_PER_LONG	    equ    10		; Decimal digits per long (dword)

c_FMT		equ	'%'		; Format escape character
c_BLANK_FILL	equ	' '		; Blank Fill character
c_ZERO_FILL	equ	'0'		; Zero Fill character
c_NEGATIVE	equ	'-'		; Negative sign character

;*  rgulUnit - array of unit values for %ld/u formatting

rgulUnit    dd	    01h 		; 1L
	    dd	    0ah 		; 10L
	    dd	    064h		; 100L
	    dd	    03e8h		; 1000L
	    dd	    02710h		; 10000L
	    dd	    0186a0h		; 100000L
	    dd	    0f4240h		; 1000000L
	    dd	    0989680h		; 10000000L
	    dd	    05f5e100h		; 100000000L
	    dd	    03b9aca00h		; 1000000000L

cDigitsLong	equ	10		; number of digits in decimal ULONG


;*  FLAGDispTable - Dispatch table for FLAG state (must be sorted!)

	dtB	FLAG	    ,stateWIDTH
	dtS	FLAG,'-'    ,FLAGLeftJustify
	dtE	FLAG

;*  WIDTHDispTable - Dispatch table for WIDTH state (must be sorted!)

	dtB	WIDTH,	     stateSIZE
	dtS	WIDTH,c_FMT ,WIDTHFormat
	dtS	WIDTH,'*'   ,WIDTHVar
	dtS	WIDTH,'0'   ,WIDTHDigit
	dtS	WIDTH,'1'   ,WIDTHDigit
	dtS	WIDTH,'2'   ,WIDTHDigit
	dtS	WIDTH,'3'   ,WIDTHDigit
	dtS	WIDTH,'4'   ,WIDTHDigit
	dtS	WIDTH,'5'   ,WIDTHDigit
	dtS	WIDTH,'6'   ,WIDTHDigit
	dtS	WIDTH,'7'   ,WIDTHDigit
	dtS	WIDTH,'8'   ,WIDTHDigit
	dtS	WIDTH,'9'   ,WIDTHDigit
	dtE	WIDTH

;*  SIZEDispTable - Dispatch table for SIZE state (must be sorted!)

	dtB	SIZE	    ,stateTYPE
	dtS	SIZE,'h'    ,SIZEShort
	dtS	SIZE,'l'    ,SIZELong
	dtE	SIZE

;*  TYPEDispTable - Dispatch table for TYPE state (must be sorted!)

	dtB	TYPE	    ,stateSTART
	dtS	TYPE,'X'    ,TYPEHexUpper
	dtS	TYPE,'d'    ,TYPEDecimalSigned
	dtS	TYPE,'s'    ,TYPEString
	dtS	SIZE,'u'    ,TYPEDecimalUnsigned
	dtS	TYPE,'x'    ,TYPEHexLower
	dtE	TYPE

;*  XLAT table for 0..F to '0'..'f'
HexLowerXlat	label	byte
	irpc	a,0123456789abcdef
	db	'&a'
	endm

;*  XLAT table for 0..F to '0'..'F'
HexUpperXlat	label	byte
	irpc	a,0123456789ABCDEF
	db	'&a'
	endm

;***	strlen - compute length of string
;
;	ENTRY	(es:di) -> ASCIIZ string
;		CLD
;
;	EXIT	(cx) = string length (not counting null terminator)
;
;	USES	ax,cx,si,flags

strlen	macro
	mov	cx,-1			;; (cx) = search longest string
	mov	al,0			;; (al) = look for null
	repne scasb			;; Do search
	not	cx			;; (cx) = len + 1
	dec	cx			;; (cx) = len
endm


;***	GetArg - MACRO to get value of next argument
;
;	ENTRY	fmt_pArg points to next argument
;		<reg> = register to receive argument
;
;	EXIT	(reg) = value of argument
;		fmt_pArg points to next argument
;
;	USES	<reg>,bx,flags

GetArg	macro	reg
	mov	bx,fmt_pArg		;; (bx) = arg pointer
	inc	bx
	inc	bx			;; (bx) = next arg ptr
	mov	fmt_pArg,bx		;; Update arg pointer
	mov	reg,ss:[bx-2]		;; (reg) = arg value
endm


;***	fmt - fast subset of C sprintf() library routine
;
;	ENTRY	fmt (pszBuf, pszFmt, arg1, arg2, ...);
;
;	EXIT	pszBuf filled in with formatted data.
;		(ax) = pszBuf
;
;	USES	ax,bx,cx,dx,es,flags
;
;	Supported formats:
;	    %[-][0n | n | *][h | l][d | s | u | x| X]
;	    %%

Procedure   _fmt
	ArgVar	    fmt_pszBuf,WORD
	ArgVar	    fmt_pszFmt,WORD
	ArgVar	    fmt_arg1,WORD

	LocalVar    fmt_pArg,WORD	; argument pointer
	LocalVar    fmt_width,WORD	; format width
	LocalVar    fmt_sHigh,WORD	; high word of long value
	LocalVar    fmt_chFill,BYTE	; fill character
	LocalVar    fmt_fLong,BYTE	; TRUE => long data
	LocalVar    fmt_fLeft,BYTE	; TRUE => left justify
	LocalVar    fmt_fWidth,BYTE	; TRUE => loop in WIDTH state
	LocalVar    fmt_chSign,BYTE	; sign of decimal value
	LocalVar    fmt_chDigit,BYTE	; digit being built for %ld/u
	LocalVar    fmt_rgch,DIGIT_PER_LONG ; temp formatting buffer
	EnterProc
	SaveReg <di,si>
	mov	di,fmt_pszBuf		; (di) = pszBuf
	mov	si,fmt_pszFmt		; (si) = pszFmt
	lea	bx,fmt_arg1		; (bx) -> arg1
	mov	fmt_pArg,bx		; Save arg pointer
	push	ds
	pop	es			; (es) = DGROUP
	mov	bx,offset cs:stateSTART ; (bx) = initial state

;*  (ds:si) -> format string
;   (es:di) -> destination buffer
;   (ss:bp) -> next arg
;      (bx) -> state

	push	di			; save pszBuf

fmt10:	lodsb				; (al) = char
	or	al,al			; End of string?
	jz	fmt20			;   YES, go exit

	call	bx			; Execute current state
	jmp	fmt10			; Get next character

fmt20:	stosb				; terminate string
	pop	ax			; (ax) = pszBuf
	RestoreReg <si,di>
	LeaveProc
	ret

EndProc     _fmt


;***	stateSTART - the START state
;
;	ENTRY:	(al)	=  char
;		(ds:si) -> format string
;		(es:di) -> destination buffer
;
;	EXIT:	(bx) = new state
;
;	USES:	bx,di,flags

Procedure   stateSTART

	cmp	al,c_FMT		; Format escape character?
	je	ss10			;   YES, go change state

	stosb				;   NO, copy character
	mov	bx,offset cs:stateSTART ; (bx) = remain in same state
	ret

;*  Initialize defaults

ss10:	xor	ax,ax
	mov	fmt_width,ax		; default width
	mov	fmt_chFill,c_BLANK_FILL ; default fill character
	mov	fmt_fLeft,al		; Right justify
.errnz	FALSE
	mov	fmt_fLong,al		; Not long
.errnz	FALSE
	mov	fmt_fWidth,al		; No width, yet
.errnz	FALSE
	mov	bx,offset cs:stateFLAG	; (bx) = go to FLAG state
	ret
EndProc     stateSTART


;***	stateFLAG - the FLAG state
;
;	ENTRY:	(al)	=  char
;		(ds:si) -> format string
;		(es:di) -> destination buffer
;
;	EXIT:	(bx) = new state
;
;	USES:	bx,di,flags

Procedure   stateFLAG

	mov	bl,al
	mov	bh,0			; (bx) = character
	dtDisp	FLAG			; Dispatch based on character
	jmp	short stateWIDTH

FLAGLeftJustify:    ;*	Left justification
	mov	fmt_fLeft,-1		; Select left justification
	mov	bx,offset cs:stateFLAG	; (bx) = go to SIZE state
	ret

EndProc     stateFLAG


;***	stateWIDTH - the WIDTH state
;
;	ENTRY:	(al)	=  char
;		(ds:si) -> format string
;		(es:di) -> destination buffer
;
;	EXIT:	(bx) = new state
;
;	USES:	bx,di,flags

Procedure   stateWIDTH

	mov	bl,al
	mov	bh,0			; (bx) = character
	dtDisp	WIDTH			; Dispatch based on character
	jmp	short stateSIZE

WIDTHFormat:	;*  A format escape character
	stosb				; Copy format escape to buffer
	mov	bx,offset cs:stateSTART ; (bx) = go to START state
	ret

WIDTHDigit:	;*  A digit
	mov	cl,al			; (cl) = ASCII form of digit
	mov	ch,0			; (cx) = ASCII form of digit
	sub	cx,'0'			; (cx) = binary form of digit
	jnz	wd10			; Not zero

	test	fmt_fWidth,-1		; Is this the leading zero?
	jnz	wd10			;   NO, don't change fill char

	mov	fmt_chFill,c_ZERO_FILL	;   YES, change fill char

wd10:	mov	fmt_fWidth,TRUE 	; Have entered WIDTH at least once
	mov	ax,fmt_width		; (ax) = current width
	mov	bl,10			; (bl) = base
	mul	bl			; (ax) = 10*width
	add	ax,cx			; (ax) = 10*width+digit
	mov	fmt_width,ax		; Update width
	mov	bx,offset cs:stateWIDTH ; (bx) = stay in same state
	ret

WIDTHVar:	;*  use next argument as width
	GetArg	ax			; (ax) = argument word
	mov	fmt_width,ax		; Save new width
	mov	bx,offset cs:stateSIZE	; (bx) = go to SIZE state
	ret

EndProc     stateWIDTH


;***	stateSIZE - the SIZE state
;
;	ENTRY:	(al)	=  char
;		(ds:si) -> format string
;		(es:di) -> destination buffer
;
;	EXIT:	(bx) = new state
;
;	USES:	bx,di,flags

Procedure   stateSIZE

	mov	bl,al
	mov	bh,0			; (bx) = character
	dtDisp	SIZE			; Dispatch based on character
	jmp	short stateTYPE

SIZELong:	;*  data value is a long (DWORD)
	mov	fmt_fLong,TRUE		; Remember long
	mov	bx,offset cs:stateTYPE	; (bx) = go to TYPE state
	ret

SIZEShort:	;*  data value is a short (WORD)
	mov	fmt_fLong,FALSE 	; Remember short
	mov	bx,offset cs:stateTYPE	; (bx) = go to TYPE state
	ret

EndProc     stateSIZE


;***	stateTYPE - the TYPE state
;
;	ENTRY:	(al)	=  char
;		(ds:si) -> format string
;		(es:di) -> destination buffer
;
;	EXIT:	(bx) = new state
;
;	USES:	bx,di,flags

Procedure   stateTYPE

	mov	bl,al
	mov	bh,0			; (bx) = character
	dtDisp	TYPE			; Dispatch based on character
	jmp	stateSTART

tu_rgch db	'** unsupported **'
tu_cch	equ	$-tu_rgch

TYPEUnsupported:
	SaveReg <si,ds>
	push	cs
	pop	ds
	mov	si,offset cs:tu_rgch	; (ds:si) = message
	mov	cx,tu_cch		; (cx) = len
	rep movsb			; Copy message to output buffer
	RestoreReg <ds,si>
	mov	bx,offset cs:stateSTART ; (bx) = go to START state
	ret


TYPEDecimalUnsigned:	;*  format as unsigned decimal number
	xor	cx,cx			; (cx) = unsigned
	jmp	short td10		; Go to common decimal code

TYPEDecimalSigned:	;*  format as signed decimal number
	mov	cx,-1			; (cx) = signed

;*  ENTRY:  (bx)    = arg ptr
;	    (cx)    =  0 ==> unsigned
;		    = -1 ==> signed
;	    (es:di) = output buffer

td10:	SaveReg <si>
	test	fmt_fLong,-1		; Long data?
	jnz	td20			;   YES, do long format
	jmp	td100			;   NO, do short format

;*  Do long format

td20:	SaveReg <di>
	GetArg	ax
	GetArg	dx			; dx:ax = long

	mov	bl,c_BLANK_FILL 	; (bl) = default sign character
	jcxz	td30			; Unsigned format

	or	dx,dx			; Is value negative?
	jns	td30			;   NO, treat as unsigned

	neg	ax
	adc	dx,0
	neg	dx			; dx:ax = positive value

	mov	bl,c_NEGATIVE		; (bl) = sign character

td30:	mov	fmt_chSign,bl		; Save sign
	mov	cx,ax			; dx:cx = positive value
	mov	si,offset rgulUnit[4*(cDigitsLong-1)] ; cs:si -> units
	lea	di,fmt_rgch		; ss:di -> temp buffer

;** Fast skip to highest unit
;
;   (ss:di) -> destination buffer
;   (cs:si) -> unit table
;   (dx:cx) = ulong to translate

td40:	mov	bx,cs:[si].hiword	; bx = high word of unit
	cmp	dx,bx			; number > unit
	jb	td50			;   number < unit, continue loop
	ja	td60			;   Yes, go start tranlating

	mov	ax,cs:[si].loword	; ax = low word of unit value
	cmp	cx,ax			; number >= unit
	jae	td60			;   Yes, go start translating

td50:	sub	si,4			; (ds:si) -> next unit
	cmp	si,offset rgulUnit	; done with units?
	jae	td40			;   No, continue loop
	mov	fmt_chDigit,'0' 	; digit = '0'
	jmp	td90			; store 0 and exit

;** Now do translation
;
;   (ss:di) -> destination buffer
;   (cs:si) -> unit table
;   (dx:cx) = remainder of ulong to translate

td60:	mov	ax,cs:[si].loword
	mov	bx,cs:[si].hiword	; bx:ax = unit value
	mov	fmt_chDigit,'0' 	; digit = '0'
	jmp	td80			; enter loop at test

td70:	inc	fmt_chDigit		; ch++
	sub	cx,ax
	sbb	dx,bx			; ul -= ulUnit

td80:	cmp	dx,bx			; number > unit
	ja	td70			;   Yes, continue loop
	jb	td90			;   number < unit, exit loop
	cmp	cx,ax			; number >= unit
	jae	td70			;   Yes, continue loop

td90:	mov	al,fmt_chDigit		; (al) = digit
	mov	ss:[di],al		; store char
	inc	di			; (di) = next char position
	sub	si,4			; (ss:si) -> next unit

	cmp	si,offset rgulUnit	; Done with units?
	jae	td60			;   No, continue loop

	lea	si,fmt_rgch		; (ss:si) -> temp buffer start
	xchg	cx,di			; (ss:cx) -> last digit + 1
	sub	cx,si			; (cx) = number of digits formatted
	RestoreReg  <di>
	jmp	td130			; go finish formatting


;*  Format short decimal

td100:	GetArg	dx			; (dx) = low word of value to format
	mov	bl,c_BLANK_FILL 	; (bl) = default sign character
	jcxz	td110			; Unsigned format

	or	dx,dx			; Is value negative?
	jns	td110			;   NO, treat as unsigned

	neg	dx			; (dx) = positive value
	mov	bl,c_NEGATIVE		; (bl) = sign character

td110:	mov	fmt_chSign,bl		; Save sign
	lea	si,fmt_rgch		; (ss:si) -> stack buffer
	add	si,DIGIT_PER_SHORT-1	; (ss:si) -> last char of stack buffer
	xor	cx,cx			; (cx) = count of chars formatted
	mov	bx,10			; (bx) = number base

;*  (ss:si)  -> scratch buffer
;   (cx)      = count of chars formatted so far
;   (dx)      = value being formatted
;   (bx)      = 10 (number base)
;   (es:di)   = destination buffer

td120:	xor	ax,ax			; (ax:dx) = value
	xchg	dx,ax			; (dx:ax) = value
	div	bx
	xchg	dx,ax			; (dx) = quotient
					; (ax) = remainder
	add	al,'0'			; (al) = character
	mov	ss:[si],al		; store char
	dec	si			; (si) = next char position
	inc	cx			; (cx) = count of chars formatted

	or	dx,dx			; Value zero?
	jnz	td120			;   NO, more to format

	inc	si			; (ss:si) -> formatted value

;*  We now have the digits formatted.  Figure out what to do about a sign
;   character and filling.
;
;   (ss:si) -> most-significant digit
;      (cx)  = number of digits
;
;    7654321	 7654321
;   > -32768<	>-032768<
;   >  -4321<	>-004321<
;   >	-321<	>-000321<
;   >	 -21<	>-000021<
;   >	  -1<	>-000001<
;   >	   0<	>-000000<
;   >	  -1<	>0000001<
;   >	 -21<	>0000021<
;   >	-321<	>0000321<
;   >  -4321<	>0004321<
;   > -32767<	>0032767<

td130:	cmp	cx,fmt_width		; Is fmt_width large enough?
	jbe	td140			;   YES, continue

	mov	fmt_width,cx		; Set width to size of formatted value

td140:	mov	bl,fmt_chSign		; (bl) = sign
	cmp	bl,c_NEGATIVE		; Negative number?
	jne	td150			;   NO, don't bother with sign

;*  NEGATIVE number

	cmp	fmt_chFill,c_BLANK_FILL ; Blank fill?
	je	td160			;   YES, do fill then sign

;*  NEGATIVE number with ZERO fill

	mov	al,bl			; (al) = sign character
	stosb				; Store sign
	dec	fmt_width		; Account for sign

;*  POSITIVE number, or NEGATIVE number with ZERO fill

td150:	cmp	cx,fmt_width		; Need to fill?
	jae	td180			;   NO, go copy formatted value

	mov	bx,cx			; (bx) = save formatted length
	sub	cx,fmt_width		; (cx) = -(chars to fill)
	neg	cx			; (cx) = chars to fill
	mov	al,fmt_chFill		; (al) = fill character
	rep stosb			; Do fill
	mov	cx,bx			; (cx) = formatted length
	jmp	td180

;*  NEGATIVE number with BLANK fill

td160:	mov	bx,cx			; (bx) = number of formatted digits
	inc	bx			; (bx) = digits + sign char
	cmp	bx,fmt_width		; Need to fill?
	jae	td170			;   NO, skip fill

	xchg	bx,cx			; (bx) = save formatted length
	sub	cx,fmt_width		; (cx) = -(chars to fill)
	neg	cx			; (cx) = chars to fill
	mov	al,fmt_chFill		; (al) = fill character
	rep stosb			; Do fill
	mov	cx,bx			; (cx) = formatted length

td170:	mov	al,fmt_chSign		; (al) = sign character
	stosb				; Store sign

td180:	rep movsb			; Copy formatted value
	RestoreReg <si>
	mov	bx,offset cs:stateSTART ; (bx) = go to START state
	ret


TYPEHexUpper:	;*  format as hexidecimal number with UPPERCASE letters
	push	offset CS:HexUpperXlat	; Save xlat table address
	jmp	short th05

TYPEHexLower:	;*  format as hexidecimal number with LOWERCASE letters
	push	offset CS:HexLowerXlat	; Save xlat table address

th05:	GetArg	dx			; (dx) = low word of value to format
	xor	ax,ax			; (ax) = assume high word is zero
	test	fmt_fLong,-1		; Long data?
	jz	th07			;   NO, high word is zero

	GetArg	ax			; (ax) = high word of long value

th07:	mov	fmt_sHigh,ax		; Save high word
	pop	bx			; (cs:bx) -> xlat table
	SaveReg <si>
	cmp	fmt_width,NIBBLE_PER_LONG ; Field width > max width
	jbe	th10			;   NO, go format number

	mov	cx,NIBBLE_PER_LONG	; (cx) = max non-zero digits
	xchg	cx,fmt_width		; (cx) = field width; set new "width"
	sub	cx,NIBBLE_PER_LONG	; (cx) = space to fill
	mov	al,fmt_chFill		; (al) = fill character
	rep stosb			; Do fill

th10:	mov	al,fmt_chFill		; (al) = fill character
	mov	ah,al			; (ah) = fill character
	push	ax
	push	ax
	push	ax
	push	ax			; Make 8-byte "filled" buffer
.errnz	NIBBLE_PER_LONG-8
	mov	si,sp			; (ss:si) -> stack buffer
	add	si,NIBBLE_PER_LONG-1	; (ss:si) -> last char of stack buffer
	xor	cx,cx			; (cx) = count of chars formatted

;*  (ss:si)  -> scratch buffer
;   (cx)      = count of chars formatted so far
;   fmt_sHigh = high word of value being formatted
;   (dx)      = low word of value being formatted
;   (cs:bx)  -> xlat table
;   (es:di)   = destination buffer

th20:	mov	al,dl			; (al) = value
	and	al,0Fh			; (al) = low nibble
	xlat	cs:[bx] 		; (al) = character
	mov	ss:[si],al		; store char
	dec	si			; (si) = next char position
	inc	cx			; (cx) = count of chars formatted

;*  Shift long value fmt_sHigh:dx right by 1 nibble (hex digit)

					; (dx) = efgh
	shr	dx,4			; (dx) = 0efg
					; fmt_sHigh = abcd
	ror	fmt_sHigh,4		; fmt_sHigh = dabc
	mov	ax,fmt_sHigh		; (ax) = dabc
	and	fmt_sHigh,0FFFh 	; fmt_sHigh = 0abc
	and	ax,0F000h		; (ax) = d000
	or	dx,ax			; (dx) = defg
	jnz	th20			; Value not zero, more to format

	cmp	cx,fmt_width		; Do we need to fill?
	jae	th30			;   NO, length >= field width

	mov	ax,fmt_width		; (ax) = field width
	sub	ax,cx			; (ax) = amount to fill
	sub	si,ax			; (si) = adjust for fill
	add	cx,ax			; (cx) = total count

th30:	inc	si			; (ss:si) = formatted value
	rep movsb
	add	sp,NIBBLE_PER_LONG	; Remove scratch buffer from stack
	RestoreReg <si>
	mov	bx,offset cs:stateSTART ; (bx) = go to START state
	ret


TYPEString:	;*  format as string
	SaveReg <si>
	SaveReg <di>
	GetArg	di			; (di) = arg, ptr to string
	mov	bx,di			; (bx) = start of string
	strlen				; (cx) = string length
	mov	si,bx			; (si) = start of string
	RestoreReg <di>
	mov	bx,cx			; (bx) = string length
	sub	cx,fmt_width		; (cx) = -(fill length)
	neg	cx			; (cx) = fill length
	mov	al,fmt_chFILL		; (al) = fill character
	test	fmt_fLeft,-1		; Left justify?
	jnz	ts10			;   YES, do copy, then fill

;*  Right justify.  Do Fill, then copy

	or	cx,cx			; Any fill characters?
	jle	ts10			;   NO, do copy

	rep stosb			; Do fill
	xor	cx,cx			; (cx) = no more fill characters
					; BUGBUG: chip bug, make sure cx=0

ts10:	xchg	cx,bx			; (cx) = string length
					; (bx) = fill length
	rep movsb			; Copy string
	mov	cx,bx			; (cx) = fill length
	or	cx,cx			; Any fill characters?
	jle	ts20			;   NO, go exit

	rep stosb			; Do fill

ts20:	RestoreReg <si>
	mov	bx,offset cs:stateSTART ; (bx) = go to START state
	ret

EndProc     stateTYPE

_TEXT	ends

	end
