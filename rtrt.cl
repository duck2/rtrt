typedef struct {
	float4 o;
	float4 d;
	float4 color;
} Ray;

__kernel void
genrays(__global Ray *rays, float4 o, float4 up, float4 gaze, float4 right, float d){
	int gidx = get_global_id(0), gidy = get_global_id(1);
	int width = get_global_size(0), height = get_global_size(1);
	rays[gidx + width*gidy].o = o;
	rays[gidx + width*gidy].d = d*gaze + 2*right*(gidx / width) - 2*up*(gidy / height);
	rays[gidx + width*gidy].color = (float4)(1, 0, 0, 0);
}

__kernel void
red(__global Ray *rays, __write_only image2d_t fb){
	int x = get_global_id(0), width = get_global_size(0);
	int y = get_global_id(1);
	int2 coords = (int2)(x, y);
	float4 color = rays[x + width*y].color;
	write_imagef(fb, coords, color);
}
