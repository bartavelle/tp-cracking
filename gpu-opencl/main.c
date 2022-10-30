#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../includes/config.h"

#define MAX_SOURCE_SIZE (0x100000)

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s opencl_kernel.cl HASH\n", argv[0]);
    exit(1);
  }

  unsigned char *target = parse_hash(argv[2]);

  // Load the kernel source code into the array source_str
  FILE *fp;
  char *source_str;
  size_t source_size;

  fp = fopen(argv[1], "r");
  if (!fp) {
    fprintf(stderr, "Failed to load kernel.\n");
    exit(1);
  }
  source_str = (char *)malloc(MAX_SOURCE_SIZE);
  source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
  fclose(fp);

  // Get platform and device information
  cl_platform_id platform_id = NULL;
  cl_device_id device_id = NULL;
  cl_uint ret_num_devices;
  cl_uint ret_num_platforms;
  cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
  if (ret) goto free_source;
  ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id,
                       &ret_num_devices);
  if (ret) goto free_source;

  size_t max_compute_units = 0;
  ret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS,
                        sizeof(max_compute_units), &max_compute_units, NULL);
  if (ret) goto free_source;
  size_t max_work_group_size = 0;
  ret =
      clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE,
                      sizeof(max_work_group_size), &max_work_group_size, NULL);
  if (ret) goto free_source;
  cl_uint max_dimensions = 0;
  ret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                        sizeof(max_dimensions), &max_dimensions, NULL);
  if (ret) goto free_source;
  size_t *max_work_item_sizes = malloc(sizeof(size_t) * max_dimensions);
  ret = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                        sizeof(size_t) * max_dimensions, max_work_item_sizes,
                        NULL);
  if (ret) goto free_source;

  printf("Device CU=%ld GROUP_SIZE=%ld ITEM_SIZES=", max_compute_units,
         max_work_group_size);
  for (int i = 0; i < max_dimensions; i++) {
    if (i > 0) printf(",");
    printf("%ld", max_work_item_sizes[i]);
  }
  printf("\n");

  // Create an OpenCL context
  cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
  if (ret) goto free_source;

  // Create a command queue
  cl_command_queue command_queue =
      clCreateCommandQueueWithProperties(context, device_id, NULL, &ret);
  if (ret) goto free_context;

  // Create memory buffers on the device for each vector
  cl_mem target_mem_obj =
      clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(target), NULL, &ret);
  if (ret) goto free_queue;
  cl_mem solution_mem_obj =
      clCreateBuffer(context, CL_MEM_READ_ONLY, PWD_LEN, NULL, &ret);
  if (ret) goto free_target_buf;

  // Copy the lists A and B to their respective memory buffers
  ret = clEnqueueWriteBuffer(command_queue, target_mem_obj, CL_TRUE, 0,
                             sizeof(target), target, 0, NULL, NULL);
  if (ret) goto free_sol_buf;

  // Create a program from the kernel source
  cl_program program =
      clCreateProgramWithSource(context, 1, (const char **)&source_str,
                                (const size_t *)&source_size, &ret);
  if (ret) {
    printf("could not create program\n");
    goto free_sol_buf;
  }

  // Build the program
  ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
  if (ret) {
    printf("could not build program\n");
    switch (ret) {
      case CL_INVALID_PROGRAM:
        printf("program is not a valid program object.\n");
        break;
      case CL_INVALID_VALUE:
        printf(
            "device_list is NULL and num_devices is greater than zero, or if "
            "device_list is not NULL and num_devices is zero.\n");
        printf("pfn_notify is NULL but user_data is not NULL.\n");
        break;
      case CL_INVALID_DEVICE:
        printf(
            "OpenCL devices listed in device_list are not in the list of "
            "devices associated with program.\n");
        break;
      case CL_INVALID_BINARY:
        printf(
            "program is created with clCreateProgramWithBinary and devices "
            "listed in device_list do not have a valid program binary "
            "loaded.\n");
        break;
      case CL_INVALID_BUILD_OPTIONS:
        printf("the build options specified by options are invalid.\n");
        break;
      case CL_COMPILER_NOT_AVAILABLE:
        printf(
            "program is created with clCreateProgramWithSource and a compiler "
            "is not available i.e. CL_DEVICE_COMPILER_AVAILABLE specified in "
            "the table of OpenCL Device Queries for clGetDeviceInfo is set to "
            "CL_FALSE.\n");
        break;
      case CL_BUILD_PROGRAM_FAILURE:
        printf(
            "there is a failure to build the program executable. This error "
            "will be returned if clBuildProgram does not return until the "
            "build has completed.\n");
        break;
      case CL_INVALID_OPERATION:
        printf(
            "the build of a program executable for any of the devices listed "
            "in device_list by a previous call to clBuildProgram for program "
            "has not completed.\n");
        printf("there are kernel objects attached to program.\n");
        printf(
            "program was not created with clCreateProgramWithSource, "
            "clCreateProgramWithIL, or clCreateProgramWithBinary.\n");
        printf(
            "the program requires independent forward progress of sub-groups "
            "but one or more of the devices listed in device_list does not "
            "return CL_TRUE for the "
            "CL_DEVICE_SUBGROUP_INDEPENDENT_FORWARD_PROGRESS query.\n");
        break;
      case CL_OUT_OF_RESOURCES:
        printf(
            "there is a failure to allocate resources required by the OpenCL "
            "implementation on the device.\n");
        break;
      case CL_OUT_OF_HOST_MEMORY:
        printf(
            "there is a failure to allocate resources required by the OpenCL "
            "implementation on the host.\n");
        break;
    }
    goto free_program;
  }

  // Create the OpenCL kernel
  cl_kernel kernel = clCreateKernel(program, "md4_crack", &ret);
  if (ret) {
    printf("could not create kernel\n");
    switch (ret) {
      case CL_INVALID_PROGRAM:
        printf("program is not a valid program object.\n");
        break;

      case CL_INVALID_PROGRAM_EXECUTABLE:
        printf("there is no successfully built executable for program.\n");
        break;

      case CL_INVALID_KERNEL_NAME:
        printf("kernel_name is not found in program.\n");
        break;

      case CL_INVALID_KERNEL_DEFINITION:
        printf(
            "the function definition for __kernel function given by "
            "kernel_name such as the number of arguments, the argument types "
            "are not the same for all devices for which the program executable "
            "has been built.\n");
        break;

      case CL_INVALID_VALUE:
        printf("kernel_name is NULL.\n");
        break;

      case CL_OUT_OF_RESOURCES:
        printf(
            "there is a failure to allocate resources required by the OpenCL "
            "implementation on the device.\n");
        break;

      case CL_OUT_OF_HOST_MEMORY:
        printf(
            "there is a failure to allocate resources required by the OpenCL "
            "implementation on the host.\n");
        break;
    }
    goto free_program;
  }

  // Set the arguments of the kernel
  ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&target_mem_obj);
  if (ret) {
    printf("Can't set kernel arg 0\n");
    goto free_kernel;
  }
  ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&solution_mem_obj);
  if (ret) {
    printf("Can't set kernel arg 1\n");
    goto free_kernel;
  }

  // Execute the OpenCL kernel on the list
  size_t global_item_size = 26 * 26 * 32;
  size_t local_item_size = 32;
  cl_event wait;
  ret =
      clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_item_size,
                             &local_item_size, 0, NULL, &wait);
  if (ret) {
    printf("clEqueueNDRangeKernel failed\n");
    switch (ret) {
      case CL_INVALID_PROGRAM_EXECUTABLE:
        printf(
            "there is no successfully built program executable available for "
            "device associated with command_queue.\n");
        break;
      case CL_INVALID_COMMAND_QUEUE:
        printf("command_queue is not a valid host command-queue.\n");
        break;
      case CL_INVALID_KERNEL:
        printf("kernel is not a valid kernel object.\n");
        break;
      case CL_INVALID_CONTEXT:
        printf(
            "context associated with command_queue and kernel are not the same "
            "or if the context associated with command_queue and events in "
            "event_wait_list are not the same.\n");
        break;
      case CL_INVALID_KERNEL_ARGS:
        printf(
            "the kernel argument values have not been specified or if a kernel "
            "argument declared to be a pointer to a type does not point to a "
            "named address space.\n");
        break;
      case CL_INVALID_WORK_DIMENSION:
        printf(
            "work_dim is not a valid value (i.e. a value between 1 and 3).\n");
        break;
      case CL_INVALID_GLOBAL_WORK_SIZE:
        printf(
            "any of the values specified in global_work_size[0], "
            "…​global_work_size [work_dim - 1] exceed the range given by "
            "the "
            "sizeof(size_t) for the device on which the kernel execution will "
            "be enqueued.\n");
        break;
      case CL_INVALID_GLOBAL_OFFSET:
        printf(
            "the value specified in global_work_size + the corresponding "
            "values in global_work_offset for any dimensions is greater than "
            "the sizeof(size_t) for the device on which the kernel execution "
            "will be enqueued.\n");
        break;
      case CL_INVALID_WORK_GROUP_SIZE:
        printf(
            "local_work_size is specified and does not match the required "
            "work-group size for kernel in the program source.\n");
        printf(
            "local_work_size is specified and is not consistent with the "
            "required number of sub-groups for kernel in the program "
            "source.\n");
        printf(
            "local_work_size is specified and the total number of work-items "
            "in the work-group computed as local_work_size[0] * …​ "
            "local_work_size[work_dim – 1] is greater than the value specified "
            "by CL_KERNEL_WORK_GROUP_SIZE in table 5.21.\n");
        printf(
            "the program was compiled with –cl-uniform-work-group-size and the "
            "number of work-items specified by global_work_size is not evenly "
            "divisible by size of work-group given by local_work_size or by "
            "the required work- group size specified in the kernel source.\n");
        break;
      case CL_INVALID_WORK_ITEM_SIZE:
        printf(
            "the number of work-items specified in any of local_work_size[0], "
            "…​ local_work_size[work_dim - 1] is greater than the "
            "corresponding values specified by "
            "CL_DEVICE_MAX_WORK_ITEM_SIZES[0], …​. "
            "CL_DEVICE_MAX_WORK_ITEM_SIZES[work_dim - 1].\n");
        break;
      case CL_MISALIGNED_SUB_BUFFER_OFFSET:
        printf(
            "a sub-buffer object is specified as the value for an argument "
            "that is a buffer object and the offset specified when the "
            "sub-buffer object is created is not aligned to "
            "CL_DEVICE_MEM_BASE_ADDR_ALIGN value for device associated with "
            "queue.\n");
        break;
      case CL_INVALID_IMAGE_SIZE:
        printf(
            "an image object is specified as an argument value and the image "
            "dimensions (image width, height, specified or compute row and/or "
            "slice pitch) are not supported by device associated with "
            "queue.\n");
        break;
      case CL_IMAGE_FORMAT_NOT_SUPPORTED:
        printf(
            "an image object is specified as an argument value and the image "
            "format (image channel order and data type) is not supported by "
            "device associated with queue.\n");
        break;
      case CL_OUT_OF_RESOURCES:
        printf(
            "there is a failure to queue the execution instance of kernel on "
            "the command-queue because of insufficient resources needed to "
            "execute the kernel. For example, the explicitly specified "
            "local_work_size causes a failure to execute the kernel because of "
            "insufficient resources such as registers or local memory. Another "
            "example would be the number of read-only image args used in "
            "kernel exceed the CL_DEVICE_MAX_READ_IMAGE_ARGS value for device "
            "or the number of write-only image args used in kernel exceed the "
            "CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS value for device or the "
            "number of samplers used in kernel exceed CL_DEVICE_MAX_SAMPLERS "
            "for device.\n");
        printf(
            "there is a failure to allocate resources required by the OpenCL "
            "implementation on the device.\n");
        break;
      case CL_MEM_OBJECT_ALLOCATION_FAILURE:
        printf(
            "there is a failure to allocate memory for data store associated "
            "with image or buffer objects specified as arguments to kernel.\n");
        break;
      case CL_INVALID_EVENT_WAIT_LIST:
        printf(
            "event_wait_list is NULL and num_events_in_wait_list > 0, or "
            "event_wait_list is not NULL and num_events_in_wait_list is 0, or "
            "if event objects in event_wait_list are not valid events.\n");
        break;
      case CL_INVALID_OPERATION:
        printf(
            "SVM pointers are passed as arguments to a kernel and the device "
            "does not support SVM or if system pointers are passed as "
            "arguments to a kernel and/or stored inside SVM allocations passed "
            "as kernel arguments and the device does not support fine grain "
            "system SVM allocations.\n");
        break;
      case CL_OUT_OF_HOST_MEMORY:
        printf(
            "there is a failure to allocate resources required by the OpenCL "
            "implementation on the host.\n");
        break;
    }
    goto free_kernel;
  }

  clFlush(command_queue);
  clFinish(command_queue);

  // Read the memory buffer C on the device to the local variable C
  unsigned char *solution = malloc(PWD_LEN + 1);
  memset(solution, 0, PWD_LEN + 1);
  ret = clEnqueueReadBuffer(command_queue, solution_mem_obj, CL_TRUE, 0,
                            PWD_LEN, solution, 0, NULL, NULL);
  clFlush(command_queue);

  for (int i = 0; i < PWD_LEN; i++) printf("%02x", solution[i]);
  printf(" -> %s\n", solution);
  free(solution);
  printf("\n");

  clFinish(command_queue);
  // Clean up
free_kernel:
  clReleaseKernel(kernel);
free_program:
  clReleaseProgram(program);
free_sol_buf:
  clReleaseMemObject(solution_mem_obj);
free_target_buf:
  clReleaseMemObject(target_mem_obj);
free_queue:
  clReleaseCommandQueue(command_queue);
free_context:
  clReleaseContext(context);
free_source:
  free(source_str);
  return ret;
}