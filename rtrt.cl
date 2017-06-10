typedef struct {
	float4 o;
	float4 d;
	float4 color;
} Ray;

typedef struct {
	float4 o;
	float r;
} Sph;
typedef struct {
	int v1;
	int v2;
	int v3;
} Trgl;
typedef struct {
	int matl_idx;
	enum {O_SPH, O_TRGL} t;
	union {
		Sph sph;
		Trgl trgl;
	};
} Obj;

typedef struct {
	float4 o;
	float4 lum;
} Light;

__kernel void
genrays(__global Ray *rays, float4 o, float4 up, float4 gaze, float4 right, float d){
	int gidx = get_global_id(0), gidy = get_global_id(1);
	int width = get_global_size(0), height = get_global_size(1);
	rays[gidx + width*gidy].o = o;
	rays[gidx + width*gidy].d = d*gaze + 2*right*(gidx / width) - 2*up*(gidy / height);
	rays[gidx + width*gidy].color = (float4)(1, 0, 0, 0);
}

__kernel void
intersect(__global Ray *rays, __global Obj *objs, int objc, __global Light *lights){
}

__kernel void
red(__global Ray *rays, __write_only image2d_t fb){
	int x = get_global_id(0), width = get_global_size(0);
	int y = get_global_id(1);
	int2 coords = (int2)(x, y);
	float4 color = rays[x + width*y].color;
	write_imagef(fb, coords, color);
}
