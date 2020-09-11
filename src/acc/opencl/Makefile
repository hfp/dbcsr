CFLAGS = -Wall -Wextra -pedantic -D__OPENCL
INCFILES = $(wildcard *.h*)
SRCFILES = $(wildcard *.c*)
OBJFILES = $(SRCFILES:.c=.o)

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
  ifeq (,$(DBG))
    CFLAGS += -O2
  else
    CFLAGS += -O0
  endif
else
  CFLAGS += -O2 -DNDEBUG
endif
ifneq (0,$(OMP))
ifneq (0,$(INTEL))
  LDFLAGS += -qopenmp
  CFLAGS += -qopenmp
else ifneq (0,$(IBM))
  LDFLAGS += -qsmp=omp
  CFLAGS += -qsmp=omp
else
  LDFLAGS += -fopenmp
  CFLAGS += -fopenmp
endif
endif

# /usr/lib/x86_64-linux-gnu/libOpenCL.so.1
dbcsr_test_acc: dbcsr_test_acc.o dbcsr_opencl.a
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL

dbcsr_opencl.a: $(OBJFILES)
	$(AR) -rs $@ $^

%.o: %.c $(INCFILES) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

dbcsr_test_acc.o: ../../../tests/dbcsr_test_acc.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONE: clean
clean:
	@rm -f $(OBJFILES) dbcsr_test_acc.o

.PHONE: realclean
realclean: clean
	@rm -f dbcsr_opencl.a dbcsr_test_acc
