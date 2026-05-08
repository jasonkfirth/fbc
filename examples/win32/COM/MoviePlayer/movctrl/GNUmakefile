##########################

LIBNAME := movctrl
LIBDEST := ../

SRCS := movctrl.bas CMovie.bas
OBJS := $(patsubst %.bas,obj/%.o,$(SRCS))

##########################

FBC ?= fbc
FBCFLAGS :=
MKDIR_P ?= mkdir -p
RM ?= rm -f
.DEFAULT_GOAL := all

ifdef DEBUG
FBCFLAGS += -g
endif

LIBRARY := $(LIBDEST)lib$(LIBNAME).a

.SUFFIXES:
.SUFFIXES: .bas

VPATH = .

obj/%.o : %.bas | obj
	$(FBC) $(FBCFLAGS) -c $< -o $@

obj:
	$(MKDIR_P) obj

##########################

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	$(FBC) $(FBCFLAGS) -lib -x $@ $(OBJS)

.PHONY : clean
clean:
	-$(RM) $(OBJS) $(LIBRARY)
