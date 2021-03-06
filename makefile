#cacheSim: cacheSim.cpp
#	g++ -std=c++11 -o cacheSim cacheSim.cpp
#
#.PHONY: clean
#clean:
#	rm -f *.o
#	rm -f cacheSim
	
	
appname := cacheSim
CXX := g++
CXXFLAGS := -std=c++11 -Wall 

srcfiles := $(shell find . -maxdepth 1 -name "*.cpp")
objects  := $(patsubst %.cpp, %.o, $(srcfiles))

all: $(appname)

$(appname): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname) $(objects) $(LDLIBS) 

depend: .depend

.depend: $(srcfiles)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	rm -f $(objects)

dist-clean: clean
	rm -f *~ .depend

include .depend