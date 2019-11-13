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

ifneq (,$(filter 0 1,$(INTEL)))
dbcsr_acc_test: dbcsr_acc_test.o openmp/libacc_openmp.a
else
dbcsr_acc_test: dbcsr_acc_test.o openmp/acc_openmp.o openmp/acc_openmp_event.o openmp/acc_openmp_mem.o openmp/acc_openmp_stream.o
endif
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONE: clean
clean:
	@rm -f dbcsr_acc_test.o openmp/acc_openmp*.o

.PHONE: realclean
realclean: clean
	@rm -f dbcsr_acc_test
