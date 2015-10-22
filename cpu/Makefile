INCLUDEDIRS = \
    -I$(ARCENGINEHOME)/include \
    -I/usr/X11R6/include \
    -I/usr/include/qt5 \
    -I/usr/include/qt5/QtCore


LIBDIRS = \
    -L$(ARCENGINEHOME)/bin \
    -L/usr/X11R6/lib

LIBS = \
    -lQt5Core -lleveldb

CPPFLAGS = -Wall -O -std=c++0x -g -DESRI_UNIX $(INCLUDEDIRS) -fPIE
LDFLAGS =  $(LIBDIRS) $(LIBS)

bin = ilms

t1 = topology
t2 = ilms
t3 = bloomfilter
t4 = main
t5 = scanner
t6 = address

obj = $(t1).o $(t2).o $(t3).o $(t4).o $(t5).o $(t6).o

all: $(bin)
$(bin): $(obj)
    $(CXX) $(CPPFLAGS) $(obj) $(LDFLAGS) -o $@

clean:
    rm -f $(bin) *.o
