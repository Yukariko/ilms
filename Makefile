LIBPATH = /root/kangmin/leveldb-master
CPPFLAGS = -Wall -O -std=c++0x -g -L$(LIBPATH) -I $(LIBPATH)/include
LDFLAGS = -lleveldb

bin = ilms_show

t1 = show_id

obj = $(t1).o

all: $(bin)
$(bin): $(obj)
	$(CXX) $(CPPFLAGS) $(obj) $(LDFLAGS) -o $@

clean:
	rm -f $(bin) *.o