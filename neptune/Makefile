CC = g++
CCFLAGS = -Wall -Wextra -flto -Wfatal-errors
MARSINCL = ../../mars/include
SATURNINCL = ../include
INCLUDES = -I$(MARSINCL) -I$(SATURNINCL) -I$(NISE_HOME)/src/common
LIBS = -lavrocpp -flto
LIBDIR = /home/xad/saturn

# -flto : link-time optimizations; needs to be passed to both compile and link commands.

# -fPIC

TARGETS = libsaturn.so
CURRENT_DIR = ${CURDIR}/..
all: $(TARGETS)
		sudo rm -f $(LIBDIR)/*
		sudo ln -s $(CURRENT_DIR)/include/saturn/* $(LIBDIR)/
		sudo ln -s $(CURRENT_DIR)/libsaturn.so $(LIBDIR)/libsaturn.so

libsaturn.so: ../src/feature_engine.cc ../src/svr_model.cc
	$(CC) $(CCFLAGS) -std=c++17 $(INCLUDES) -fPIC -shared $^ $(LIBS) -o $@

clean:
	rm -f *.o
	rm -f *.so
	rm -f $(TARGETS)

rpm:
	xpkg neptune-saturn.conf
