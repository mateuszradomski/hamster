CXX=g++
CFLAGS=-Wall -Wextra -pedantic -Werror -Wno-class-memaccess
LDFLAGS=-lglfw -lGL -lGLEW

debug:
	$(CXX) -g src/hamster.cpp -o bin/hamster_debug $(CFLAGS) $(LDFLAGS)

fast:
	$(CXX) -O2 src/hamster.cpp -o bin/hamster_fast $(CFLAGS) $(LDFLAGS)