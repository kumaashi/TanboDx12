/*
 * Copyright (c) 2020 gyabo <gyaboyan@gmail.com>
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

struct ObjectFormat {
	float4 pos;
	float4 scale;
	float4 rotate;
	float4 color;
	float4 uvinfo;
	uint metadata[4];
};

struct VertexFormat {
	float4 pos;
	float4 uv;
	float4 color;
	uint matid;
	uint reserved[3];
};

RWStructuredBuffer<ObjectFormat> obj : register(u0);
RWStructuredBuffer<VertexFormat> vtx : register(u1);

float2 rotate(float2 p, float a) {
	float c = cos(a);
	float s = sin(a);
	return float2(
		p.x * c - p.y * s,
		p.x * s + p.y * c);
}

[numthreads(256, 1, 1)]
void CSMain(uint3 gl_GlobalInvocationID : SV_DispatchThreadID)
{
	uint tid = gl_GlobalInvocationID.x;
	uint valid = obj[tid].metadata[0];
	if(valid == 0)
		return;

	float4 pos = obj[tid].pos;
	float4 scale = obj[tid].scale;
	float4 color = obj[tid].color;
	float4 uvinfo = obj[tid].uvinfo;
	float rotvalue = obj[tid].rotate.x;
	uint matid = obj[tid].metadata[1];

	float2 basepos[4];
	float2 baseuv[4];

	float2 uv_div = uvinfo.zw;
	float2 uv_unit = 1.0 / uv_div;
	float2 uv_offset = uv_unit * uvinfo.xy;

	baseuv[0] = (float2( 0,  0) / uv_div) + uv_offset;
	baseuv[1] = (float2( 0,  1) / uv_div) + uv_offset;
	baseuv[2] = (float2( 1,  0) / uv_div) + uv_offset;
	baseuv[3] = (float2( 1,  1) / uv_div) + uv_offset;

	//scale
	basepos[0] = float2(-1, -1) * scale.xy;
	basepos[1] = float2(-1,  1) * scale.xy;
	basepos[2] = float2( 1, -1) * scale.xy;
	basepos[3] = float2( 1,  1) * scale.xy;
	
	//rotate
	basepos[0] = rotate(basepos[0], rotvalue);
	basepos[1] = rotate(basepos[1], rotvalue);
	basepos[2] = rotate(basepos[2], rotvalue);
	basepos[3] = rotate(basepos[3], rotvalue);

	//trans
	basepos[0] += pos.xy;
	basepos[1] += pos.xy;
	basepos[2] += pos.xy;
	basepos[3] += pos.xy;

	//result
	float2 aspect = float2(1.0, 1.0);
	vtx[tid * 6 + 0].pos = float4(basepos[0] * aspect, 0, 1);
	vtx[tid * 6 + 1].pos = float4(basepos[1] * aspect, 0, 1);
	vtx[tid * 6 + 2].pos = float4(basepos[2] * aspect, 0, 1);
	vtx[tid * 6 + 3].pos = float4(basepos[1] * aspect, 0, 1);
	vtx[tid * 6 + 4].pos = float4(basepos[3] * aspect, 0, 1);
	vtx[tid * 6 + 5].pos = float4(basepos[2] * aspect, 0, 1);

	//uv
	vtx[tid * 6 + 0].uv = float4(baseuv[0], 0, 1);
	vtx[tid * 6 + 1].uv = float4(baseuv[1], 0, 1);
	vtx[tid * 6 + 2].uv = float4(baseuv[2], 0, 1);
	vtx[tid * 6 + 3].uv = float4(baseuv[1], 0, 1);
	vtx[tid * 6 + 4].uv = float4(baseuv[3], 0, 1);
	vtx[tid * 6 + 5].uv = float4(baseuv[2], 0, 1);
	
	//color
	vtx[tid * 6 + 0].color = color;
	vtx[tid * 6 + 1].color = color;
	vtx[tid * 6 + 2].color = color;
	vtx[tid * 6 + 3].color = color;
	vtx[tid * 6 + 4].color = color;
	vtx[tid * 6 + 5].color = color;
	
	//matid
	vtx[tid * 6 + 0].matid = matid;
	vtx[tid * 6 + 1].matid = matid;
	vtx[tid * 6 + 2].matid = matid;
	vtx[tid * 6 + 3].matid = matid;
	vtx[tid * 6 + 4].matid = matid;
	vtx[tid * 6 + 5].matid = matid;
}
