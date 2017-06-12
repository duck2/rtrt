#define EPSILON 0.001f

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
	float3 mirror;
	float3 transp;
	float eta;
	int phong;
	enum {M_PHONG, M_MIRROR, M_DIELECTRIC} type;
} Matl;

typedef struct {
	Obj objs[32768];
	Light lights[8];
	Matl matls[8];
	float3 amb;
	int objc;
	int lightc;
	int matlc;
} Scene;

/* see jtdaugherty/T2's RayStack */
#define DEPTH 4
#define STACKDEPTH 16

typedef struct {
	Ray r[STACKDEPTH];
	int depth[STACKDEPTH];
	float3 weight[STACKDEPTH];
	int top;
} Rstack;

void
Rstack_push(Rstack *s, Ray *r, int depth, float3 weight){
	if(s->top > STACKDEPTH) return;
	s->r[s->top] = *r;
	s->depth[s->top] = depth;
	s->weight[s->top] = weight;
	s->top++;
}

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

	float t1 = -b + sqrt(d), t2 = -b - sqrt(d);
	float t = fabs(t1) < fabs(t2) ? (t1 > 0 ? t1 : t2) : (t2 > 0 ? t2 : t1);
	if(t<0) return 0;

	hit->dist = t;
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

	hit->dist = dist;
	hit->point = ray->o + dist*ray->d;
	hit->normal = normalize(cross(e1, e2));
	hit->matl_idx = obj->matl_idx;
	return 1;
}

int
shadow(Ray *ray, float maxd, __global Scene *scene){
	Hit hit;
	for(int i=0; i<scene->objc; i++){
		Obj obj = scene->objs[i];
		if(obj.type == O_SPH && sphxray(&obj, ray, &hit) && hit.dist < maxd) return 1;
		else if(obj.type == O_TRGL && trglxray(&obj, ray, &hit) && hit.dist < maxd) return 1;
	}
	return 0;
}

float3
shade_phong(Ray *ray, Hit hit, Matl matl, __global Scene *scene){
	float3 out = scene->amb;
	for(int i=0; i<scene->lightc; i++){
		Light light = scene->lights[i];
		float3 L = light.o - hit.point;
		float Ld = length(L);

		Ray shray;
		shray.o = hit.point + EPSILON*L;
		shray.d = normalize(L);
		if(shadow(&shray, Ld, scene)) continue;

		float3 lum = (1/(Ld*Ld)) * light.lum;
		float3 V = -1 * ray->d;
		float3 H = normalize(L + V);
		float3 N = hit.normal;
		float diff = fmax(dot(N, normalize(L)), 0);
		float spec = native_powr(fmax(dot(N, H), 0), matl.phong);
		out += lum*diff*matl.diff + lum*spec*matl.spec;
	}
	return (1 / 255.0f) * out;
}

/* conductor. bounce single ray */
float3
shade_mirror(Ray *ray, Rstack *stack, Hit hit, Matl matl, __global Scene *scene){
	int depth = stack->depth[stack->top];
	if(depth == DEPTH) return(float3)0;
	float3 weight = stack->weight[stack->top];

	Ray refl;
	refl.d = ray->d - 2*dot(ray->d, hit.normal)*hit.normal;
	refl.o = hit.point + EPSILON*refl.d;
	Rstack_push(stack, &refl, depth+1, matl.mirror*weight);
	return 0;
}

/* dielectric. bounce 2 rays, account for Beer's law if hit from inside. */
float3
shade_dielectric(Ray *ray, Rstack *stack, Hit hit, Matl matl, __global Scene *scene){
	int depth = stack->depth[stack->top];
	if(depth == DEPTH) return 0;
	float3 weight = stack->weight[stack->top];

	float3 beer = 1;
	float3 I = ray->d, N = hit.normal;
	float cosi = -dot(I, N);
	float etai = 1, etat = matl.eta, eta = 1/matl.eta;
	if(cosi < 0){
		cosi *= -1;
		eta = etai = matl.eta; etat = 1;
		N *= -1;
		beer = native_powr(matl.transp, hit.dist);
	}

	Ray refl;
	refl.d = ray->d + 2*cosi*N;
	refl.o = hit.point + EPSILON*refl.d;

	float sin2phi = eta*eta*(1 - cosi*cosi);
	if(sin2phi>1){
		Rstack_push(stack, &refl, depth+1, beer*weight);
		return 0;
	}

	float Ro = (etai - etat) / (etai + etat); Ro *= Ro;
	float R = Ro + (1 - Ro) * native_powr(1-cosi, 5);

	Ray refr;
	refr.d = eta*I + (eta*cosi - sqrt(1-sin2phi))*N;
	refr.o = hit.point + EPSILON*refr.d;
	Rstack_push(stack, &refr, depth+1, (1-R)*beer*weight);
	Rstack_push(stack, &refl, depth+1, R*beer*weight);
	return 0;
}

float3
trace(Ray *ray, Rstack *stack, __global Scene *scene){
	int found = 0;
	Hit hit, nhit;
	hit.dist = 1e20;

	for(int i=0; i<scene->objc; i++){
		Obj obj = scene->objs[i];
		if(obj.type == O_SPH){
			if(sphxray(&obj, ray, &nhit) && nhit.dist < hit.dist){
				found = 1;
				hit = nhit;
			}
		}else{
			if(trglxray(&obj, ray, &nhit) && nhit.dist < hit.dist){
				found = 1;
				hit = nhit;
			}
		}
	}
	if(!found) return (float3)0;

	Matl matl = scene->matls[hit.matl_idx];
	switch(matl.type){
	case M_PHONG: return shade_phong(ray, hit, matl, scene);
	case M_MIRROR: return shade_mirror(ray, stack, hit, matl, scene);
	case M_DIELECTRIC: return shade_dielectric(ray, stack, hit, matl, scene);
	}
}

float3
rectrace(Ray ray, __global Scene *scene){
	Rstack stack;
	stack.top = 0;
	Rstack_push(&stack, &ray, 0, (float3)1);

	float3 out = (float3)(0, 0, 0);
	while(stack.top > 0){
		stack.top--;
		out += stack.weight[stack.top] * trace(&stack.r[stack.top], &stack, scene);
	}
	return out;
}

/* giant kernel. poor partitioning, lower memory traffic. */
__kernel void
clmain(float3 o, float3 up, float3 gaze, float3 right, float d, float4 nplane, __global Scene *scene, __write_only image2d_t fb){
	int x = get_global_id(0), y = get_global_id(1);
	int width = get_global_size(0), height = get_global_size(1);
	int id = x + y*width;
	
	float plw = nplane.y - nplane.x;
	float plh = nplane.w - nplane.z;
	Ray ray;
	ray.o = o;
	ray.d = normalize(d*gaze - 0.5f*plw*right - 0.5f*plh*up + plw*right*(x/(float)width) + plh*up*(y/(float)height));
	float4 color = (float4)(rectrace(ray, scene), 1);
	write_imagef(fb, (int2)(x, y), color);
}
