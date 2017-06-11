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
} Trgl;
typedef struct {
	union {
		Sph sph;
		Trgl trgl;
	};
	int matl_idx;
	enum {O_SPH, O_TRGL} type;
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
	float4 rayd = d*gaze - right - up + 2*right*(gidx / (float)width) + 2*up*(gidy / (float)height);
	rays[gidx + width*gidy].o = o;
	rays[gidx + width*gidy].d = normalize(rayd);
	rays[gidx + width*gidy].color = (float4)(0, 0, 0, 1);
	rays[gidx + width*gidy].hit = 0;
}

/* https://gamedev.stackexchange.com/questions/96459/fast-ray-sphere-collision-code */
int
sphxray(Obj *obj, Ray *ray){
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

/* your old möller-trumbore */
int
trglxray(Obj *obj, Ray *ray){
	Trgl trgl = obj->trgl;
	float4 e1 = trgl.v2 - trgl.v1, e2 = trgl.v3 - trgl.v1;

	float3 p = cross(ray->d.xyz, e2.xyz);
	float det = dot(e1.xyz, p);
	float inv_det = 1/det;

	float3 t = (ray->o - trgl.v1).xyz;
	float u = dot(t, p) * inv_det;
	if(u<0 || u>1) return 0;

	float3 q = cross(t, e1.xyz);
	float v = dot(ray->d.xyz, q) * inv_det;
	if(v<0 || u+v>1) return 0;

	float dist = dot(e2.xyz, q) * inv_det;
	if(dist<0) return 0;

	ray->hit = 1;
	ray->dist = dist;
	ray->hitpoint = ray->o + dist*ray->d;
	ray->normal = normalize((float4)(cross(e1.xyz, e2.xyz), 0));
	ray->color = (float4)(1, 0, 1, 1);
	return 1;
}

__kernel void
intersect(__global Ray *rays, __global Obj *objs, int objc){
	int gidx = get_global_id(0), width = get_global_size(0);
	int gidy = get_global_id(1), id = gidx + width*gidy, i;
	Ray ray = rays[id];

	float nearest = 1e20;
	for(i=0; i<objc; i++){
		Obj obj = objs[i];
		if(obj.type == O_SPH){
			if(sphxray(&obj, &ray) && ray.dist < nearest){ rays[id] = ray; nearest = ray.dist; }
		}else{
			if(trglxray(&obj, &ray) && ray.dist < nearest){ rays[id] = ray; nearest = ray.dist; }
		}
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
