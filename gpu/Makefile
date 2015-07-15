LIBPATH = /home/jo/leveldb-master
CPPFLAGS = -Wall -O -std=c++0x -g -L$(LIBPATH) -I $(LIBPATH)/include -I/usr/local/cuda/include/
LDFLAGS = -lleveldb  -L /usr/local/cuda/lib64 -lcudart

NVCC = nvcc


bin = ilms

t1 = topology
t2 = ilms
t3 = bloomfilter
t4 = main
t5 = scanner

obj = $(t1).o $(t2).o $(t4).o $(t5).o
obj2 = $(t3).o

all: $(bin)

$(bin): $(obj) $(obj2)
	$(CXX) $(CPPFLAGS) $(obj) $(obj2) $(LDFLAGS) -o $@

$(obj2): $(t3).cu
	$(NVCC) -c $^ -o $@ -std=c++11

#$(obj): $(t1).cpp $(t2).cpp $(t4).cpp $(t5).cpp $(obj2)
#	$(CXX) $(CPPFLAGS) $^  $(LDFLAGS) -o $@

clean:
	rm -f $(bin) *.o
