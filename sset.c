#include "sset.h"

static void
die(char * f, ...){
	va_list ap;
	va_start(ap, f);
	vfprintf(stderr, f, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
v4_to_float3(void *out, v4 in){
	*(float *)(out) = in.x;
	*(float *)(out+sizeof(float)) = in.y;
	*(float *)(out+2*sizeof(float)) = in.z;
}

/* read into a v4 from something like 1 1 1.
 * returns 1 if all OK, 0 if not all OK. */
int
sreadv4(char *in, v4 *v){
	double x, y, z;
	if(sscanf(in, "%lf %lf %lf", &x, &y, &z) != 3) return 0;
	v4 tmp = mkv4(x, y, z, 0);
	memcpy((void *)v, (void *)&tmp, sizeof(v4));
	return 1;
}

/* rtrt: read only the first cam */
void
mkcam(XMLNode * node){
	v4 tmp;
	int i;
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("Position", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(o, tmp);
		}else if(strcmp("Gaze", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(gaze, norv4(tmp));
			vec3_norm(gaze, gaze);
		}else if(strcmp("Up", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(up, norv4(tmp));
			vec3_norm(up, up);
		}else if(strcmp("NearDistance", n->tag) == 0){
			sscanf(n->text, "%f", &d);
		}else if(strcmp("NearPlane", n->tag) == 0){
			sscanf(n->text, "%f %f %f %f", &nplane[0], &nplane[1], &nplane[2], &nplane[3]);
		}
	}
	/* orthonormalize */
	vec3_mul_cross(right, gaze, up);
	vec3_norm(right, right);
	vec3_mul_cross(up, right, gaze);
	vec3_norm(up, up);
}

void
mkptlight(XMLNode * node){
	Light q;
	v4 tmp;
	int i;
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("Position", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(&q.o, tmp);
		}else if(strcmp("Intensity", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(&q.lum, tmp);
		}
	}
	scene.lights[scene.lightc++] = q;
}

void
mklights(XMLNode * node){
	v4 tmp;
	int i;
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("AmbientLight", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(&scene.amb, tmp);
		}else if(strcmp("PointLight", n->tag) == 0){
			mkptlight(n);
		}else if(strcmp("AreaLight", n->tag) == 0){
			mkptlight(n);
		}
	}
}

void
mkmatl(XMLNode * node){
	Matl q;
	v4 tmp;
	memset(&q, 0, sizeof(Matl));
	int i, mirror = 0, transp = 0;
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("AmbientReflectance", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(&q.amb, tmp);
		}else if(strcmp("DiffuseReflectance", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(&q.diff, tmp);
		}else if(strcmp("SpecularReflectance", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(&q.spec, tmp);
		}else if(strcmp("PhongExponent", n->tag) == 0){
			sscanf(n->text, "%d", (int *)&q.phong);
		}else if(strcmp("MirrorReflectance", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(&q.mirror, tmp);
			if(q.mirror[0] != 0 && q.mirror[1] != 0 && q.mirror[2] != 0) mirror = 1;
		}else if(strcmp("Transparency", n->tag) == 0){
			sreadv4(n->text, &tmp);
			v4_to_float3(&q.transp, tmp);
			if(q.transp[0] != 0 && q.transp[1] != 0 && q.transp[2] != 0) transp = 1;
		}else if(strcmp("RefractionIndex", n->tag) == 0){
			sscanf(n->text, "%f", (float *)&q.eta);
		}
	}
	if(mirror) if(transp) die("mkmatl: Undefined material %d has both MirrorReflectance and Transparency\n");
			else q.type = M_MIRROR;
	else if(transp) q.type = M_DIELECTRIC;
	else q.type = M_PHONG;
	scene.matls[scene.matlc++] = q;
}

void
mkmatls(XMLNode * node){
	int i;
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("Material", n->tag) == 0) mkmatl(n);
		else die("mkmatls: Unknown tag name in Materials: %s\n", n->tag);
	}
}

/* vertices are points. their t is 1 */
void
mkvertices(XMLNode * node){
	for(char * v = strtok(node->text, "\n"); v != NULL; v = strtok(NULL, "\n")){
		if(!sreadv4(v, &(vertices[vertexc++]))) vertexc--;
		vertices[vertexc-1].t = 1;
	}
}

/* first we reverse the string, then parse like 1t 1s 1r 2t. this way we can go along multiplying matrices */
m4
mktransmtx(XMLNode * node){
	int i, j, x; char t; m4 out = mkm4(mkv4(1, 0, 0, 0), mkv4(0, 1, 0, 0), mkv4(0, 0, 1, 0), mkv4(0, 0, 0, 1));

	char * s = node->text;
	if(s && strlen(s)) for(i=0, j=strlen(node->text)-1; i < j; i++,j--){ t = s[i]; s[i] = s[j]; s[j] = t; }
	
	for(char * v = strtok(s, " "); v != NULL; v = strtok(NULL, " ")){
		if(sscanf(v, "%d%c", &x, &t) == 2){
			switch(t){
			case 't': out = mulm4(out, trs[x-1]); break;
			case 'r': out = mulm4(out, rots[x-1]); break;
			case 's': out = mulm4(out, scs[x-1]); break;
			default: die("mktransmtx: bogus transformation type: %c\n", t);
			}
		}
	}
	return out;
}

/* we precompute trgl normal N, d and perpendicular planes to trgl N1, d1, N2, d2.
 * see rt.c:/^trglxray/ also see dat.h:/^struct Trgl/ */
Obj
preptrgl(int matl_idx, v4 v1, v4 v2, v4 v3){
	Obj q;
	q.type = O_TRGL;
	q.matl_idx = matl_idx - 1;
	v4_to_float3(&q.trgl.v1, v1);
	v4_to_float3(&q.trgl.v2, v2);
	v4_to_float3(&q.trgl.v3, v3);
	return q;
}
void
mktrgl(XMLNode * node){
	int matl_idx;
	int i, a, b, c;
	m4 q = mkm4(mkv4(1, 0, 0, 0), mkv4(0, 1, 0, 0), mkv4(0, 0, 1, 0), mkv4(0, 0, 0, 1));
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("Material", n->tag) == 0){
			sscanf(n->text, "%d", &matl_idx);
		}else if(strcmp("Indices", n->tag) == 0){
			sscanf(n->text, "%d %d %d", &a, &b, &c);
		}else if(strcmp("Transformations", n->tag) == 0){
			q = mktransmtx(n);
		}else{
			die("mktrgl: Unknown tag name in Triangle: %s\n", n->tag);
		}
	}
	v4 p1 = vecm4(q, vertices[a-1]), p2 = vecm4(q, vertices[b-1]), p3 = vecm4(q, vertices[c-1]);
	scene.objs[scene.objc++] = preptrgl(matl_idx, p1, p2, p3);
}

