#ifndef _SCENE_H_
#define _SCENE_H_

typedef struct {
	float o[4];
	float r;
} __attribute__((__aligned__(16))) Sph;
typedef struct {
	int v1;
	int v2;
	int v3;
} __attribute__((__aligned__(16))) Trgl;
typedef struct {
	int matl_idx;
	enum {O_SPH, O_TRGL} t;
	union {
		Sph sph;
		Trgl trgl;
	};
} __attribute__((__aligned__(16))) Obj;

typedef struct {
	float o[4];
	float lum[4];
} __attribute__((__aligned__(16))) Light;

typedef struct {
	float amb[4];
	float diff[4];
	float spec[4];
	int phong;
} __attribute__((__aligned__(16))) Matl;

float o[4] = {0, 0, 0, 1};
float up[4] = {0, 1, 0, 0};
float gaze[4] = {0, 0, -1, 0};
float right[4] = {1, 0, 0, 0};
float d = 1;

float amb[4] = {25, 25, 25, 0};
Light lights[8]; int lightc = 0;
Obj objs[8]; int objc = 0;
Matl matls[8]; int matlc = 0;
float vertices[32]; int vertexc = 0;

void
scene1(){
	Light light = {{0, 0, 0, 1}, {1000, 1000, 1000, 0}};
	lights[0] = light;
	lightc = 1;

	float tmp_vertices[] = {-0.5, 0.5, -2, 1,
				-0.5, -0.5, -2, 1,
				0.5, -0.5, -2, 1,
				0.5, 0.5, -2, 1,
				0.75, 0.75, -2, 1,
				1, 0.75, -2, 1,
				0.875, 1, -2, 1,
				-0.875, 1, -2, 1};
	memcpy(vertices, tmp_vertices, 32 * sizeof(float));
	vertexc = 8;

	Obj obj;
	obj.matl_idx = 1;
	obj.t = O_TRGL;
	obj.trgl.v1 = 3; obj.trgl.v2 = 1; obj.trgl.v3 = 2;
	objs[0] = obj;
	objc = 1;

	Matl matl = {.amb = {1, 1, 1, 0}, .diff = {1, 1, 1, 0}, .spec = {1, 1, 1, 0}, .phong = 1};
	matls[0] = matl;
	matlc = 1;
}

#endif
