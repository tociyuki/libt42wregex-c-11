test : compile execute
	./compile
	./execute

compile : compile.cpp t42wrecomp.o t42wreexec.o
	c++ -std=c++11 -I.. -o compile compile.cpp t42wrecomp.o t42wreexec.o

execute : execute.cpp t42wrecomp.o t42wreexec.o
	c++ -std=c++11 -I.. -o execute execute.cpp t42wrecomp.o t42wreexec.o

t42wrecomp.o : ../t42wregex.hpp ../t42wrecomp.cpp
	c++ -std=c++11 -I.. -c ../t42wrecomp.cpp

t42wreexec.o : ../t42wregex.hpp ../t42wreexec.cpp
	c++ -std=c++11 -I.. -c ../t42wreexec.cpp

clean:
	rm -f *.o compile execute