/* first precompute given triangles. then loop through them, add their normals to respective vertex normals, normalize
 * see dat.h:/Mesh/ */
int trgls[MESHOBJ_MAX][3];
Obj outobjs[MESHOBJ_MAX];
void
mkmesh(XMLNode * node){
	int matl_idx;
	int i, trglc=0, off;
	m4 q = mkm4(mkv4(1, 0, 0, 0), mkv4(0, 1, 0, 0), mkv4(0, 0, 1, 0), mkv4(0, 0, 0, 1));

	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("Material", n->tag) == 0){
			sscanf(n->text, "%d", &matl_idx);
		}else if(strcmp("Faces", n->tag) == 0){
			off=0;
			if(n->n_attributes && strcmp(n->attributes[0].name, "vertexOffset") == 0) sscanf(n->attributes[0].value, "%d", &off);
			int a, b, c;
			for(char * v = strtok(n->text, "\n"); v != NULL; v = strtok(NULL, "\n")){
				if(sscanf(v, "%d %d %d", &a, &b, &c) == 3){
					trgls[trglc][0] = a+off; trgls[trglc][1] = b+off; trgls[trglc][2] = c+off;
					trglc++;
				}
			}
		}else if(strcmp("Transformations", n->tag) == 0){
			q = mktransmtx(n);
		}else{
			die("mkmesh: Unknown tag name in Mesh: %s\n", n->tag);
		}
	}

	for(i=0; i<trglc; i++){
		v4 p1 = vecm4(q, vertices[trgls[i][0]-1]), p2 = vecm4(q, vertices[trgls[i][1]-1]), p3 = vecm4(q, vertices[trgls[i][2]-1]);
		outobjs[i] = preptrgl(matl_idx, p1, p2, p3);
	}

	for(i=0; i<trglc; i++) scene.objs[scene.objc++] = outobjs[i];
}

