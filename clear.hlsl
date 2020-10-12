/*
 *
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

Texture2D<float4> layer_tex[] : register(t0);
Texture2D<float4> user_tex[] : register(t1);
SamplerState samplers[]   : register(s0);

struct VSInput {
	float4 pos : POSITION;
	float4 uv : TEXCOORD;
	float4 color : COLOR;
	uint4 id : MATID;
};

struct PSInput {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

PSInput VSMain(VSInput vsin)
{
	PSInput result = (PSInput)0;
	result.pos = float4(vsin.pos.xyz, 1.0);
	result.uv = vsin.uv.xy;
	return result;
}

void PSMain(PSInput input, out float4 mrt0 : SV_TARGET)
{
	float4 bgcol = float4(0.0, 0.0, 0.0, 0.0);
	mrt0 = float4(0.0, 0.0, 0.0, 0.0);;
}
