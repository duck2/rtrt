#ifndef _SCENE_H_
#define _SCENE_H_

typedef struct {
	cl_float o[3];
	cl_float dummy1;
	cl_float r;
	cl_float dummy2[3];
} __attribute__((__aligned__(16))) Sph;
typedef struct {
	cl_float v1[3];
	cl_float dummy1;
	cl_float v2[3];
	cl_float dummy2;
	cl_float v3[3];
	cl_float dummy3;
} __attribute__((__aligned__(16))) Trgl;
typedef struct {
	union {
		Sph sph;
		Trgl trgl;
	};
	cl_int matl_idx;
	enum {O_SPH, O_TRGL} type;
} __attribute__((__aligned__(16))) Obj;

typedef struct {
	cl_float o[3];
	cl_float dummy1;
	cl_float lum[3];
	cl_float dummy2;
} __attribute__((__aligned__(16))) Light;

typedef struct {
	cl_float amb[3];
	cl_float dummy1;
	cl_float diff[3];
	cl_float dummy2;
	cl_float spec[3];
	cl_float dummy3;
	cl_int phong;
	cl_float dummy4[3];
} __attribute__((__aligned__(16))) Matl;

typedef struct {
	Obj objs[8];
	Light lights[4];
	Matl matls[4];
	cl_float amb[3];
	cl_float dummy;
	cl_int objc;
	cl_int lightc;
	cl_int matlc;
	cl_float dummy2;
} __attribute__((__aligned__(16))) Scene;

Scene scene;

float o[3] = {0, 5, 25};
float up[3] = {0, 1, 0};
float gaze[3] = {0, 0, -1};
float right[3] = {1, 0, 0};
float d = 1;
float speedx = 0, speedz = 0, accx = 0, accz = 0;

float amb[3] = {25, 25, 25};
Light lights[8]; int lightc = 0;
Obj objs[8]; int objc = 0;
Matl matls[8]; int matlc = 0;

void
scene1(){
	Light light = {{10, 10, 10}, 0, {100000, 100000, 100000}, 0};
	scene.lights[0] = light;
	scene.lightc = 1;

	Obj obj = {.matl_idx = 1, .type = O_SPH, .sph = {.o = {0, 5, 0}, .r = 5}};
	scene.objs[0] = obj;

	obj.matl_idx = 0;
	obj.type = O_TRGL;
	obj.trgl.v1[0] = 100; obj.trgl.v1[1] = 0; obj.trgl.v1[2] = -100;
	obj.trgl.v2[0] = -100; obj.trgl.v2[1] = 0; obj.trgl.v2[2] = 100;
	obj.trgl.v3[0] = 100; obj.trgl.v3[1] = 0; obj.trgl.v3[2] = 100;
	scene.objs[1] = obj;

	obj.trgl.v1[0] = -100; obj.trgl.v1[1] = 0; obj.trgl.v1[2] = 100;
	obj.trgl.v2[0] = 100; obj.trgl.v2[1] = 0; obj.trgl.v2[2] = -100;
	obj.trgl.v3[0] = -100; obj.trgl.v3[1] = 0; obj.trgl.v3[2] = -100;
	scene.objs[2] = obj;

	scene.objc = 3;

	Matl matl = {.amb = {1, 1, 1}, .diff = {1, 1, 1}, .spec = {1, 1, 1}, .phong = 1};
	scene.matls[0] = matl;
	Matl matl2 = {.amb = {1, 1, 1}, .diff = {1, 0, 0}, .spec = {1, 1, 1}, .phong = 100};
	scene.matls[1] = matl2;
	scene.matlc = 2;

	scene.amb[0] = 25; scene.amb[1] = 25; scene.amb[2] = 25;
}

#endif
