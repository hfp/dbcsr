CFLAGS += -Wall -Wextra -pedantic

DBG ?= 0
SYM ?= $(DBG)

INTEL ?= 0
IBM ?= 0

ifneq (0,$(INTEL))
ifeq (1,$(INTEL))
  CC := icc
else
  CC := icx
  LIB ?= 0
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

LIB ?= 1
ifneq (0,$(LIB))
dbcsr_acc_test: ../../tests/dbcsr_test_acc.o omp/dbcsr_omp.a
else
dbcsr_acc_test: ../../tests/dbcsr_test_acc.o omp/dbcsr_omp.o omp/dbcsr_omp_event.o omp/dbcsr_omp_mem.o omp/dbcsr_omp_stream.o
endif
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONE: clean
clean:
	@rm -f ../../tests/dbcsr_acc_test.o omp/dbcsr_omp*.o

.PHONE: realclean
realclean: clean
	@rm -f dbcsr_acc_test
