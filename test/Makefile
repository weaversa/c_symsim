# For a C program 'foo.c' that links against ABC and the c_syssim tools
# built on top of ABC, run the following in a shell.
#     $ make foo
#
# This will create 'foo' in the current directory by (1) compiling
# foo.c into foo.o and (2) by linking foo.o with libsymsimabc.a and
# libabc.a

LIBSYMSIM_DIR = ../
LIBABC_DIR = $(LIBSYMSIM_DIR)/lib/abc

CC = gcc
DBG = -g -Wall -fstack-protector-all -pedantic
OPT = #-march=native -O3 -DNDEBUG -ffast-math -fomit-frame-pointer

INCLUDES = -I$(LIBABC_DIR)/src/ -I$(LIBSYMSIM_DIR)/include -I.
CFLAGS = -DABC_USE_STDINT_H $(DBG) $(OPT) $(INCLUDES)
LDFLAGS = -L$(LIBSYMSIM_DIR)/lib -L$(LIBABC_DIR) -L.
LIBS = -rdynamic -ltermcap -lsymsimabc -labc -lpthread -lstdc++ -lm -lreadline -ldl

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

.PHONY: clean
clean:
	rm -rf $(OBJS) $(LIBSYMSIM) *.dSYM *~ *.aig
