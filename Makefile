CC = gcc
CFLAGS = -g -Werror -Wall
programs: sbpp

ifdef COVERAGE
$(info "building sbpp with coverage/profiling enabled")
CFLAGS += --coverage
endif


OBJDIR = build
SBPP_SRCS = $(basename $(wildcard *.c))
SBPP_OBJS = $(SBPP_SRCS:%=%.o)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) -o $@ $< -I ../include

$(OBJDIR)/preprocessor-parser.o : preprocessor-parser.c
	@mkdir -p $(@D)
	$(CC) -c $(CFLAGS) -o $@ $< -I ./include -Wno-discarded-qualifiers -Wno-incompatible-pointer-types-discards-qualifiers -Wno-unused-but-set-variable -Wno-unused-variable -Wno-unused-function

parser: preprocessor-parser.peg
	packcc -a preprocessor-parser.peg

sbpp: $(addprefix $(OBJDIR)/,$(SBPP_OBJS))
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJDIR)/*
	rm -f ./sbpp

info:
	$(info SBPP SRCS="$(SBPP_SRCS)")
	$(info SBPP OBJS="$(SBPP_OBJS)")
