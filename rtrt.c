#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/glut.h>
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/opencl.h>

#include "sset.h"
#include "linmath.h"

#define SCRW 640
#define SCRH 640

float speedx = 0, speedz = 0, accx = 0, accz = 0;
float anglev = 0, angleh = 0;

GLuint fbtex = 0;
GLuint glprog = 0;
GLuint fbtexloc;
GLuint quadvbo;
unsigned char fb[SCRW*SCRH*4];
int winID;
/* just enough to render a texture on screen quad */
const char *vshadersrc = "#version 130\n"
"in vec3 modelspace;\n"
"out vec2 UV;\n"
"void main(){\n"
"	gl_Position =  vec4(modelspace,1);\n"
"	UV = (modelspace.xy+vec2(1,1))/2.0;\n"
"}\n";

const char *fshadersrc = "#version 130\n"
"uniform sampler2D fbtex;\n"
"in vec2 UV;\n"
"out vec4 color;\n"
"void main(){\n"
"	color = texture(fbtex, UV);\n"
"}\n";

GLfloat quad[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
};

cl_context ctx = 0;
cl_device_id dev = 0;
cl_command_queue cqueue = 0;
cl_program clprog = 0;

cl_kernel clmain = 0;

cl_mem clfb = 0;
cl_mem scenebuf = 0;

void
nukecl(){
	if(clfb) clReleaseMemObject(clfb);
	if(scenebuf) clReleaseMemObject(scenebuf);
	if(cqueue) clReleaseCommandQueue(cqueue);
	if(clprog) clReleaseProgram(clprog);
	if(ctx) clReleaseContext(ctx);
}

/* die */
void
die(char * f, ...){
	nukecl();
	va_list ap;
	va_start(ap, f);
	vfprintf(stderr, f, ap);
	va_end(ap);
	exit(1);
}


GLuint
mkglprog(){
	GLuint out;
	char log[16384];
	int nlog = 0;
	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vshader, 1, &vshadersrc, NULL);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_INFO_LOG_LENGTH, &nlog);
	if(nlog){
		glGetShaderInfoLog(vshader, nlog, NULL, log);
		die("couldn't compile vertex shader:\n%s\n", log);
	}

	glShaderSource(fshader, 1, &fshadersrc, NULL);
	glCompileShader(fshader);
	glGetShaderiv(fshader, GL_INFO_LOG_LENGTH, &nlog);
	if(nlog){
		glGetShaderInfoLog(fshader, nlog, NULL, log);
		die("couldn't compile fragment shader:\n%s\n", log);
	}

	out = glCreateProgram();
	glAttachShader(out, vshader);
	glAttachShader(out, fshader);
	glLinkProgram(out);

	glDetachShader(out, vshader);
	glDetachShader(out, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);
	return out;
}

void
errgl(int err, const char *desc){
	die("glfw error: %s\n", desc);
}

/* set up window, gl context, out texture, screen quad and shaders */
void
initgl(int argc,char** argv){

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(SCRW, SCRH);
	winID = glutCreateWindow("Screen");
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);

	if(glewInit() != GLEW_OK) die("couldn't init glew\n");

	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &quadvbo);
	glBindBuffer(GL_ARRAY_BUFFER, quadvbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_DYNAMIC_DRAW);

	GLuint fbtex;
	glGenTextures(1, &fbtex);
	glBindTexture(GL_TEXTURE_2D, fbtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCRW, SCRH, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	if(glGetError()) die("could not make empty texture\n");

	glprog = mkglprog();
	fbtexloc = glGetUniformLocation(glprog, "fbtex");
}

