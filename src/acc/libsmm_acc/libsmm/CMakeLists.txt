
set(DBCSR_LIBSMM_SRCS
  libsmm.c
  libsmm_transpose.c
  libsmm_process.c
)

add_library(dbcsr_libsmm OBJECT ${DBCSR_LIBSMM_SRCS})
