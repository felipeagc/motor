#pragma motor compute_entry main

struct Output
{
	uint id;
};

[[vk::binding(0, 0)]] RWStructuredBuffer<Output> buffer_out;
[[vk::binding(1, 0)]] cbuffer input
{
	int2 coords;
};
[[vk::binding(2, 0)]] Texture2D<uint> bitmap;

[numthreads(1, 1, 1)]
void main(void)
{
	buffer_out[0].id = bitmap.Load(int3(coords, 0));
}
