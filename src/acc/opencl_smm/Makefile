INCFILES := $(wildcard *.h*)
SRCFILES := $(wildcard *.c*)
OBJFILES := $(SRCFILES:.c=.o)

LIBXSMMROOT ?= $(wildcard ../../../../libxsmm)
UNAME := $(shell uname)
INTEL ?= 0
IBM ?= 0
DEV ?= 0
OMP ?= 0

CFLAGS := \
  -Wall -Wextra -pedantic \
  -Wno-variadic-macros -Wno-long-long \
  -D__OPENCL  \
  $(NULL)

ifneq (0,$(DEV))
  CFLAGS := -std=c89 $(CFLAGS)
  CFLAGS += -Wno-unused-parameter
else
  CFLAGS := -std=c99 $(CFLAGS)
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

ifneq (0,$(INTEL))
  LDFLAGS := -qopenmp $(LDFLAGS)
else ifneq (0,$(IBM))
  LDFLAGS := -qsmp=omp $(LDFLAGS)
else ifneq (Darwin,$(UNAME))
  LDFLAGS := -fopenmp $(LDFLAGS)
else
  LDFLAGS += -lomp
endif

ifneq (Darwin,$(UNAME))
  LDFLAGS += -lOpenCL
else
  LDFLAGS += -framework OpenCL
endif

ifneq (,$(LIBXSMMROOT))
  LDFLAGS := -pthread $(LDFLAGS) -L$(LIBXSMMROOT)/lib -lxsmmext -lxsmm -lxsmmnoblas -ldl -lm
  CFLAGS += -pthread -D__LIBXSMM -I$(LIBXSMMROOT)/include
endif

.PHONE: all
all: acc_opencl_bench_trans

acc_opencl_bench_trans: acc_opencl_bench_trans.o dbcsr_opencl_smm.a ../opencl/dbcsr_opencl.a
ifneq (Darwin,$(UNAME))
	$(CC) -o $@ $^ $(LDFLAGS)
else
	$(CC) -o $@ $^ $(LDFLAGS)
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
	@rm -f dbcsr_opencl_smm.a acc_opencl_bench_trans
