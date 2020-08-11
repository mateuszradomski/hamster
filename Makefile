CXX=g++
CFLAGS=-Wall -Wextra -pedantic -Werror -Wno-class-memaccess
LDFLAGS=-lglfw -lGL -lGLEW

debug:
	$(CXX) -g hamster.cpp -o hamster $(CFLAGS) $(LDFLAGS)

fast:
	$(CXX) -O2 hamster.cpp -o hamster $(CFLAGS) $(LDFLAGS)