CC=g++
CXX=g++
RANLIB=ranlib

LIBSRC=uthreads.cpp Scheduler.cpp Thread.cpp UThreadError.cpp
LIBOBJ=$(LIBSRC:.cpp=.o)
HEADERS=Scheduler.h Thread.h UThreadError.h

INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

LIBFILE = libuthreads.a
TARGETS = $(LIBFILE)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(LIBSRC) $(HEADERS) Makefile README

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(OSMLIB) $(OBJ) $(LIBOBJ) *~ *core

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
