CC = g++
CCFLAGS = -std=c++11 -Wall -Wextra -Wfatal-errors
INCLUDES = -I../../include
LIBS = -rpath ../.. -lsaturn -lavrocpp

# -flto : link-time optimizations; needs to be passed to both compile and link commands.

all: do_svr

do_svr: %.cc
	$(CC) $(CCFLAGS) $(INCLUDES) -flto $^ $(LIBS) -o $@

clean:
	rm -f *.o
	rm -f *.so
	rm -f do_svr
