#ifndef _V4_AVX_H_
#define _V4_AVX_H_

/* minimalistic mat4d/vec4d library with AVX. mat4d is m4, vec4d is v4.
 * #define is your friend if you want to port code using any other name.
 * add -mavx to your CFLAGS. you might also want to use -O3 -Winline for the inlines.
 * v4_avx.c in the repository has extern inline decls which can be used to make a v4_avx.o
 * @author Fahrican Koşar, duck2 at protonmail.com, MIT license. */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <immintrin.h>

#define clamp(x) (fmin(1, fmax(x, 0)))

/* v4p holds the packed 128 bit vector. */
struct v4 {
	union {
		__m256d v4p;
		struct {
			union {double x; double r;};
			union {double y; double g;};
			union {double z; double b;};
			union {double t; double a;};
		};
	};
};
typedef struct v4 v4;

struct m4 {
	union {
		v4 row[4];
		double d[16];
		double dd[4][4];
	};
};
typedef struct m4 m4;

inline v4
mkv4(double x, double y, double z, double t){
	v4 out;
	out.v4p= _mm256_set_pd(t, z, y, x);
	return out;
}

inline v4
addv4(v4 p, v4 q){
	v4 out;
	out.v4p = _mm256_add_pd(p.v4p, q.v4p);
	return out;
}

inline v4
subv4(v4 p, v4 q){
	v4 out;
	out.v4p = _mm256_sub_pd(p.v4p, q.v4p);
	return out;
}

/* elementwise product */
inline v4
elpv4(v4 p, v4 q){
	v4 out;
	out.v4p = _mm256_mul_pd(p.v4p, q.v4p);
	return out;
}

inline v4
scv4(double s, v4 p){
	v4 out; __m256d sc = _mm256_set1_pd(s);
	out.v4p = _mm256_mul_pd(p.v4p, sc);
	return out;
}

/* XXX: I believe there is a more quick way to do this */
inline double
dotv4(v4 p, v4 q){
	v4 x;
	x.v4p = _mm256_mul_pd(p.v4p, q.v4p);
	return x.x + x.y + x.z + x.t;
}

/* there is no cross product in 4d. we assume 3d */
#define MASK _MM_SHUFFLE(3, 0, 2, 1)
inline v4
crv4(v4 p, v4 q){
	v4 out;
	__m256d pt = _mm256_permute4x64_pd(p.v4p,  MASK);
	__m256d qt = _mm256_permute4x64_pd(q.v4p, MASK);
	__m256d pqt = _mm256_sub_pd(_mm256_mul_pd(p.v4p, qt), _mm256_mul_pd(q.v4p, pt));
	out.v4p = _mm256_permute4x64_pd(pqt, MASK);
	return out;
}
#undef MASK

inline v4
norv4(v4 p){
	v4 out; double a = 1/sqrt(dotv4(p, p));
	__m256d dn = _mm256_set1_pd(a);
	out.v4p = _mm256_mul_pd(p.v4p, dn);
	return out;
}

inline double
lenv4(v4 p){
	return sqrt(dotv4(p, p));
}

/* m4 :D:D we carelessly pass 64 byte matrices by value, relying on the compiler to inline */

inline m4
mkm4(v4 x, v4 y, v4 z, v4 t){
	m4 out;
	out.row[0] = x; out.row[1] = y; out.row[2] = z; out.row[3] = t;
	return out;
}

/* if you feel so AVX you can load from out.row[0] to __m256's and op them with _mm256256_op_pd. */
inline m4
addm4(m4 p, m4 q){
	m4 out;
	out.row[0] = addv4(p.row[0], q.row[0]); out.row[1] = addv4(p.row[1], q.row[1]);
	out.row[2] = addv4(p.row[2], q.row[2]); out.row[3] = addv4(p.row[3], q.row[3]);
	return out;
}

inline m4
subm4(m4 p, m4 q){
	m4 out;
	out.row[0] = subv4(p.row[0], q.row[0]); out.row[1] = subv4(p.row[1], q.row[1]);
	out.row[2] = subv4(p.row[2], q.row[2]); out.row[3] = subv4(p.row[3], q.row[3]);
	return out;
}

inline m4
elpm4(m4 p, m4 q){
	m4 out;
	out.row[0] = elpv4(p.row[0], q.row[0]); out.row[1] = elpv4(p.row[1], q.row[1]);
	out.row[2] = elpv4(p.row[2], q.row[2]); out.row[3] = elpv4(p.row[3], q.row[3]);
	return out;
}

