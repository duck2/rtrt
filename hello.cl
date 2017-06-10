__kernel void
red(__write_only image2d_t fb){
	int x = get_global_id(0);
	int y = get_global_id(1);
	int2 coords = (int2)(x, y);
	float4 red = (float4)(1.0, 0.0, 0.0, 1.0);
	write_imagef(fb, coords, red);
}
