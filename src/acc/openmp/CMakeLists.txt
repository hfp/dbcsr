
set(DBCSR_OPENMP_SRCS
  acc_openmp.c
  acc_openmp_event.c
  acc_openmp_mem.c
  acc_openmp_stream.c
)

add_library(dbcsr_offload OBJECT ${DBCSR_OPENMP_SRCS})

if (OpenMP_FOUND)
  # with CMake 3.12+ the following can be replaced by:
  #   target_link_libraries(dbcsr_offload PRIVATE OpenMP::OpenMP_C)
  target_compile_options(dbcsr_offload PRIVATE
      $<TARGET_PROPERTY:OpenMP::OpenMP_C,INTERFACE_COMPILE_OPTIONS>
    )
endif ()
