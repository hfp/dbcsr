INCACC := $(wildcard *.h*) ../acc.h
SRCACC := $(wildcard *.cpp)
OBJACC := $(SRCACC:.cpp=.o) acc_cublas.o

GPUSMM := $(wildcard ../libsmm_acc/kernels/*.h*)
INCSMM := $(wildcard ../libsmm_acc/*.h*) ../acc_libsmm.h \
  ../libsmm_acc/smm_acc_kernels.h \
  ../libsmm_acc/parameters.h \
  $(NULL)
SRCSMM := $(wildcard ../libsmm_acc/*.cpp)
OBJSMM := $(SRCSMM:.cpp=.o)

INCALL := $(INCACC) $(INCSMM)

LIBXSMMROOT ?= $(wildcard ../../../../libxsmm)
NVCC ?= $(shell which nvcc 2>/dev/null)
CUDA_PATH ?= $(if $(NVCC),$(abspath $(dir $($(NVCC)))/..))
PYTHON := python3
UNAME := $(shell uname)
WITH_GPU ?= V100
INTEL ?= 0
DEV ?= 0

CFLAGS := -fPIC \
  -Wall -Wextra -pedantic \
  -D__CUDA -D__ACC -D__DBCSR_ACC \
  -DARCH_NUMBER=GPU_ARCH_NUMBER_$(WITH_GPU) \
  -DNO_DBCSR_TIMESET \
  $(NULL)

ifeq (1,$(INTEL))
  CXX := icpc
  CC := icc
  AR := xiar
else ifneq (0,$(INTEL))
  CXX := icpx
  CC := icx
  AR := xiar
else
  CXX := g++
  CC := gcc
  AR := gcc-ar
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
  SYM := 0
endif
ifneq (0,$(SYM))
  CFLAGS += -g
endif

ifeq (Darwin,$(UNAME))
  OMP ?= 0
endif

ifneq (0,$(OMP))
ifneq (0,$(INTEL))
  LDFLAGS += -qopenmp
  CFLAGS += -qopenmp
else
  LDFLAGS += -fopenmp
  CFLAGS += -fopenmp
endif
endif

ifneq (,$(LIBXSMMROOT))
  LDFLAGS := -pthread $(LDFLAGS) -L$(LIBXSMMROOT)/lib -lxsmmext -lxsmm -lxsmmnoblas -ldl -lm
  CFLAGS += -pthread -D__LIBXSMM -I$(LIBXSMMROOT)/include
endif

ifneq (0,$(DEV))
  CXXFLAGS := -std=c++11 $(CFLAGS)
  CFLAGS := -std=c89 $(CFLAGS)
else
  CXXFLAGS := $(CFLAGS)
endif

LDFLAGS += -L$(CUDA_PATH)/lib64 -lcudart -lcublas -lnvrtc -lcuda
CFLAGS += -Wno-variadic-macros -Wno-long-long

.PHONY: all
all: ../dbcsr_acc.a ../dbcsr_acc_smm.a bench test

.PHONY: bench
test: ../acc_bench_trans

.PHONY: test
test: ../dbcsr_acc_test

../libsmm_acc/parameters.h: Makefile ../libsmm_acc/generate_parameters.py ../libsmm_acc/parameters/parameters_$(WITH_GPU).json
	@cd ../libsmm_acc && $(PYTHON) ../libsmm_acc/generate_parameters.py --gpu_version=$(WITH_GPU) --base_dir=../libsmm_acc/parameters

../libsmm_acc/smm_acc_kernels.h: $(GPUSMM) Makefile ../libsmm_acc/generate_kernels.py ../libsmm_acc/parameters/parameters_$(WITH_GPU).json
	@cd ../libsmm_acc && $(PYTHON) ../libsmm_acc/generate_kernels.py ../libsmm_acc/kernels

acc_cublas.o: acc_cublas.cu Makefile
	$(NVCC) $(addprefix -Xcompiler $(NULL),$(CXXFLAGS)) -c $< -o $@

../dbcsr_acc.a: $(OBJACC) acc_cublas.o ../libsmm_acc/libsmm_acc_init.o
	$(AR) -rs $@ $^

../dbcsr_acc_smm.a: $(OBJSMM)
	$(AR) -rs $@ $^

%.o: %.cpp $(INCALL) Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

acc_bench_trans.o: ../acc_bench_trans.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

dbcsr_acc_test.o: ../../../tests/dbcsr_acc_test.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

../acc_bench_trans: acc_bench_trans.o ../dbcsr_acc_smm.a ../dbcsr_acc.a
	$(CXX) $^ $(LDFLAGS) -o $@

../dbcsr_acc_test: dbcsr_acc_test.o ../dbcsr_acc_smm.a ../dbcsr_acc.a
	$(CXX) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	@rm -f $(OBJACC) acc_bench_trans.o dbcsr_acc_test.o

.PHONY: realclean
realclean: clean
	@rm -f ../dbcsr_acc.a ../dbcsr_acc_smm.a ../dbcsr_acc_test ../acc_bench_trans
