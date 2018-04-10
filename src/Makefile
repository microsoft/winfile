CC = i686-w64-mingw32-gcc
CXX = i686-w64-mingw32-g++
AS = i686-w64-mingw32-as
LD = i686-w64-mingw32-ld
STRIP = i686-w64-mingw32-strip
WINDRES = i686-w64-mingw32-windres

LIBS = -lole32 -lshell32 -lshlwapi -lcomctl32 -lgdi32 -luser32 -lkernel32 -ladvapi32 -loleaut32
CFLAGS = -m32 -D_X86_ -D_M_IX86 -DWIN32 -D_WIN32 -D_WINDOWS -DLFN -DSTRICT \
	-DFASTMOVE -DLFNCLIPBOARD -DW3 -DUNICODE -D_UNICODE -DUSELASTDOT -DMEMDOUBLE

OBJECTS = dbg.o lfn.o lfnmisc.o numfmt.o suggest.o tbar.o treectl.o wfassoc.o \
	wfchgnot.o wfgoto.o wfdrop.o wfcomman.o wfcopy.o wfdir.o wfdirrd.o wfdirsrc.o wfdlgs.o \
	wfdlgs2.o wfdlgs3.o wfdos.o wfdrives.o wfext.o wffile.o wfinfo.o \
	wfinit.o wfmem.o wfprint.o wfsearch.o wftree.o wfutil.o winfile.o wnetcaps.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@
	
all: $(OBJECTS)
	$(WINDRES) res.rc -o res.o
	$(CXX) $(OBJECTS) $(LIBS) res.o -o Winfile.exe -static-libgcc -static-libstdc++

clean:
	rm -rf *.o *.exe
