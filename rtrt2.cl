#define EPSILON 0.0001f

typedef struct {
	float3 o;
	float3 d;
} Ray;
typedef struct {
	float3 point;
	float3 normal;
	float dist;
	int matl_idx;
} Hit;

typedef struct {
	float3 o;
	float r;
} Sph;
typedef struct {
	float3 v1;
	float3 v2;
	float3 v3;
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
	float3 o;
	float3 lum;
} Light;

typedef struct {
	float3 amb;
	float3 diff;
	float3 spec;
	int phong;
} Matl;

typedef struct {
	Obj objs[8];
	Light lights[4];
	Matl matls[4];
	float3 amb;
	int objc;
	int lightc;
	int matlc;
} Scene;

/* https://gamedev.stackexchange.com/questions/96459/fast-ray-sphere-collision-code */
int
sphxray(Obj *obj, Ray *ray, Hit *hit){
	Sph sph = obj->sph;
	float3 m = ray->o - sph.o;
	float b = dot(m, ray->d);
	float c = dot(m, m) - sph.r*sph.r;
	if(c>0 && b>0) return 0;

	float d = b*b - c;
	if(d<0) return 0;

	if(!hit) return 1;
	float t = hit->dist = -b - sqrt(d);
	hit->point = ray->o + t*ray->d;
	hit->normal = normalize(hit->point - sph.o);
	hit->matl_idx = obj->matl_idx;
	return 1;
}

/* your old möller-trumbore */
int
trglxray(Obj *obj, Ray *ray, Hit *hit){
	Trgl trgl = obj->trgl;
	float3 e1 = trgl.v2 - trgl.v1, e2 = trgl.v3 - trgl.v1;

	float3 p = cross(ray->d, e2);
	float det = dot(e1, p);
	float inv_det = 1/det;

	float3 t = (ray->o - trgl.v1);
	float u = dot(t, p) * inv_det;
	if(u<0 || u>1) return 0;

	float3 q = cross(t, e1);
	float v = dot(ray->d, q) * inv_det;
	if(v<0 || u+v>1) return 0;

	float dist = dot(e2, q) * inv_det;
	if(dist<0) return 0;

	if(!hit) return 1;
	hit->dist = dist;
	hit->point = ray->o + dist*ray->d;
	hit->normal = normalize(cross(e1, e2));
	hit->matl_idx = obj->matl_idx;
	return 1;
}

int
shadow(Ray ray, __global Scene *scene){
	for(int i=0; i<scene->objc; i++){
		Obj obj = scene->objs[i];
		if(obj.type == O_SPH && sphxray(&obj, &ray, NULL)) return 1;
		else if(obj.type == O_TRGL && trglxray(&obj, &ray, NULL)) return 1;
	}
	return 0;
}

float3
shade(Ray ray, Hit hit, __global Scene *scene){
	float3 out = scene->amb;
	Matl matl = scene->matls[hit.matl_idx];
	for(int i=0; i<scene->lightc; i++){
		Light light = scene->lights[i];
		float3 L = light.o - hit.point;

		Ray shray;
		shray.o = hit.point + EPSILON*L;
		shray.d = normalize(L);
		if(shadow(shray, scene)) continue;

		float3 lum = (1/dot(L, L)) * light.lum;
		float3 V = -1 * ray.d;
		float3 H = normalize(L + V);
		float3 N = hit.normal;
		float diff = fmax(dot(N, normalize(L)), 0);
		float spec = native_powr(fmax(dot(N, H), 0), matl.phong);
		out += lum*diff*matl.diff + lum*spec*matl.spec;
	}
	return (1 / 255.0f) * out;
}

float3
trace(Ray ray, __global Scene *scene){
	int found = 0;
	Hit hit, nhit;
	hit.dist = 1e20;
	for(int i=0; i<scene->objc; i++){
		Obj obj = scene->objs[i];
		if(obj.type == O_SPH){
			if(sphxray(&obj, &ray, &nhit) && nhit.dist < hit.dist){
				found = 1;
				hit = nhit;
			}
		}else{
			if(trglxray(&obj, &ray, &nhit) && nhit.dist < hit.dist){
				found = 1;
				hit = nhit;
			}
		}
	}
	if(found) return shade(ray, hit, scene);
	else return (float3)(0, 0, 0);
}

/* giant kernel. poor partitioning, lower memory traffic. */
__kernel void
clmain(float3 o, float3 up, float3 gaze, float3 right, float d, __global Scene *scene, __write_only image2d_t fb){
	int x = get_global_id(0), y = get_global_id(1);
	int width = get_global_size(0), height = get_global_size(1);
	int id = x + y*width;
	Ray ray;
	ray.o = o;
	ray.d = normalize(d*gaze - right - up + 2*right*(x/(float)width) + 2*up*(y/(float)height));
	float4 color = (float4)(trace(ray, scene), 1);
	write_imagef(fb, (int2)(x, y), color);
}
