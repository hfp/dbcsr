set(SMM_ACC_KERNELS kernels/multiply.cl kernels/transpose.cl
                    kernels/transpose_inplace.cl)

set(LIBSMM_ACC_SRC_FILES opencl_libsmm.c)
set(LIBSMM_ACC_HEADER_FILES opencl_libsmm.h)
set(LIBSMM_ACC_FILES ${LIBSMM_ACC_SRC_FILES} ${LIBSMM_ACC_HEADER_FILES})

if (USE_OPENCL)
  add_library(libsmm_acc OBJECT ${LIBSMM_ACC_FILES})
  target_compile_definitions(libsmm_acc PRIVATE __OPENCL)
  target_compile_definitions(libsmm_acc PRIVATE __LIBXSMM)
  # account for DBCSR not calling libsmm_acc_init() (DBCSR only calls acc_init)
  target_compile_definitions(acc PRIVATE __DBCSR_ACC)
  target_include_directories(libsmm_acc PRIVATE ${LIBXSMM_INCLUDE_DIRS})
endif ()

if (OpenMP_FOUND)
  target_link_libraries(libsmm_acc PRIVATE OpenMP::OpenMP_C)
endif ()
