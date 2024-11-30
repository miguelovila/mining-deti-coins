#
# makefile for the first practical assignment (A1)
#

#
# CUDA installation directory --- /usr/local/cuda or $(CUDA_HOME)
#
CUDA_DIR = /usr/local/cuda-12.6
#
# OpenCL installation directory (for a NVidia graphics card, sama as CUDA)
#
OPENCL_DIR = $(CUDA_DIR)

#
# CUDA device architecture
#   GeForce GTX 1660 Ti --- sm_75
#   RTX A2000 Ada --------- sm_86
#   RTX A6000 Ada --------- sm_86
#   RTX 4070 -------------- sm_89
#
CUDA_ARCH = sm_86

# Source and header files
SRC       = deti_coins.c
H_FILES   = includes/cpu_utilities.h
H_FILES  += includes/md5.h includes/md5_test_data.h includes/cpu/md5_cpu.h includes/avx/md5_cpu_avx.h includes/md5_cpu_neon.h
H_FILES  += includes/deti_coins_vault.h includes/cpu/deti_coins_cpu_search.h includes/cu/deti_coins_cuda_search.h 
C_FILES   = includes/cuda_driver_api_utilities.h includes/md5_cuda.h

#
# clean up
#
clean:
	rm -f *.o *.cubin deti_coins_intel_cuda deti_coins_intel deti_coins_apple


#
# compile for Intel/AMD processors without CUDA
#
deti_coins_intel:	$(SRC) $(H_FILES)
	cc -Wall -O2 -fopenmp -mavx2 -DUSE_CUDA=0 $(SRC) -o deti_coins_intel

#
# compilation for Apple silicon without CUDA
#
deti_coins_apple: $(SRC) $(H_FILES)
	cc -Wall -O2 -DUSE_CUDA=0 $(SRC) -o deti_coins_apple


#
# compile for Intel/AMD processors with CUDA
#
deti_coins_intel_cuda: deti_coins.o md5_cuda_kernel.cubin deti_coins_cuda_kernel_search.cubin
	cc -Wall -O2 -mavx2 -DUSE_CUDA=1 -I$(CUDA_DIR)/include deti_coins.o -o deti_coins_intel_cuda \
		-L$(CUDA_DIR)/lib64 -lcuda

deti_coins.o: $(SRC) $(H_FILES)
	cc -Wall -O2 -mavx2 -DUSE_CUDA=1 -I$(CUDA_DIR)/include -c $(SRC) -o deti_coins.o

md5_cuda_kernel.cubin: includes/md5.h md5_cuda_kernel.cu
	nvcc -arch=$(CUDA_ARCH) --compiler-options -O2,-Wall -I$(CUDA_DIR)/include --cubin md5_cuda_kernel.cu -o md5_cuda_kernel.cubin

deti_coins_cuda_kernel_search.cubin: includes/md5.h includes/cu/deti_coins_cuda_search.cu
	nvcc -arch=$(CUDA_ARCH) --compiler-options -O2,-Wall -I$(CUDA_DIR)/include --cubin includes/cu/deti_coins_cuda_search.cu -o deti_coins_cuda_kernel_search.cubin

