SRCS = \
	dbg.c \
	lfn.c \
	lfnmisc.c \
	numfmt.c \
	suggest.c \
	tbar.c \
	treectl.c \
	wfassoc.c \
	wfchgnot.c \
	wfcomman.c \
	wfcopy.c \
	wfdir.c \
	wfdirrd.c \
	wfdirsrc.c \
	wfdlgs.c \
	wfdlgs2.c \
	wfdlgs3.c \
	wfdos.c \
	wfdrives.c \
	wfdrop.c \
	wfext.c \
	wffile.c \
	wfinfo.c \
	wfinit.c \
	wfmem.c \
	wfloc.c \
	wfprint.c \
	wfsearch.c \
	wftree.c \
	wfutil.c \
	winfile.c \
	wnetcaps.c

OBJS = $(subst .c,.o,$(SRCS)) wfgoto.o res.o

CFLAGS = -DUNICODE -DFASTMOVE -DSTRSAFE_NO_DEPRECATE
LIBS = -mwindows -lgdi32 -lcomctl32 -lole32 -lshlwapi -loleaut32 -lversion
TARGET = winfile
ifeq ($(OS),Windows_NT)
TARGET := $(TARGET).exe
endif

.PHONY: all depend clean
.SUFFIXES: .c .cpp .o .res

all : $(TARGET)

$(TARGET) : $(OBJS)
	g++ -o $@ $(OBJS) $(LIBS)

.c.o :
	gcc -c $(CFLAGS) -I. $< -o $@

.cpp.o :
	g++ -c $(CFLAGS) -I. $< -o $@

res.o : res.rc lang/*.rc lang/*.dlg
	windres -DNOWINRES -I. -i res.rc -o res.o

clean :
	rm -f $(OBJS) $(TARGET)

depend:
	gcc -E -MM -w *.c > Makefile.depends

-include Makefile.depends
