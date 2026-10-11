CXX ?= c++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra -Wpedantic

bonus: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o bonus

test: bonus
	sh tests.sh

demo: bonus
	./bonus --demo

bench: bonus
	./bonus --bench

clean:
	rm -f bonus

.PHONY: test demo bench clean
