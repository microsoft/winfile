#
#   Makefile for Sample Application
#

CC     =   cl -c -AM -Gsw -nologo -Od -W3 -Zpe
LN     =   link /m/NOD/NOE/NOL/A:16

OBJ    =   main.obj init.obj bmp.obj exehdr.obj iconcur.obj filllb.obj menudlg.obj res.obj

.c.obj      :
				echo Compiling: $*.c
				$(CC) -NT $* $*.c >$*.err
				type $*.err
				del $*.err

exeview.exe :   $(OBJ) main.res main.def
				echo Linking: main.exe
				$(LN) @<<
				 main.obj init.obj bmp.obj exehdr.obj+
				 iconcur.obj filllb.obj menudlg.obj res.obj
				 exeview.exe
				 exeview.map
				 commdlg mlibcew libw
				 main.def
<<
				echo Adding Resources
				rc -v main.res exeview.exe

main.res    :   main.rc global.h main.ico
				echo RCing: main.rc
				rc -r main.rc

clean:
    -del *.obj
    -del *.res
    -del *.map
    -del *.sym
