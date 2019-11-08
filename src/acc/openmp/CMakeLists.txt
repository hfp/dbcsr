
set(DBCSR_OPENMP_SRCS
  acc_openmp.c
  acc_openmp_event.c
  acc_openmp_mem.c
  acc_openmp_stream.c
)

add_library(dbcsr_offload OBJECT ${DBCSR_OPENMP_SRCS})