inline m4
scm4(double s, m4 p){
	m4 out;
	out.row[0] = scv4(s, p.row[0]); out.row[1] = scv4(s, p.row[1]);
	out.row[2] = scv4(s, p.row[2]); out.row[3] = scv4(s, p.row[3]);
	return out;
}

/* see http://stackoverflow.com/questions/18499971/ */
inline m4
mulm4(m4 p, m4 q){
	m4 out; __m256d xs, ys, zs, ts; int i;
	for(i=0; i<4; i++){
		xs = _mm256_set1_pd(p.row[i].x); ys = _mm256_set1_pd(p.row[i].y);
		zs = _mm256_set1_pd(p.row[i].z); ts = _mm256_set1_pd(p.row[i].t);
		out.row[i].v4p = _mm256_add_pd(_mm256_add_pd(_mm256_mul_pd(xs, q.row[0].v4p), _mm256_mul_pd(ys, q.row[1].v4p)),
								_mm256_add_pd(_mm256_mul_pd(zs, q.row[2].v4p), _mm256_mul_pd(ts, q.row[3].v4p)));
	}
	return out;
}

/* XXX: there is certainly a quicker way */
inline v4
vecm4(m4 p, v4 q){
	v4 out;
	out.v4p = _mm256_set_pd(dotv4(p.row[3], q), dotv4(p.row[2], q), dotv4(p.row[1], q), dotv4(p.row[0], q));
	return out;
}

/* XXX: there is certainly a quicker way */
inline m4
trpm4(m4 p){
	m4 q = mkm4(mkv4(p.row[0].x, p.row[1].x, p.row[2].x, p.row[3].x),
				mkv4(p.row[0].y, p.row[1].y, p.row[2].y, p.row[3].y),
				mkv4(p.row[0].z, p.row[1].z, p.row[2].z, p.row[3].z),
				mkv4(p.row[0].t, p.row[1].t, p.row[2].t, p.row[3].t));
	return q;
}

/* see http://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
 * slightly modified but there might be room for improvement. */
