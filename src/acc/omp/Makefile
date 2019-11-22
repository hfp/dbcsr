CFLAGS = -Wall -Wextra -pedantic
INCFILES = $(wildcard *.h*)
SRCFILES = $(wildcard *.c*)
OBJFILES = $(SRCFILES:.c=.o)

DBG ?= 0
SYM ?= $(DBG)

INTEL ?= 0
IBM ?= 0

ifneq (0,$(INTEL))
ifeq (1,$(INTEL))
  CC := icc
else
  CC := icx
endif
else ifneq (0,$(IBM))
  CC := xlc
endif

ifneq (0,$(SYM))
  CFLAGS += -g
endif
ifneq (0,$(DBG))
  CFLAGS += -O0
else
  CFLAGS += -O2
endif
ifneq (0,$(OMP))
ifneq (0,$(INTEL))
ifeq (1,$(INTEL))
  LDFLAGS += -qopenmp -qoffload
  CFLAGS += -qopenmp -qoffload
else
  LDFLAGS += -fiopenmp -fopenmp-targets=spir64
  CFLAGS += -fiopenmp -fopenmp-targets=spir64
endif
else ifneq (0,$(IBM))
  LDFLAGS += -qsmp=omp -qoffload
  CFLAGS += -qsmp=omp -qoffload
else
  LDFLAGS += -fopenmp
  CFLAGS += -fopenmp
endif
endif

dbcsr_omp.a: $(OBJFILES)
	$(AR) -rs $@ $^

%.o: %.c $(INCFILES) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONE: clean
clean:
	@rm -f $(OBJFILES)

.PHONE: realclean
realclean: clean
	@rm -f dbcsr_omp.a
