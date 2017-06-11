typedef struct {
	float4 o;
	float4 d;
	float4 color;
	float4 hitpoint;
	float4 normal;
	float dist;
	int hit;
} Ray;

typedef struct {
	float4 o;
	float r;
} Sph;
typedef struct {
	float4 v1;
	float4 v2;
	float4 v3;
	float4 n;
} Trgl;
typedef struct {
	union {
		Sph sph;
		Trgl trgl;
	};
	int matl_idx;
	int type;
} Obj;

typedef struct {
	float4 o;
	float4 lum;
} Light;

typedef struct {
	float4 amb;
	float4 diff;
	float4 spec;
	int phong;
} Matl;

__kernel void
genrays(__global Ray *rays, float4 o, float4 up, float4 gaze, float4 right, float d){
	int gidx = get_global_id(0), gidy = get_global_id(1);
	int width = get_global_size(0), height = get_global_size(1);
	float4 rayd = d*gaze - right + up + 2*right*(gidx / (float)width) - 2*up*(gidy / (float)height);
	rays[gidx + width*gidy].o = o;
	rays[gidx + width*gidy].d = normalize(rayd);
	rays[gidx + width*gidy].color = (float4)(0, 0, 0, 1);
	rays[gidx + width*gidy].hit = 0;
}

/* https://gamedev.stackexchange.com/questions/96459/fast-ray-sphere-collision-code */
int
sphxray(Obj * obj, Ray * ray){
	Sph sph = obj->sph;
	float4 m = ray->o - sph.o;
	float b = dot(m, ray->d);
	float c = dot(m, m) - sph.r*sph.r;
	if(c>0 && b>0) return 0;

	float d = b*b - c;
	if(d<0) return 0;

	ray->hit = 1;
	float t = ray->dist = -b - sqrt(d);
	ray->hitpoint = ray->o + t*ray->d;
	ray->normal = ray->hitpoint - sph.o;
	ray->color = (float4)(1, 1, 1, 1);
	return 1;
}

__kernel void
intersect(__global Ray *rays, __global Obj *objs, int objc){
	int gidx = get_global_id(0), width = get_global_size(0);
	int gidy = get_global_id(1), id = gidx + width*gidy, i;
	Ray ray = rays[id];

	for(i=0; i<objc; i++){
		Obj obj = objs[i];
		if(sphxray(&obj, &ray)) rays[id] = ray;
	}
}

__kernel void
final(__global Ray *rays, __write_only image2d_t fb){
	int x = get_global_id(0), width = get_global_size(0);
	int y = get_global_id(1);
	int2 coords = (int2)(x, y);
	float4 color = rays[x + width*y].color;
	write_imagef(fb, coords, color);
}
