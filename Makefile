CPPFLAGS = -Wall -O -g
bin = ilms

t1 = topology
t2 = ilms
t3 = bloomfilter
t4 = main

obj = $(t1).o $(t2).o $(t3).o $(t4.o)

all: $(bin)
$(bin): $(obj)
	$(CXX) $(CPPFLAGS) $(obj) -o $@

clean:
	rm -f $(bin) *.o