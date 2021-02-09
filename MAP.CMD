    t1:= arg replace "DOSEXIT"		arg newline "DosExit"	       arg newline
    t2:= arg replace "DOSGETINFOSEG"	arg newline "DosGetInfoSeg"    arg newline
    t3:= arg replace "DOSQPROCSTATUS"	arg newline "DosQProcstatus"   arg newline
    t4:= arg replace "DOSWRITE" 	arg newline "DosWrite"	       arg newline
    t5:= arg replace "DOSSEMSETWAIT"	arg newline "DosSemSetWait"    arg newline
    t6:= arg replace "DOSCREATETHREAD"	arg newline "DosCreateThread"  arg newline
    t7:= arg replace "DOSSETSIGHANDLER" arg newline "DosSetSigHandler" arg newline
    t8:= arg replace "DOSSEMREQUEST"	arg newline "DosSemRequest"    arg newline
    t9:= arg replace "DOSSEMCLEAR"	arg newline "DosSemClear"      arg newline
    ta:= arg replace "DOSSEMSET"	arg newline "DosSemSet"        arg newline
    tb:= arg replace "DOSSEMWAIT"	arg newline "DosSemWait"       arg newline
    translate:= t1 t2 t3 t4 t5 t6 t7 t8 t9 ta tb
    translate:alt+k
