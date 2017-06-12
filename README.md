# rtrt

an attempt to raytrace in real-time.

roadmap:

- [x] make roadmap

	- [x] get CL/GL interop working, render empty buffer with some fps
	- [x] flat Blinn-Phong, spheres/triangles, no interaction, no BVH, no recursion, hardcoded scene
	- [x] scene file
	- [x] interaction
		- [ ] mouse grab
	- [x] reflective Blinn-Phong
	- [ ] acceleration structure
	- [x] refractions: one reflective, one refractive pass over rays
	- [ ] refine until reasonable fps: >20 on hd4000, >60 on actual graphics cards.
	- [ ] make roadmap for path tracer rtpt

this is a term project for [this course][795].

[795]: https://catalog.metu.edu.tr/course.php?prog=571&course_code=5710795
