
set(DBCSR_LIBSMM_SRCS
  libsmm.c
  libsmm_transpose.c
  libsmm_process.c
)

add_library(dbcsr_libsmm OBJECT ${DBCSR_LIBSMM_SRCS})

if (OpenMP_FOUND)
  # with CMake 3.12+ the following can be replaced by:
  #   target_link_libraries(dbcsr_libsmm PRIVATE OpenMP::OpenMP_C)
  target_compile_options(dbcsr_libsmm PRIVATE
      $<TARGET_PROPERTY:OpenMP::OpenMP_C,INTERFACE_COMPILE_OPTIONS>
    )
endif ()
