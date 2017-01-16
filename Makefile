CXX = g++
CXXFLAGS = -O3 -std=c++14

all: Main.o RAS.o Pipe.o 
	${CXX} $^ ${CXXFLAGS} -o RASserver

%.o: %.cpp
	${CXX} $< ${CXXFLAGS} -c

clean:
	rm -rf *.o RASserver	