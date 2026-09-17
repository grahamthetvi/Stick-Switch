CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -I../stick_switch/include

test_runner: test_stick_level.cpp ../stick_switch/include/stick_level.h
	$(CXX) $(CXXFLAGS) -o $@ test_stick_level.cpp

.PHONY: test clean
test: test_runner
	./test_runner

clean:
	rm -f test_runner
