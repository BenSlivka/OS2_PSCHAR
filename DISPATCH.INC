;***	dispatch.inc - table dispatch macros
;
;	Author:
;	    Benjamin W. Slivka
;	    Microsoft Corporation
;	    (c) 1987


BREAK <dtB,dtS,dtE,dtDisp - Dispatch Table Macros>

;***	dtB - BEGIN dispatch table
;
;	ENTRY:	name = name of dt table
;		badAddr = local label for unspecified entries
;	EXIT:	Table name declared.
;		?sIndex = -1, indicates no entries specified yet
;		?badAddr set to default handler for unspecified entries
dtB	macro	name,badAddr		;; Beginning of Dispatch Table
    ?sIndex = -1
    ?badAddr	=   badAddr
    &name&DispTable label   word	;; Start of dispatch table
endm


;***	dtS - SPECIFIC dispatch table entry
;
;	ENTRY:	name  = name of dt table
;		index = index of entry
;		addr  = address of handler
;	EXIT:	Unspecified entries prior to "index" filled in;
;		Specified entry filled in.
;		?sIndex = next index to fill in;
;		?Low&name = lowest specified index, if first dtS
dtS	macro	name,index,addr 	;; Specific entry in Dispatch Table
    if ?sIndex eq -1			;; First entry in table
	Low&name    equ &index		;; Set lowest index
	?sIndex = index
    endif
    if	?sIndex gt index
	.err	;dtS: index out of order
    else
	if ?sIndex lt index		;; Fill unspecified entries
	    rept    index - ?sIndex
		dw  ?badAddr
	    endm
	endif
	dw	addr			;; Specified entry
	?sIndex = index+1		;; Set new start index
    endif
endm


;***	dtE - END dispatch table
;
;	ENTRY:	name  = name of dt table
;	EXIT:	?High&name defined as highest specified index
dtE	macro	name			;; End of Dispatch Table
    High&name	equ ?sIndex-1		;; Set highest iIndex
endm


;***	dtDisp - Make dispatch using dt table
;
;	ENTRY:	name = name of dt table
;		(BX) = index of handler to call
;
;	EXIT:	IF index in "reg" is in range THEN
;		    jmp to handler in table
;		ELSE
;		    continue execution at next instruction
;
;	USES:	BX,flags

dtDisp	macro	name
	local	a
	sub	bx,Low&Name		;; (bx) = index into table
	jb	short a 		;; Index is too small
	cmp	bx,High&Name - Low&Name ;;
	ja	short a 		;; Index is too large
	shl	bx,1
	jmp	cs:&name&DispTable[bx]	;; Go to handler
a:
endm
