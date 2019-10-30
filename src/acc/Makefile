CFLAGS += -Wall -Wextra -pedantic

DBG ?= 0
SYM ?= $(DBG)

ifneq (0,$(SYM))
  CFLAGS += -g
endif
ifneq (0,$(DBG))
  CFLAGS += -O0
else
  CFLAGS += -O2
endif
ifneq (0,$(OMP))
  LDFLAGS += -fopenmp
  CFLAGS += -fopenmp
endif

dbcsr_acc_test: dbcsr_acc_test.o openmp/libacc_openmp.a
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

.PHONE: clean
clean:
	@rm -f dbcsr_acc_test.o

.PHONE: realclean
realclean: clean
	@rm -f dbcsr_acc_test
