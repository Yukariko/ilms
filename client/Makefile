CPPFLAGS = -Wall -O -std=c++0x -g

bin = ilmscli

t1 = ilmscli
t2 = main

obj = $(t1).o $(t2).o

all: $(bin)
$(bin): $(obj)
	$(CXX) $(CPPFLAGS) $(obj) -o $@

clean:
	rm -f $(bin) *.o