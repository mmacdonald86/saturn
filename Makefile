CC = g++
CCFLAGS = -Wall -Wextra -flto -Wfatal-errors
MARSINCL = ../mars/include
SATURNINCL = include
INCLUDES = -I$(MARSINCL) -I$(SATURNINCL)
LIBS = -lavrocpp -flto

# -flto : link-time optimizations; needs to be passed to both compile and link commands.

# -fPIC

TARGETS = libsaturn.so

all: $(TARGETS)

libsaturn.so: src/feature_engine.cc src/svr_model.cc
	$(CC) $(CCFLAGS) -std=c++17 $(INCLUDES) -fPIC -shared $^ $(LIBS) -o $@

clean:
	rm -f *.o
	rm -f *.so
	rm -f $(TARGETS)

