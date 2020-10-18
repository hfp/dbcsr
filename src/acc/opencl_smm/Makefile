INCFILES := $(wildcard *.h*)
SRCFILES := $(wildcard *.c*)
OBJFILES := $(SRCFILES:.c=.o)

LIBXSMMROOT ?= $(wildcard ../../../../libxsmm)
UNAME := $(shell uname)
INTEL ?= 0
IBM ?= 0
DEV ?= 0

CFLAGS := \
  -std=c89 -Wall -Wextra -pedantic \
  -Wno-variadic-macros -Wno-long-long \
  -D__OPENCL -D__LIBXSMM -I$(LIBXSMMROOT)/include \
  $(NULL)

ifeq (Darwin,$(UNAME))
  OMP ?= 0
endif

ifneq (0,$(DEV))
  CFLAGS += -Wno-unused-parameter
endif

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
all: acc_opencl_trans_bench

acc_opencl_trans_bench: acc_opencl_trans_bench.o dbcsr_opencl_smm.a ../opencl/dbcsr_opencl.a
ifneq (Darwin,$(UNAME))
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL -L$(LIBXSMMROOT)/lib -lxsmm -lxsmmnoblas -ldl -lm
else
	$(CC) $(LDFLAGS) -o $@ $^ -framework OpenCL -L$(LIBXSMMROOT)/lib -lxsmm -lxsmmnoblas -ldl -lm
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
	@rm -f dbcsr_opencl_smm.a acc_opencl_trans_bench
