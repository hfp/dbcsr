INCFILES := $(wildcard *.h*)
SRCFILES := $(wildcard *.c*)
OBJFILES := $(SRCFILES:.c=.o)

UNAME := $(shell uname)
INTEL ?= 0
IBM ?= 0

CFLAGS := \
  -std=c89 -Wall -Wextra -pedantic \
  -Wno-variadic-macros -Wno-long-long \
  -D__OPENCL \
  $(NULL)

ifeq (Darwin,$(UNAME))
  OMP ?= 0
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
all: dbcsr_test_acc acc_opencl_test

# /usr/lib/x86_64-linux-gnu/libOpenCL.so.1
dbcsr_test_acc: dbcsr_test_acc.o dbcsr_opencl.a
ifneq (Darwin,$(UNAME))
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL
else
	$(CC) $(LDFLAGS) -o $@ $^ -framework OpenCL
endif

acc_opencl_test: acc_opencl_test.o dbcsr_opencl.a
ifneq (Darwin,$(UNAME))
	$(CC) $(LDFLAGS) -o $@ $^ -lOpenCL
else
	$(CC) $(LDFLAGS) -o $@ $^ -framework OpenCL
endif

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
	@rm -f dbcsr_opencl.a dbcsr_test_acc acc_opencl_test