void
mksph(XMLNode * node){
	Obj q;
	q.type = O_SPH;
	int i;
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("Material", n->tag) == 0){
			sscanf(n->text, "%d", &q.matl_idx);
			q.matl_idx--;
		}else if(strcmp("Center", n->tag) == 0){
			int a;
			sscanf(n->text, "%d", &a);
			v4_to_float3(&q.sph.o, vertices[a-1]);
		}else if(strcmp("Radius", n->tag) == 0){
			sscanf(n->text, "%f", &q.sph.r);
		}
	}
	scene.objs[scene.objc++] = q;
}

void
mkobjs(XMLNode * node){
	int i;
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("Mesh", n->tag) == 0){
			mkmesh(n);
		}else if(strcmp("Triangle", n->tag) == 0){
			mktrgl(n);
		}else if(strcmp("Sphere", n->tag) == 0){
			mksph(n);
		}else{
			die("mkobjs: Unknown tag name in Objects: %s\n", n->tag);
		}
	}
}

/* see http://ksuweb.kennesaw.edu/~plaval/math4490/rotgen.pdf */
void
mktrans(XMLNode * node){
	int i; double phi, x, y, z;
	for(i=0; i<node->n_children; i++){
		XMLNode * n = node->children[i];
		if(strcmp("Translation", n->tag) == 0){
			sscanf(n->text, "%lf %lf %lf", &x, &y, &z);
			m4 q = mkm4(mkv4(1, 0, 0, x), mkv4(0, 1, 0, y), mkv4(0, 0, 1, z), mkv4(0, 0, 0, 1));
			trs[trc++] = q;
		}else if(strcmp("Scaling", n->tag) == 0){
			sscanf(n->text, "%lf %lf %lf", &x, &y, &z);
			m4 q = mkm4(mkv4(x, 0, 0, 0), mkv4(0, y, 0, 0), mkv4(0, 0, z, 0), mkv4(0, 0, 0, 1));
			scs[scc++] = q;
		}else if(strcmp("Rotation", n->tag) == 0){
			sscanf(n->text, "%lf %lf %lf %lf", &phi, &x, &y, &z);
			v4 nor = norv4(mkv4(x, y, z, 0));
			phi = phi * M_PI / 180.0; double C = cos(phi), S = sin(phi), t = 1-C;
			m4 q = mkm4(mkv4(t*nor.x*nor.x + C, t*nor.x*nor.y - S*nor.z, t*nor.x*nor.z + S*nor.y, 0),
						mkv4(t*nor.x*nor.y + S*nor.z, t*nor.y*nor.y + C, t*nor.y*nor.z - S*nor.x, 0),
						mkv4(t*nor.x*nor.z - S*nor.y, t*nor.y*nor.z + S*nor.x, t*nor.z*nor.z + C, 0),
						mkv4(0, 0, 0, 1));
			rots[rotc++] = q;
		}else{
			die("mkobjs: Unknown tag name in Transformations: %s\n", n->tag);
		}
	}
}

void
sset(char * f){
	scene.lightc = vertexc = scene.matlc = scene.objc = 0;
	XMLDoc doc;
	XMLDoc_init(&doc);
	XMLDoc_parse_file_DOM(f, &doc);
	int i;
	XMLNode * root = doc.nodes[doc.i_root];
	for(i=0; i<root->n_children; i++){
		XMLNode * node = root->children[i];
		if(strcmp("Camera", node->tag) == 0){
			mkcam(node);
		}else if(strcmp("Lights", node->tag) == 0){
			mklights(node);
		}else if(strcmp("Materials", node->tag) == 0){
			mkmatls(node);
		}else if(strcmp("Textures", node->tag) == 0){
			printf("Ignoring Textures.\n");
		}else if(strcmp("Transformations", node->tag) == 0){
			mktrans(node);
		}else if(strcmp("VertexData", node->tag) == 0){
			mkvertices(node);
		}else if(strcmp("TexCoordData", node->tag) == 0){
			printf("Ignoring TexCoordData.\n");
		}else if(strcmp("Objects", node->tag) == 0){
			mkobjs(node);
		}
	}
}
