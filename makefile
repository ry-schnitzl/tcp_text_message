# CXX Make variable for compiler
CC=gcc
# -std=c++11  C/C++ variant to use, e.g. C++ 2011
# -Wall       show the necessary warning files
# -g3         include information for symbolic debugger e.g. gdb
CCFLAGS=-std=c11 -Wall -g3 -c

# object files
OBJS = tcp_client.o tcp_server.o main.o

# Program name
PROGRAM = tcp_text_message

# Rules format:
# target : dependency1 dependency2 ... dependencyN
#     Command to make target, uses default rules if not specified

# First target is the one executed if you just type make
# make target specifies a specific target
# $^ is an example of a special variable.  It substitutes all dependencies
$(PROGRAM) : $(OBJS)
	$(CC) -o $(PROGRAM) $^

tcp_server.o : tcp_server.h tcp_server.c
	$(CC) $(CCFLAGS) tcp_server.c

tcp_client.o : tcp_client.h tcp_client.c tcp_server.c
	$(CC) $(CCFLAGS) tcp_client.c

main.o : main.c tcp_client.c tcp_server.c
	$(CC) $(CCFLAGS) main.c

clean :
	rm -f *.o $(PROGRAM)