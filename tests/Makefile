CXX=c++
CXXFLAGS=-std=c++11 -O2 -I..
OBJS=t42wrecomp.o t42wreexec.o

test : compile execute
	./compile
	./execute

compile : compile.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o compile compile.cpp $(OBJS)

execute : execute.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o execute execute.cpp $(OBJS)

t42wrecomp.o : ../t42wregex.hpp ../t42wrecomp.cpp
	$(CXX) $(CXXFLAGS) -c ../t42wrecomp.cpp

t42wreexec.o : ../t42wregex.hpp ../t42wreexec.cpp
	$(CXX) $(CXXFLAGS) -c ../t42wreexec.cpp

clean:
	rm -f *.o compile execute
