# Compiler to use
CXX = g++
# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++11
# Object files
OBJS = potato.o ringmaster.o player.o utils.o
# Targets
TARGETS = ringmaster player
all: $(TARGETS)
potato.o: potato.cpp potato.hpp
	$(CXX) $(CXXFLAGS) -c potato.cpp -o potato.o
ringmaster.o: ringmaster.cpp utils.hpp
	$(CXX) $(CXXFLAGS) -c ringmaster.cpp -o ringmaster.o
player.o: player.cpp utils.hpp
	$(CXX) $(CXXFLAGS) -c player.cpp -o player.o
utils.o: utils.cpp utils.hpp
	$(CXX) $(CXXFLAGS) -c utils.cpp -o utils.o
ringmaster: ringmaster.o potato.o utils.o
	$(CXX) $(CXXFLAGS) -o $@ ringmaster.o potato.o utils.o
player: player.o potato.o utils.o
	$(CXX) $(CXXFLAGS) -o $@ player.o potato.o utils.o
clean:
	rm -f $(TARGETS) $(OBJS)