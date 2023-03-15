CXX = g++
CXXFLAGS = -Wall -Werror -pedantic -Og -g -std=c++17 -fsanitize=address -fsanitize=undefined -D_GLIBCXX_DEBUG

all: msh

msh: main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f *.o msh

.PHONY: clean all
