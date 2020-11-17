INCACC := $(wildcard *.h*) ../acc.h
SRCACC := $(wildcard *.cpp)
OBJACC := $(SRCACC:.cpp=.o)

INCSMM := $(wildcard ../libsmm_acc/*.h*) acc_libsmm.h
SRCSMM := $(wildcard ../libsmm_acc/*.cpp)
OBJSMM := $(SRCSMM:.cpp=.o)

INCALL := $(INCACC) $(INCSMM)

LIBXSMMROOT ?= $(wildcard ../../../../libxsmm)
UNAME := $(shell uname)
INTEL ?= 0
IBM ?= 0
DEV ?= 0

CXXFLAGS := \
  -Wall -Wextra -pedantic \
  -Wno-variadic-macros -Wno-long-long \
  -D__CUDA \
  $(NULL)

ifneq (0,$(DEV))
  CXXFLAGS := -std=c89 $(CXXFLAGS)
  CXXFLAGS += -Wno-unused-parameter
else
  #CXXFLAGS := -std=c++11 $(CXXFLAGS)
endif

ifneq (0,$(INTEL))
ifeq (1,$(INTEL))
  CXX := icpc
else
  CXX := icpx
endif
else ifneq (0,$(IBM))
  CXX := xlc
endif

ifneq (0,$(DBG))
  ifeq (,$(DBG))
    CXXFLAGS += -O2
  else
    ifneq (1,$(DBG))
      CXXFLAGS += -D_DEBUG
    endif
    CXXFLAGS += -O0
  endif
else
  CXXFLAGS += -O2 -DNDEBUG
  SYM := 0
endif
ifneq (0,$(SYM))
  CXXFLAGS += -g
endif

ifeq (Darwin,$(UNAME))
  OMP ?= 0
endif

ifneq (0,$(OMP))
ifneq (0,$(INTEL))
  LDFLAGS += -qopenmp
  CXXFLAGS += -qopenmp
else ifneq (0,$(IBM))
  LDFLAGS += -qsmp=omp
  CXXFLAGS += -qsmp=omp
else
  LDFLAGS += -fopenmp
  CXXFLAGS += -fopenmp
endif
endif

ifneq (,$(LIBXSMMROOT))
  LDFLAGS := -pthread $(LDFLAGS) -L$(LIBXSMMROOT)/lib -lxsmmext -lxsmm -lxsmmnoblas -ldl -lm
  CXXFLAGS += -pthread -D__LIBXSMM -I$(LIBXSMMROOT)/include
endif

ifeq (Darwin,$(UNAME))
  LDFLAGS += -framework OpenCL
else
  LDFLAGS += -lOpenCL
endif

.PHONY: all
all: ../dbcsr_acc.a ../dbcsr_acc_smm.a bench test

.PHONY: bench
test: ../acc_bench_trans

.PHONY: test
test: ../dbcsr_acc_test acc_opencl_test

../dbcsr_acc.a: $(OBJACC)
	$(AR) -rs $@ $^

../dbcsr_acc_smm.a: $(OBJSMM)
	$(AR) -rs $@ $^

%.o: %.c $(INCALL) Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

acc_opencl_bench_trans.o: ../acc_bench_trans.c Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

dbcsr_acc_test.o: ../../../tests/dbcsr_acc_test.c Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

../acc_bench_trans: acc_opencl_bench_trans.o ../dbcsr_acc_smm.a ../dbcsr_acc.a
	$(CXX) $^ $(LDFLAGS) -o $@

../dbcsr_acc_test: dbcsr_acc_test.o ../dbcsr_acc.a
	$(CXX) $^ $(LDFLAGS) -o $@

acc_opencl_test: acc_opencl_test.o ../dbcsr_acc.a
	$(CXX) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	@rm -f $(OBJACC) acc_opencl_bench_trans.o dbcsr_acc_test.o

.PHONY: realclean
realclean: clean
	@rm -f ../dbcsr_acc.a ../dbcsr_acc_smm.a ../dbcsr_acc_test ../acc_bench_trans acc_opencl_test
