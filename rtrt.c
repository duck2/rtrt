#include <stdio.h>

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/opencl.h>

#define SIZE 512

cl_context ctx = 0;
cl_device_id dev = 0;
cl_command_queue cqueue = 0;
cl_program prog = 0;
cl_kernel kernel = 0;
cl_mem objs[3] = {0, 0, 0};

void
cleanup(){
	int i;
	for(i=0; i<3; i++) if(objs[i]) clReleaseMemObject(objs[i]);
	if(cqueue) clReleaseCommandQueue(cqueue);
	if(kernel) clReleaseKernel(kernel);
	if(prog) clReleaseProgram(prog);
	if(ctx) clReleaseContext(ctx);
}

/* die */
void
die(char * f, ...){
	cleanup();
	va_list ap;
	va_start(ap, f);
	vfprintf(stderr, f, ap);
	va_end(ap);
	exit(1);
}

/* compile .cl file into program */
cl_program
mkprog(char *fname){
	cl_int err;
	cl_program out;

	FILE *f = fopen(fname, "rb");
	if(!f) die("could not open %s\n", fname);
	fseek(f, 0, SEEK_END); int fsize = ftell(f); fseek(f, 0, SEEK_SET);

	char *dump = malloc(fsize+1);
	fread(dump, 1, fsize, f);
	dump[fsize] = 0;
	fclose(f);

	out = clCreateProgramWithSource(ctx, 1, (const char **)&dump, NULL, NULL);
	if(!out) die("could not create program for %s\n", fname);

	err = clBuildProgram(out, 0, NULL, NULL, NULL, NULL);
	if(err != CL_SUCCESS){
		char log[16384];
		clGetProgramBuildInfo(out, dev, CL_PROGRAM_BUILD_LOG, 16384, log, NULL);
		die("couldn't compile %s. log:\n%s", fname, log);
	}

	free(dump);
	return out;
}

/* make context and command queue, compile kernels */
void
init(void){
	cl_int err;
	cl_platform_id platform;
	err = clGetPlatformIDs(1, &platform, NULL);
	if(err != CL_SUCCESS) die("no opencl platforms\n");

	cl_context_properties props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
	ctx = clCreateContextFromType(props, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
	if(err != CL_SUCCESS) die("no gpu context\n");

	cl_device_id devs[8]; /* XXX: assume <8 devices */
	err = clGetContextInfo(ctx, CL_CONTEXT_DEVICES, 8, devs, NULL);
	if(err != CL_SUCCESS) die("couldn't get device info\n");

	cqueue = clCreateCommandQueue(ctx, devs[0], 0, NULL);
	if(!cqueue) die("couldn't make command queue\n");
	dev = devs[0];

	prog = mkprog("hello.cl");
	kernel = clCreateKernel(prog, "hello", NULL);
	if(!kernel) die("couldn't make kernel\n");
}

int
main(){
	init();
	float a[SIZE], b[SIZE], out[SIZE];
	int i;
	for(i=0; i<SIZE; i++){
		a[i] = i;
		b[i] = i*2;
	}
	objs[0] = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * SIZE, a, NULL);
	objs[1] = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * SIZE, b, NULL);
	objs[2] = clCreateBuffer(ctx, CL_MEM_READ_WRITE, sizeof(float) * SIZE, NULL, NULL);
	if(!objs[0] || !objs[1] || !objs[2]) die("couldn't make memory objects\n");

	clSetKernelArg(kernel, 0, sizeof(cl_mem), &objs[0]);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &objs[1]);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), &objs[2]);

	size_t gsize[1] = {SIZE}, lsize[1] = {1};
	clEnqueueNDRangeKernel(cqueue, kernel, 1, NULL, gsize, lsize, 0, NULL, NULL);
	clEnqueueReadBuffer(cqueue, objs[2], CL_TRUE, 0, sizeof(float) * SIZE, out, 0, NULL, NULL);

	cleanup();
	return 0;
}
