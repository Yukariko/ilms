INCLUDEDIRS = \
		-I$(ARCENGINEHOME)/include \
		-I/usr/X11R6/include \
		-I/usr/include/qt5 \
		-I/usr/include/qt5/QtCore \
		-I/usr/local/cuda/include


LIBDIRS = \
		-L$(ARCENGINEHOME)/bin \
		-L/usr/X11R6/lib \
		-L/usr/local/cuda/lib64

LIBS = \
		-lQt5Core -lleveldb -lcudart -lrt

CPPFLAGS = -Wall -O -std=c++0x -g -DESRI_UNIX $(INCLUDEDIRS) -fPIE
LDFLAGS =  $(LIBDIRS) $(LIBS)
NVCC = nvcc

bin = ilms

t1 = topology
t2 = ilms
t3 = bloomfilter
t4 = main
t5 = scanner
t6 = address

obj = $(t1).o $(t2).o $(t4).o $(t5).o $(t6).o
obj2 = $(t3).o

all: $(bin)
$(bin): $(obj) $(obj2)
		$(CXX) $(CPPFLAGS) $(obj) $(obj2) $(LDFLAGS) -o $@

$(obj2): $(t3).cu
	$(NVCC) -c $^ -o $@ -std=c++11

clean:
		rm -f $(bin) *.o
