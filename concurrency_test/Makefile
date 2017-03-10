BGThreads := BGThreads.cc
src := test_for_BGThreads.cc
cxxflags := -I.
ldflags := -pthread
cc := g++
target := test

all: BGTest
BGTest:
	$(cc) $(cxxflags) $(BGThreads) $(src) -o $(target) $(ldflags)

.PHONY:
clean:
	rm -rf *.o $(target)
