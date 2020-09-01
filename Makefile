CXX=g++
CFLAGS=-Wall -Wextra -Wno-class-memaccess -Wno-unused-function -I./
DEFINES=-DGCC_COMPILE
LDFLAGS=-lglfw -lGL -lGLEW

debug:
	$(CXX) -g src/hamster.cpp -o bin/hamster_debug -Werror $(CFLAGS) $(DEFINES) $(LDFLAGS)

fast:
	$(CXX) -O2 src/hamster.cpp -o bin/hamster_fast $(CFLAGS) $(DEFINES) $(LDFLAGS)

asan:
	$(CXX) -g -fsanitize=address src/hamster.cpp -o bin/hamster_debug -Werror $(CFLAGS) $(DEFINES) $(LDFLAGS)