inline m4
invm4(m4 p){
	m4 inv; double det;

	inv.d[0] = p.d[5]  * p.d[10] * p.d[15] - 
	p.d[5]  * p.d[11] * p.d[14] - 
	p.d[9]  * p.d[6]  * p.d[15] + 
	p.d[9]  * p.d[7]  * p.d[14] +
	p.d[13] * p.d[6]  * p.d[11] - 
	p.d[13] * p.d[7]  * p.d[10];

	inv.d[4] = -p.d[4]  * p.d[10] * p.d[15] + 
	p.d[4]  * p.d[11] * p.d[14] + 
	p.d[8]  * p.d[6]  * p.d[15] - 
	p.d[8]  * p.d[7]  * p.d[14] - 
	p.d[12] * p.d[6]  * p.d[11] + 
	p.d[12] * p.d[7]  * p.d[10];

	inv.d[8] = p.d[4]  * p.d[9] * p.d[15] - 
	p.d[4]  * p.d[11] * p.d[13] - 
	p.d[8]  * p.d[5] * p.d[15] + 
	p.d[8]  * p.d[7] * p.d[13] + 
	p.d[12] * p.d[5] * p.d[11] - 
	p.d[12] * p.d[7] * p.d[9];

	inv.d[12] = -p.d[4]  * p.d[9] * p.d[14] + 
	p.d[4]  * p.d[10] * p.d[13] +
	p.d[8]  * p.d[5] * p.d[14] - 
	p.d[8]  * p.d[6] * p.d[13] - 
	p.d[12] * p.d[5] * p.d[10] + 
	p.d[12] * p.d[6] * p.d[9];

	inv.d[1] = -p.d[1]  * p.d[10] * p.d[15] + 
	p.d[1]  * p.d[11] * p.d[14] + 
	p.d[9]  * p.d[2] * p.d[15] - 
	p.d[9]  * p.d[3] * p.d[14] - 
	p.d[13] * p.d[2] * p.d[11] + 
	p.d[13] * p.d[3] * p.d[10];

	inv.d[5] = p.d[0]  * p.d[10] * p.d[15] - 
	p.d[0]  * p.d[11] * p.d[14] - 
	p.d[8]  * p.d[2] * p.d[15] + 
	p.d[8]  * p.d[3] * p.d[14] + 
	p.d[12] * p.d[2] * p.d[11] - 
	p.d[12] * p.d[3] * p.d[10];

	inv.d[9] = -p.d[0]  * p.d[9] * p.d[15] + 
	p.d[0]  * p.d[11] * p.d[13] + 
	p.d[8]  * p.d[1] * p.d[15] - 
	p.d[8]  * p.d[3] * p.d[13] - 
	p.d[12] * p.d[1] * p.d[11] + 
	p.d[12] * p.d[3] * p.d[9];

	inv.d[13] = p.d[0]  * p.d[9] * p.d[14] - 
	p.d[0]  * p.d[10] * p.d[13] - 
	p.d[8]  * p.d[1] * p.d[14] + 
	p.d[8]  * p.d[2] * p.d[13] + 
	p.d[12] * p.d[1] * p.d[10] - 
	p.d[12] * p.d[2] * p.d[9];

	inv.d[2] = p.d[1]  * p.d[6] * p.d[15] - 
	p.d[1]  * p.d[7] * p.d[14] - 
	p.d[5]  * p.d[2] * p.d[15] + 
	p.d[5]  * p.d[3] * p.d[14] + 
	p.d[13] * p.d[2] * p.d[7] - 
	p.d[13] * p.d[3] * p.d[6];

	inv.d[6] = -p.d[0]  * p.d[6] * p.d[15] + 
	p.d[0]  * p.d[7] * p.d[14] + 
	p.d[4]  * p.d[2] * p.d[15] - 
	p.d[4]  * p.d[3] * p.d[14] - 
	p.d[12] * p.d[2] * p.d[7] + 
	p.d[12] * p.d[3] * p.d[6];

	inv.d[10] = p.d[0]  * p.d[5] * p.d[15] - 
	p.d[0]  * p.d[7] * p.d[13] - 
	p.d[4]  * p.d[1] * p.d[15] + 
	p.d[4]  * p.d[3] * p.d[13] + 
	p.d[12] * p.d[1] * p.d[7] - 
	p.d[12] * p.d[3] * p.d[5];

	inv.d[14] = -p.d[0]  * p.d[5] * p.d[14] + 
	p.d[0]  * p.d[6] * p.d[13] + 
	p.d[4]  * p.d[1] * p.d[14] - 
	p.d[4]  * p.d[2] * p.d[13] - 
	p.d[12] * p.d[1] * p.d[6] + 
	p.d[12] * p.d[2] * p.d[5];

	inv.d[3] = -p.d[1] * p.d[6] * p.d[11] + 
	p.d[1] * p.d[7] * p.d[10] + 
	p.d[5] * p.d[2] * p.d[11] - 
	p.d[5] * p.d[3] * p.d[10] - 
	p.d[9] * p.d[2] * p.d[7] + 
	p.d[9] * p.d[3] * p.d[6];

	inv.d[7] = p.d[0] * p.d[6] * p.d[11] - 
	p.d[0] * p.d[7] * p.d[10] - 
	p.d[4] * p.d[2] * p.d[11] + 
	p.d[4] * p.d[3] * p.d[10] + 
	p.d[8] * p.d[2] * p.d[7] - 
	p.d[8] * p.d[3] * p.d[6];

	inv.d[11] = -p.d[0] * p.d[5] * p.d[11] + 
	p.d[0] * p.d[7] * p.d[9] + 
	p.d[4] * p.d[1] * p.d[11] - 
	p.d[4] * p.d[3] * p.d[9] - 
	p.d[8] * p.d[1] * p.d[7] + 
	p.d[8] * p.d[3] * p.d[5];

	inv.d[15] = p.d[0] * p.d[5] * p.d[10] - 
	p.d[0] * p.d[6] * p.d[9] - 
	p.d[4] * p.d[1] * p.d[10] + 
	p.d[4] * p.d[2] * p.d[9] + 
	p.d[8] * p.d[1] * p.d[6] - 
	p.d[8] * p.d[2] * p.d[5];

	det = p.d[0] * inv.d[0] + p.d[1] * inv.d[4] + p.d[2] * inv.d[8] + p.d[3] * inv.d[12];
	det = 1.0 / det;

	__m256d detv = _mm256_set1_pd(det);
	inv.row[0].v4p = _mm256_mul_pd(inv.row[0].v4p, detv);
	inv.row[1].v4p = _mm256_mul_pd(inv.row[1].v4p, detv);
	inv.row[2].v4p = _mm256_mul_pd(inv.row[2].v4p, detv);
	inv.row[3].v4p = _mm256_mul_pd(inv.row[3].v4p, detv);

	return inv;
}

#endif
