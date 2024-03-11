CC = gcc
CFLAGS = -g -Werror -Wall -fsanitize=address
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

sbpp: $(addprefix $(OBJDIR)/,$(SBPP_OBJS)) ../build/util.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJDIR)/*
	rm -f ./sbpp

info:
	$(info SBPP SRCS="$(SBPP_SRCS)")
	$(info SBPP OBJS="$(SBPP_OBJS)")
