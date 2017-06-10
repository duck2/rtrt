#include <stdio.h>
#include <unistd.h>

#include <GLFW/glfw3.h>
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/opencl.h>

#define SIZE 512
#define SCRW 800
#define SCRH 600

GLFWwindow *win = NULL;
GLuint fbtex = 0;

/* just enough to render a texture on screen quad */
const char *vshader = ""
"#version 130"
"precision mediump float;"
"const madd = vec2(0.5, 0.5);"
"in vec2 texcoords;"
"out vec2 texcoord;"
"void main(){"
"	texcoord = texcoords.xy * madd + madd;"
"	gl_Position = vec4(texcoords.xy, 0.0, 1.0);"
"}";

const char *fshader = ""
"#version 130"
"precision mediump float;"
"uniform sampler2D fbtex;"
"in vec2 texcoord;
"out vec4 color;"
"void main(){"
"	color = texture2D(fbtex, texcoord);
"}";

cl_context ctx = 0;
cl_device_id dev = 0;
cl_command_queue cqueue = 0;
cl_program prog = 0;
cl_kernel kernel = 0;
cl_mem objs[3] = {0, 0, 0};

/* cleanup */
void
nukegl(){
	if(fbtex) glDeleteTextures(1, &fbtex);
	if(win) glfwDestroyWindow(win);
	glfwTerminate();
}

void
nukecl(){
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
	nukecl();
	nukegl();
	va_list ap;
	va_start(ap, f);
	vfprintf(stderr, f, ap);
	va_end(ap);
	exit(1);
}

void
errgl(int err, const char *desc){
	die("glfw error: %s\n", desc);
}

/* set up window, gl context, out texture, screen quad and passthrough shaders */
void
initgl(){
	if(!glfwInit()) die("couldn't init glfw\n");
	glfwSetErrorCallback(errgl);
	win = glfwCreateWindow(SCRW, SCRH, "rtrt", NULL, NULL);
	if(!win) die("couldn't create window\n");

	glfwSetWindowSizeLimits(win, SCRW, SCRH, SCRW, SCRH);
	glfwMakeContextCurrent(win);
	glViewport(0, 0, SCRW, SCRH);

	GLuint fbtex;
	glGenTextures(1, &fbtex);
	glBindTexture(GL_TEXTURE_2D, fbtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCRW, SCRH, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if(glGetError()) die("could not make empty texture\n");
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

	clUnloadCompiler();
	free(dump);
	return out;
}

/* make context and command queue, compile kernels */
void
initcl(){
	cl_int err;
	cl_platform_id platform;
	err = clGetPlatformIDs(1, &platform, NULL);
	if(err != CL_SUCCESS) die("no opencl platforms\n");

	cl_context_properties props[] = {CL_GL_CONTEXT_KHR, (cl_context_properties)glfwGetCurrentContext(),
								CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
	ctx = clCreateContextFromType(props, CL_DEVICE_TYPE_GPU, NULL, NULL, &err);
	if(err != CL_SUCCESS) die("no gpu context\n");

	cl_device_id devs[8]; /* XXX: assume <8 devices */
	err = clGetContextInfo(ctx, CL_CONTEXT_DEVICES, 8, devs, NULL);
	if(err != CL_SUCCESS) die("couldn't get context info\n");

	char name[1024];
	err = clGetDeviceInfo(devs[0], CL_DEVICE_NAME, 1024, name, NULL);
	if(err != CL_SUCCESS) die("couldn't get device info\n");
	printf("got device %s\n", name);

	cqueue = clCreateCommandQueue(ctx, devs[0], 0, NULL);
	if(!cqueue) die("couldn't make command queue\n");
	dev = devs[0];

	prog = mkprog("hello.cl");
	kernel = clCreateKernel(prog, "hello", NULL);
	if(!kernel) die("couldn't make kernel\n");
}

void
step(){
	usleep(2000); /* limits to ~500 fps */
	glfwSwapBuffers(win);
	glfwPollEvents();
}

int
main(){
	initgl();
	initcl();
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

	char title[32];
	while(!glfwWindowShouldClose(win)){
		double time = glfwGetTime();
		step();
		int fps = 1 / (glfwGetTime() - time);
		snprintf(title, 32, "rtrt | %d fps", fps);
		glfwSetWindowTitle(win, (const char *)title);
	}

	nukecl();
	nukegl();
	return 0;
}
