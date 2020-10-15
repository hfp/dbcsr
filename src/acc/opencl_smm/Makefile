CFLAGS = -std=c89 -Wall -Wextra -pedantic -Wno-variadic-macros -D__OPENCL
INCFILES = $(wildcard *.h*)
SRCFILES = $(wildcard *.c*)
OBJFILES = $(SRCFILES:.c=.o)

LIBXSMMROOT ?= $(wildcard ../../../../libxsmm)
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
    ifneq (1,$(DBG))
      CFLAGS += -D_DEBUG
    endif
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

.PHONE: all
all: acc_opencl_smm_test

acc_opencl_smm_test: acc_opencl_smm_test.o dbcsr_opencl_smm.a ../opencl/dbcsr_opencl.a $(LIBXSMMROOT)/lib/libxsmm.a
ifneq (Darwin,$(shell uname))
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL
else
	$(CC) $(LDFLAGS) -o $@ $^ -framework OpenCL
endif

dbcsr_opencl_smm.a: $(OBJFILES)
	$(AR) -rs $@ $^

%.o: %.c $(INCFILES) Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONE: clean
clean:
	@rm -f $(OBJFILES)

.PHONE: realclean
realclean: clean
	@rm -f dbcsr_opencl_smm.a acc_opencl_smm_test
