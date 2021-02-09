del *.obj
nmake -i GCFLAGS="-DCheckAsserts"
ls -l *.obj >sizes

del *.obj
nmake -i
ls -l *.obj >>sizes
