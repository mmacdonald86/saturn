CC = g++
CCFLAGS = -Wall -Wextra -Wfatal-errors -flto
LIBS = -lavrocpp -flto

# -flto : link-time optimizations; needs to be passed to both compile and link commands.

TARGETS = libsaturn.so test_svr latency run_saturn

all: $(TARGETS)

libsaturn.so: src/feature_engine.cc src/svr_model.cc src/utils.cc
	$(CC) -std=c++17 $(CCFLAGS) -Iinclude -fPIC -shared $^ $(LIBS) -o $@

test_svr: tests/test_svr.cc
	$(CC) -std=c++11 $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o test_svr

latency: tests/latency.cc
	$(CC) -std=c++17 $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o latency

run_saturn: scripts/run_saturn.cc
	$(CC) -std=c++11 $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o run_saturn

clean:
	rm -f *.o
	rm -f *.so
	rm -f test_svr latency run_saturn

