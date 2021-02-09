if "%1" == "" goto notest

:test
nmake -i GCFLAGS="-DCheckAsserts -DDEBUG -Zi" GLFLAGS="/CO /LI"
goto exit

:notest
nmake -i

:exit
