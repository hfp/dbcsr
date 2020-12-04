set(SMM_ACC_KERNELS
    kernels/multiply.cl
    kernels/transpose.cl
    kernels/transpose_inplace.cl)

set(LIBSMM_ACC_SRC_FILES acc_opencl_smm.c)
set(LIBSMM_ACC_HEADER_FILES acc_opencl_smm.h)
set(LIBSMM_ACC_FILES ${LIBSMM_ACC_SRC_FILES} ${LIBSMM_ACC_HEADER_FILES})

if (USE_OPENCL)
  add_library(libsmm_acc OBJECT ${LIBSMM_ACC_FILES})
  target_compile_definitions(libsmm_acc PRIVATE __OPENCL)
  target_compile_definitions(libsmm_acc PRIVATE __LIBXSMM)
endif ()

if (OpenMP_FOUND)
  target_link_libraries(libsmm_acc PRIVATE OpenMP::OpenMP_C)
endif ()
