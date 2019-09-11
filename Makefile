CC = g++
CCFLAGS = -Wall -Wextra -Wfatal-errors -flto
HEADER = include/saturn/saturn.h
LIBS = -lavrocpp -flto

# -flto : link-time optimizations; needs to be passed to both compile and link commands.

TARGETS = libsaturn.so latency run_ctr run_saturn test_svr run_winrate

all: $(TARGETS)

libsaturn.so: src/feature_engine.cc src/ctr_model.cc src/svr_model.cc src/utils.cc src/wr_model.cc
	$(CC) -std=c++17 $(CCFLAGS) -Iinclude -fPIC -shared $^ $(LIBS) -o $@

latency: tests/latency.cc
	$(CC) -std=c++17 $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o latency

run_ctr: scripts/run_ctr.cc
	$(CC) -std=c++11 $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o run_ctr

test_svr: tests/test_svr.cc
	$(CC) -std=c++11 $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o test_svr

run_saturn: scripts/run_saturn.cc
	$(CC) -std=c++11 $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o run_saturn

run_winrate: scripts/run_winrate.cc
	$(CC) -std=c++11 $(CCFLAGS) -Iinclude $^ ./libsaturn.so $(LIBS) -o run_winrate

clean:
	rm -f *.o
	rm -f *.so
	rm -f test_svr latency run_ctr run_saturn run_winrate

