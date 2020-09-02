CXX=g++
CFLAGS=-Wall -Wextra -Wno-class-memaccess -Wno-unused-function -I./
DEFINES=-DGCC_COMPILE
LDFLAGS=-lglfw -lGL -lGLEW
OBJFILES=libs/imgui/*.o

debug:
	$(CXX) -g src/hamster.cpp $(OBJFILES) -o bin/hamster_debug -Werror $(CFLAGS) $(DEFINES) $(LDFLAGS)

fast:
	$(CXX) -O2 src/hamster.cpp $(OBJFILES) -o bin/hamster_fast $(CFLAGS) $(DEFINES) $(LDFLAGS)

asan:
	$(CXX) -g -fsanitize=address src/hamster.cpp $(OBJFILES) -o bin/hamster_debug -Werror $(CFLAGS) $(DEFINES) $(LDFLAGS)