#ifndef _SCENE_H_
#define _SCENE_H_

typedef struct {
	cl_float o[4];
	cl_float r;
	cl_float dummy[3];
} Sph;
typedef struct {
	cl_float v1[4];
	cl_float v2[4];
	cl_float v3[4];
} Trgl;
typedef struct {
	union {
		Sph sph;
		Trgl trgl;
	};
	cl_int matl_idx;
	enum {O_SPH, O_TRGL} type;
	float dummy[2];
} __attribute__((__aligned__(16))) Obj;

typedef struct {
	cl_float o[4];
	cl_float lum[4];
} __attribute__((__aligned__(16))) Light;

typedef struct {
	cl_float amb[4];
	cl_float diff[4];
	cl_float spec[4];
	cl_int phong;
} __attribute__((__aligned__(16))) Matl;

float o[4] = {0, 5, 25, 1};
float up[4] = {0, 1, 0, 0};
float gaze[4] = {0, 0, -1, 0};
float right[4] = {1, 0, 0, 0};
float d = 1;

float amb[4] = {25, 25, 25, 0};
Light lights[8]; int lightc = 0;
Obj objs[8]; int objc = 0;
Matl matls[8]; int matlc = 0;

void
scene1(){
	Light light = {{10, 10, 10, 1}, {100000, 100000, 100000, 0}};
	lights[0] = light;
	lightc = 1;

	Obj obj = {.matl_idx = 1, .type = O_SPH, .sph = {.o = {0, 5, 0, 1}, .r = 5}};
	objs[0] = obj;

	obj.matl_idx = 1;
	obj.type = O_TRGL;
	obj.trgl.v1[0] = 100; obj.trgl.v1[1] = 0; obj.trgl.v1[2] = -100; obj.trgl.v1[3] = 1;
	obj.trgl.v2[0] = -100; obj.trgl.v2[1] = 0; obj.trgl.v2[2] = 100; obj.trgl.v2[3] = 1;
	obj.trgl.v3[0] = 100; obj.trgl.v3[1] = 0; obj.trgl.v3[2] = 100; obj.trgl.v3[3] = 1;
	objs[1] = obj;
	
	obj.trgl.v1[0] = -100; obj.trgl.v1[1] = 0; obj.trgl.v1[2] = 100; obj.trgl.v1[3] = 1;
	obj.trgl.v2[0] = 100; obj.trgl.v2[1] = 0; obj.trgl.v2[2] = -100; obj.trgl.v2[3] = 1;
	obj.trgl.v3[0] = -100; obj.trgl.v3[1] = 0; obj.trgl.v3[2] = -100; obj.trgl.v3[3] = 1;
	objs[2] = obj;

	objc = 3;

	Matl matl = {.amb = {1, 1, 1, 0}, .diff = {1, 1, 1, 0}, .spec = {1, 1, 1, 0}, .phong = 1};
	matls[0] = matl;
	matlc = 1;
}

#endif
