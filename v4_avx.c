#include "v4_avx.h"

extern inline v4
mkv4(double x, double y, double z, double t);

extern inline v4
addv4(v4 p, v4 q);

extern inline v4
subv4(v4 p, v4 q);

extern inline v4
elpv4(v4 p, v4 q);

extern inline v4
scv4(double s, v4 p);

extern inline double
dotv4(v4 p, v4 q);

extern inline v4
crv4(v4 p, v4 q);

extern inline v4
norv4(v4 p);

extern inline double
lenv4(v4 p);

extern inline m4
mkm4(v4 x, v4 y, v4 z, v4 t);

extern inline m4
addm4(m4 p, m4 q);

extern inline m4
subm4(m4 p, m4 q);

extern inline m4
elpm4(m4 p, m4 q);

extern inline m4
scm4(double s, m4 p);

extern inline m4
mulm4(m4 p, m4 q);

extern inline v4
vecm4(m4 p, v4 q);

extern inline m4
trpm4(m4 p);

extern inline m4
invm4(m4 p);
