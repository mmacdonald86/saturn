CC = g++
CCFLAGS = -Wall -Wextra -Wfatal-errors -flto
LIBS = -lavrocpp -flto

# -flto : link-time optimizations; needs to be passed to both compile and link commands.

TARGETS = libsaturn.so do_svr

all: $(TARGETS)

libsaturn.so: src/feature_engine.cc src/svr_model.cc src/utils.cc
	$(CC) -std=c++17 $(CCFLAGS) -Iinclude -fPIC -shared $^ $(LIBS) -o $@

do_svr: benchmarks/do_svr.cc
	$(CC) $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o do_svr

clean:
	rm -f *.o
	rm -f *.so
	rm -f do_svr

