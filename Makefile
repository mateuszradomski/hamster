CXX=g++
CFLAGS=-Wall -Wextra -Werror -Wno-class-memaccess -Wno-unused-function -I./
LDFLAGS=-lglfw -lGL -lGLEW

debug:
	$(CXX) -g src/hamster.cpp -o bin/hamster_debug $(CFLAGS) $(LDFLAGS)

fast:
	$(CXX) -O2 src/hamster.cpp -o bin/hamster_fast $(CFLAGS) $(LDFLAGS)