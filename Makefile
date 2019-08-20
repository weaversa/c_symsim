# Compiles the AIG simulator code to libsymsimabc.a
# Doesn't compile libabc

EXTRAS = Makefile LICENSE README.md

HEADERS = include/vectype.h include/pcode_definitions.h include/queue.h

SOURCES = src/constcheck.c src/memory.c src/pcode_definitions.c src/queue.c src/aig_vectors.c

OBJECTS = $(SOURCES:src/%.c=obj/%.o)

SYMSIMLIB = symsimabc

LIBABC_DIR = lib/abc

CC = gcc
DBG = #-g -Wall -fstack-protector-all -pedantic
OPT = -march=native -O3 -DNDEBUG -ffast-math -fomit-frame-pointer
INCLUDES = -I$(LIBABC_DIR)/src -Iinclude
LIBS =
LDFLAGS = -Llib
CFLAGS = -std=gnu99 $(DBG) $(OPT) -DABC_USE_STDINT_H $(INCLUDES)
AR = ar r
RANLIB = ranlib

all: depend lib/lib$(SYMSIMLIB).a

depend: .depend
.depend: $(SOURCES)
	@echo "Building dependencies" 
ifneq ($(wildcard ./.depend),)
	@rm -f "./.depend"
endif
	@$(CC) $(CFLAGS) -MM $^ > .depend
# Make .depend use the 'obj' directory
	@sed -i.bak -e :a -e '/\\$$/N; s/\\\n//; ta' .depend
	@sed -i.bak 's/^/obj\//' .depend
	@rm -f .depend.bak
-include .depend

$(OBJECTS): obj/%.o : src/%.c Makefile
	@echo "Compiling "$<""
	@[ -d obj ] || mkdir -p obj
	@$(CC) $(CFLAGS) -c $< -o $@

lib/lib$(SYMSIMLIB).a: $(OBJECTS) Makefile
	@echo "Creating "$@""
	@[ -d lib ] || mkdir -p lib
	@rm -f $@
	@$(AR) $@ $(OBJECTS)
	@$(RANLIB) $@

clean:
	rm -rf *~ */*~ $(OBJECTS) ./.depend *.dSYM

edit:
	emacs -nw $(EXTRAS) $(HEADERS) $(SOURCES)
