################################################################################
# Makefile for project 'compile'                                               #
################################################################################
.PHONY: debug clean install uninstall

# compiler
COMPILE = gcc -c
LINK = gcc
OPTIONS = -ansi -D BUILD_COMPILE_POSIX -D _GNU_SOURCE
OUT = -o

# output
ifeq ($(MAKECMDGOALS),debug)
OPTIONS := $(OPTIONS) -g
PROGRAM = compile-debug
OBJECTDIR = dobj/
else
PROGRAM = compile
OBJECTDIR = obj/
endif
OBJECTS = compile.o compiler.o settings.o stringbuf.o
OBJECTS := $(addprefix $(OBJECTDIR),$(OBJECTS))

# dependencies
STRINGBUF_H = stringbuf.h
SETTINGS_H = settings.h
COMPILER_H = compiler.h $(STRINGBUF_H) $(SETTINGS_H)


all: $(OBJECTDIR) $(PROGRAM)
debug: $(OBJECTDIR) $(PROGRAM)

$(PROGRAM): $(OBJECTS)
	$(LINK) $(OBJECTS) $(OUT)$(PROGRAM)

$(OBJECTDIR)compile.o: compile.c
	$(COMPILE) $(OPTIONS) compile.c $(OUT)$(OBJECTDIR)compile.o

$(OBJECTDIR)compiler.o: compiler.c compiler_posix.c $(COMPILER_H)
	$(COMPILE) $(OPTIONS) compiler.c $(OUT)$(OBJECTDIR)compiler.o

$(OBJECTDIR)settings.o: settings.c settings_posix.c $(SETTINGS_H)
	$(COMPILE) $(OPTIONS) settings.c $(OUT)$(OBJECTDIR)settings.o

$(OBJECTDIR)stringbuf.o: stringbuf.c $(STRINGBUF_H)
	$(COMPILE) $(OPTIONS) stringbuf.c $(OUT)$(OBJECTDIR)stringbuf.o

$(OBJECTDIR):
	mkdir $(OBJECTDIR)

clean:
	rm -f compile
	rm -f compile-debug
	rm -f obj/*.o
	if [ -d obj ]; then rmdir obj; fi
	rm -f dobj/*.o
	if [ -d dobj ]; then rmdir dobj; fi

install:
	cp $(PROGRAM) /usr/local/bin

uninstall:
	rm /usr/local/bin/$(PROGRAM)
