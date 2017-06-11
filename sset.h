#ifndef _SSET_H_
#define _SSET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/opencl.h>

#include "sxmlc.h"
#include "v4_avx.h" /* only for interaction with old sset.c */
#include "linmath.h"

#define OUTF_MAX 64
#define VERTICES_MAX 32768
#define MESHOBJ_MAX 32768
#define OUTF_MAX 64
#define LIGHTS_MAX 8
#define MATLS_MAX 8
#define MESH_MAX 16
#define TRANS_MAX 16
#define OBJS_MAX 32768

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
	cl_float mirror[3];
	cl_float dummy4;
	cl_float transp[3];
	cl_float dummy5;
	cl_float eta;
	cl_int phong;
	enum {M_PHONG, M_MIRROR, M_DIELECTRIC} type;
	cl_float dummy6;
} __attribute__((__aligned__(16))) Matl;

typedef struct {
	Obj objs[OBJS_MAX];
	Light lights[LIGHTS_MAX];
	Matl matls[MATLS_MAX];
	cl_float amb[3];
	cl_float dummy;
	cl_int objc;
	cl_int lightc;
	cl_int matlc;
	cl_float dummy2;
} __attribute__((__aligned__(16))) Scene;
Scene scene;

float o[3];
float up[3];
float gaze[3];
float right[3];
float d;

v4 vertices[VERTICES_MAX]; int vertexc;
Light lights[LIGHTS_MAX]; int lightc;
Matl matls[MATLS_MAX]; int matlc;
m4 trs[TRANS_MAX]; int trc;
m4 scs[TRANS_MAX]; int scc;
m4 rots[TRANS_MAX]; int rotc;
Obj objs[OBJS_MAX]; int objc;

void sset(char *f);

#endif