/* compile .cl file into program */
cl_program
mkclprog(char *fname){
	cl_int err;
	cl_program out;

	FILE *f = fopen(fname, "rb");
	if(!f) die("could not open %s\n", fname);
	fseek(f, 0, SEEK_END); int fsize = ftell(f); fseek(f, 0, SEEK_SET);

	char *dump = malloc(fsize+1);
	err = fread(dump, 1, fsize, f);
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

	cl_context_properties props[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
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

	char ext[1024];
	clGetDeviceInfo(devs[0], CL_DEVICE_EXTENSIONS, 1024, ext, NULL);
	printf("extensions: %s\n", ext);

	clprog = mkclprog("rtrt.cl");
	clmain = clCreateKernel(clprog, "clmain", NULL);
	if(!clmain) die("couldn't make kernel\n");
}


void
keydown(unsigned char key, int x, int y){

	switch(key){
	case 27: /* exit on ESC */
		glutDestroyWindow(winID);
		exit(0);
		break; 
	case 'w':
		accz = 0.01; break;
	case 's':
		accz = -0.01; break;
	case 'a':
		accx = -0.01; break;
	case 'd':
		accx = 0.01; break;
	case 'j': // turn left
		anglev = 0.01f; break;
	case 'l': // right
		anglev = -0.01f; break;
	case 'k': // look up
		angleh = 0.01f; break;
	case 'm': // look down
		angleh = -0.01f; break;
	default: break;
	}
}

void
keyup(unsigned char key, int x, int y){
	switch(key){
	case 27: /* exit on ESC */
		glutDestroyWindow(winID);
		exit(0);
		break; 
	case 'w':
	case 's':
		accz = 0; break;
	case 'a':
	case 'd':
		accx = 0; break;
	case 'j':
	case 'l':
		anglev = 0; break;
	case 'k':
	case 'm':
		angleh = 0; break;
	default: break;
	}
}

void mouse(int button, int state, int x, int y)
{
	if ((button == 3) || (button == 4)){
		if(state == GLUT_UP) return;
		if(button == 3) d += 0.1; //zoom in
		else if(button == 4) d -= 0.1; //zoom out
	}
}

void
step(){
	float base = glutGet(GLUT_ELAPSED_TIME);
	quat rotate, temp;
	size_t gsize[2] = {SCRW, SCRH};

	speedx += accx;
	speedz += accz;
	o[0] += gaze[0]*speedz;
	o[1] += gaze[1]*speedz; 
	o[2] += gaze[2]*speedz;
	o[0] += right[0]*speedx;
	o[1] += right[1]*speedx; 
	o[2] += right[2]*speedx;
	speedx *= 0.95;
	speedz *= 0.95;
	quat_rotate(rotate, anglev, up);
	quat_mul_vec3(temp, rotate, right);
	memcpy(right, temp, 3*sizeof(float));
	vec3_mul_cross(gaze, up, right);

	quat_rotate(rotate, angleh, right);
	quat_mul_vec3(temp, rotate, up);
	memcpy(up, temp, 3*sizeof(float));
	vec3_mul_cross(gaze, up, right);

	clSetKernelArg(clmain, 0, 4*sizeof(float), o);
	clSetKernelArg(clmain, 1, 4*sizeof(float), up);
	clSetKernelArg(clmain, 2, 4*sizeof(float), gaze);
	clSetKernelArg(clmain, 3, 4*sizeof(float), right);
	clSetKernelArg(clmain, 4, sizeof(float), &d);
	clEnqueueNDRangeKernel(cqueue, clmain, 2, NULL, gsize, NULL, 0, NULL, NULL);

	const size_t origin[3] = {0, 0, 0};
	const size_t region[3] = {SCRW, SCRH, 1};
	clEnqueueReadImage(cqueue, clfb, CL_TRUE, origin, region, 0, 0, fb, 0, NULL, NULL);

	glViewport(0, 0, SCRW, SCRH);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(glprog);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbtex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCRW, SCRH, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glUniform1i(fbtexloc, 0);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, quadvbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(0);

	glutSwapBuffers();

	char title[32];
	sprintf(title, "rtrt | %4.2f fps", 1000.0/(glutGet(GLUT_ELAPSED_TIME) - base));
	glutSetWindowTitle(title);
}

void
reshape(int w, int h){
	glutReshapeWindow(SCRW, SCRH);
}

int
main(int argc, char** argv){
	if(argc != 2) die("usage: %s <scene file>\n", argv[0]);

	printf("reading scene file...\n");
	sset(argv[1]);

	initgl(argc, argv);
	initcl();

	scenebuf = clCreateBuffer(ctx, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, sizeof(Scene), &scene, NULL);
	if(!scenebuf) die("couldn't make scene buffer\n");

	const cl_image_format fmt = {CL_RGBA, CL_UNORM_INT8};
	clfb = clCreateImage2D(ctx, CL_MEM_READ_WRITE, &fmt, SCRW, SCRH, 0, NULL, NULL);
	if(!clfb) die("couldn't make opencl framebuffer image\n");

	clSetKernelArg(clmain, 5, 4*sizeof(float), nplane);
	clSetKernelArg(clmain, 6, sizeof(cl_mem), &scenebuf);
	clSetKernelArg(clmain, 7, sizeof(cl_mem), &clfb);

	glutReshapeFunc(reshape);
	glutDisplayFunc(step);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutMouseFunc(mouse);
	glutIdleFunc(glutPostRedisplay);
	glutMainLoop();

	nukecl();
	return 0;
}
