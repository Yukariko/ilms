INCLUDEDIRS = \
		-I$(ARCENGINEHOME)/include \
		-I/usr/X11R6/include \
		-I/usr/include/qt5 \
		-I/usr/include/qt5/QtCore


LIBDIRS = \
		-L$(ARCENGINEHOME)/bin \
		-L/usr/X11R6/lib

LIBS = \
		-lQt5Core

CPPFLAGS = -Wall -O -std=c++0x -g -DESRI_UNIX $(INCLUDEDIRS) -fPIE
LDFLAGS =  $(LIBDIRS) $(LIBS)

bin = ilmscli

t1 = ilmscli
t2 = address

obj = $(t1).o $(t2).o

all: $(bin)
$(bin): $(obj)
		$(CXX) $(CPPFLAGS) $(obj) $(LDFLAGS) -o $@

clean:
		rm -f $(bin) *.o
