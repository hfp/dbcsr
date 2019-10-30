CFLAGS = -Wall -Wextra -pedantic
SRCFILES = $(wildcard *.c)
OBJFILES = $(SRCFILES:.c=.o)

DBG ?= 0
SYM ?= $(DBG)

ifneq (0,$(SYM))
  CFLAGS += -g
endif
ifneq (0,$(DBG))
  CFLAGS += -O0
else
  CFLAGS += -O2
endif
ifneq (0,$(OMP))
  LDFLAGS += -fopenmp
  CFLAGS += -fopenmp
endif

libacc_openmp.a: $(OBJFILES)
	$(AR) -rs $@ $^

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONE: clean
clean:
	@rm -f $(OBJFILES)

.PHONE: realclean
realclean: clean
	@rm -f libacc_openmp.a
