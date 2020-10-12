#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <algorithm>

#include <windows.h>

#include <dwmapi.h>
#include <D3Dcompiler.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "D3DCompiler.lib")

#define err(...) printf("[ERR] : " __FUNCTION__ " : " __VA_ARGS__)
#define dbg(...) printf("[DBG] : " __FUNCTION__ " : " __VA_ARGS__)

static LRESULT WINAPI
win_msg_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_SYSCOMMAND:
		switch ((wParam & 0xFFF0)) {
		case SC_MONITORPOWER:
		case SC_SCREENSAVE:
			return 0;
		default:
			break;
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_IME_SETCONTEXT:
		lParam &= ~ISC_SHOWUIALL;
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}


HWND
win_create(const char *name, int w, int h)
{
	auto instance = GetModuleHandle(NULL);
	auto style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
	auto ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	RECT rc = {0, 0, w, h};
	WNDCLASSEX twc = {
		sizeof(WNDCLASSEX), CS_CLASSDC, win_msg_proc, 0L, 0L, instance,
		LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)GetStockObject(BLACK_BRUSH), NULL, name, NULL
	};

	RegisterClassEx(&twc);
	AdjustWindowRectEx(&rc, style, FALSE, ex_style);
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	auto hwnd = CreateWindowEx(
			ex_style, name, name, style,
			(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
			rc.right, rc.bottom, NULL, NULL, instance, NULL);
	ShowWindow(hwnd, SW_SHOW);
	SetFocus(hwnd);

	return (hwnd);
};

int
win_update()
{
	MSG msg;
	int is_active = 1;

	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			is_active = 0;
			break;
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return is_active;
}

ID3D12DescriptorHeap *
create_heap(ID3D12Device *dev, D3D12_DESCRIPTOR_HEAP_TYPE type,
	D3D12_DESCRIPTOR_HEAP_FLAGS flags, UINT num)
{
	ID3D12DescriptorHeap *ret = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};

	desc.NumDescriptors = num;
	desc.Type = type;
	desc.Flags = flags;
	dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&ret));
	return (ret);
}

IDXGISwapChain3 *
create_swap_chain(ID3D12CommandQueue *queue, HWND hwnd, int w, int h,
	int framecount)
{
	IDXGISwapChain3 *ret = nullptr;
	DXGI_SWAP_CHAIN_DESC desc = {};
	IDXGIFactory4 *factory = nullptr;
	IDXGISwapChain *temp = nullptr;

	desc.BufferCount = framecount;
	desc.BufferDesc.Width = w;
	desc.BufferDesc.Height = h;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	desc.Windowed = TRUE;
	desc.OutputWindow = hwnd;

	CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	factory->CreateSwapChain(queue, &desc, &temp);
	temp->QueryInterface(IID_PPV_ARGS(&ret));
	temp->Release();
	factory->Release();

	return (ret);
}

ID3D12CommandQueue *
create_queue(ID3D12Device *dev)
{
	ID3D12CommandQueue *ret = nullptr;
	D3D12_COMMAND_QUEUE_DESC desc = {};

	dev->CreateCommandQueue(&desc, IID_PPV_ARGS(&ret));
	return (ret);
}

ID3D12Device *
create_device()
{
	ID3D12Device *ret = nullptr;
#ifdef _DEBUG_
	{
		ID3D12Debug* dctrl;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dctrl)))) {
			dctrl->EnableDebugLayer();
			dctrl->Release();
		}
	}
#endif //_DEBUG_
	D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&ret));
	return (ret);
}

ID3D12Resource *
create_res(ID3D12Device *dev, int w, int h, DXGI_FORMAT fmt,
	D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE htype,
	D3D12_RESOURCE_DIMENSION dim, D3D12_TEXTURE_LAYOUT layout)
{
	ID3D12Resource *ret = nullptr;
	D3D12_RESOURCE_DESC desc = {};
	D3D12_HEAP_PROPERTIES hprop  = {};

	hprop.Type = htype;
	hprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hprop.CreationNodeMask = 1;
	hprop.VisibleNodeMask = 1;

	desc.Dimension = dim;
	desc.Width = w;
	desc.Height = h;
	desc.Format = fmt;
	desc.Layout = layout;
	desc.Flags = flags;
	desc.Alignment = 0;
	desc.DepthOrArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.MipLevels = 1;
	auto state = D3D12_RESOURCE_STATE_COMMON;
	if (htype == D3D12_HEAP_TYPE_UPLOAD)
		state = D3D12_RESOURCE_STATE_GENERIC_READ;

	auto hr = dev->CreateCommittedResource(&hprop, D3D12_HEAP_FLAG_NONE,
			&desc, state, nullptr,
			IID_PPV_ARGS(&ret));
	if (hr) {
		err("w=%d, h=%d, flags=%08X, hr=%08X\n",
			w, h, flags, hr);
		err("GetDeviceRemovedReason=%08X\n",
			dev->GetDeviceRemovedReason());
		return nullptr;
	}

	return (ret);
}


ID3D12Resource *
create_res_render_target(ID3D12Device *dev, UINT w, UINT h, DXGI_FORMAT fmt)
{
	return create_res(dev, w, h, fmt,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			D3D12_TEXTURE_LAYOUT_UNKNOWN);
}

ID3D12Resource *
create_res_buffer(ID3D12Device *dev, UINT bytes)
{
	return create_res(dev, bytes, 1, DXGI_FORMAT_UNKNOWN,
			D3D12_RESOURCE_FLAG_NONE,
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_DIMENSION_BUFFER,
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR);
}

ID3D12Resource *
create_res_uav_buffer(ID3D12Device *dev, UINT bytes)
{
	return create_res(dev, bytes, 1, DXGI_FORMAT_UNKNOWN,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_DIMENSION_BUFFER,
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR);
}

void *
get_data_address(ID3D12Resource *res)
{
	UINT8 *ret = nullptr;

	res->Map(0, NULL, reinterpret_cast<void **>(&ret));
	if (!ret) {
		printf("Cant Map res=%p\n", res);
		exit(1);
	}
	return ret;
}

void
upload_data(ID3D12Resource *res, void *data, size_t size)
{
	UINT8 *dest = nullptr;
	res->Map(0, NULL, reinterpret_cast<void **>(&dest));
	if (dest) {
		memcpy(dest, data, size);
		res->Unmap(0, NULL);
	} else {
		printf("Cant Upload res=%p\n", res);
		exit(1);
	}
}

int
create_rtv(ID3D12Device *dev, ID3D12Resource *res,
	D3D12_CPU_DESCRIPTOR_HANDLE hcpu_rtv)
{
	D3D12_RENDER_TARGET_VIEW_DESC desc_rtv = {};
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();

	desc_rtv.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	desc_rtv.Format = desc_res.Format;
	dev->CreateRenderTargetView(res, &desc_rtv, hcpu_rtv);
	return (0);
}

int
create_srv(ID3D12Device *dev, ID3D12Resource *res,
	D3D12_CPU_DESCRIPTOR_HANDLE hcpu_srv)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc_srv = {};
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();

	desc_srv.Format = desc_res.Format;
	desc_srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc_srv.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc_srv.Texture2D.MipLevels = 1;
	dev->CreateShaderResourceView(res, &desc_srv, hcpu_srv);
	return (0);
}

int
create_dsv(ID3D12Device *dev, ID3D12Resource *res,
	D3D12_CPU_DESCRIPTOR_HANDLE hcpu_dsv)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC desc_dsv = {};
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();

	desc_dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	desc_dsv.Format = desc_res.Format;
	dev->CreateDepthStencilView(res, &desc_dsv, hcpu_dsv);
	return (0);
}

int
create_cbv(ID3D12Device *dev, ID3D12Resource *res,
	D3D12_CPU_DESCRIPTOR_HANDLE hcpu_cbv)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc_cbv = {};
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();

	desc_cbv.SizeInBytes = desc_res.Width;
	desc_cbv.SizeInBytes = (desc_cbv.SizeInBytes + 255) & ~255;
	desc_cbv.BufferLocation = res->GetGPUVirtualAddress();
	dev->CreateConstantBufferView(&desc_cbv, hcpu_cbv);
	return (0);
}

int
create_uav(ID3D12Device *dev, ID3D12Resource *res,
	UINT num_elem, UINT byte_stride,
	D3D12_CPU_DESCRIPTOR_HANDLE hcpu_srv)
{
	D3D12_RESOURCE_DESC desc_res = res->GetDesc();
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc_uav = {};

	desc_uav.Format = desc_res.Format;
	desc_uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	desc_uav.Buffer.FirstElement = 0;
	desc_uav.Buffer.NumElements = num_elem;
	desc_uav.Buffer.StructureByteStride = byte_stride;
	desc_uav.Buffer.CounterOffsetInBytes = 0;
	dev->CreateUnorderedAccessView(res, nullptr, &desc_uav, hcpu_srv);
	return (0);
}

int
create_sampler(ID3D12Device *dev, D3D12_FILTER filter,
	D3D12_CPU_DESCRIPTOR_HANDLE hcpu)
{
	D3D12_SAMPLER_DESC desc = {};

	desc.Filter = filter;
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.MinLOD = 0;
	desc.MaxLOD = D3D12_FLOAT32_MAX;
	desc.MipLODBias = 0.0f;
	desc.MaxAnisotropy = 1;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	dev->CreateSampler(&desc, hcpu);
	return (0);
}


D3D12_DESCRIPTOR_RANGE
create_desc_range(D3D12_DESCRIPTOR_RANGE_TYPE type,
	UINT num_desc, UINT start = 0)
{
	D3D12_DESCRIPTOR_RANGE ret = {};

	ret.NumDescriptors = num_desc;
	ret.RangeType = type;
	ret.BaseShaderRegister = start;
	ret.OffsetInDescriptorsFromTableStart = 0;
	return (ret);
}

D3D12_ROOT_PARAMETER
create_root_param(D3D12_DESCRIPTOR_RANGE *ranges, UINT num)
{
	D3D12_ROOT_PARAMETER ret = {};

	ret.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ret.DescriptorTable.pDescriptorRanges = ranges;
	ret.DescriptorTable.NumDescriptorRanges = num;
	ret.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	return (ret);
}

ID3D12RootSignature *
create_root_gsig(ID3D12Device *dev, const UINT num = 256)
{
	ID3D12RootSignature *ret = nullptr;
	D3D12_ROOT_SIGNATURE_DESC desc = {};

	ID3DBlob *pblob = nullptr;
	ID3DBlob *perrblob = nullptr;
	std::vector<D3D12_ROOT_PARAMETER> rparams;
	std::vector<D3D12_DESCRIPTOR_RANGE> dranges;

	dranges.push_back(
		create_desc_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, num, 0));
	dranges.push_back(
		create_desc_range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, num, num));
	dranges.push_back(
		create_desc_range(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, num, 0));
	dranges.push_back(
	    create_desc_range(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, num));
	for (int i = 0 ; i < dranges.size(); i++)
		rparams.push_back(create_root_param(&dranges[i], 1));

	desc.pParameters = rparams.data();
	desc.NumParameters = rparams.size();
	desc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	auto hr = D3D12SerializeRootSignature(
		&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pblob, &perrblob);
	if (hr || perrblob) {
		err("D3D12SerializeRootSignature:\n%s\n",
			(char *)perrblob->GetBufferPointer());
		return nullptr;
	}

	hr = dev->CreateRootSignature(
		0, pblob->GetBufferPointer(), pblob->GetBufferSize(),
		IID_PPV_ARGS(&ret));
	if (perrblob)
		perrblob->Release();
	if (pblob)
		pblob->Release();

	if (hr) {
		err("Failed hr=%08X\n", hr);
		return nullptr;
	}
	return (ret);
}

ID3D12RootSignature *
create_root_csig(ID3D12Device *dev, const UINT num = 256)
{
	ID3D12RootSignature *ret = nullptr;
	D3D12_ROOT_SIGNATURE_DESC desc = {};

	ID3DBlob *pblob = nullptr;
	ID3DBlob *perrblob = nullptr;
	std::vector<D3D12_ROOT_PARAMETER> rparams;
	std::vector<D3D12_DESCRIPTOR_RANGE> dranges;
	

	dranges.push_back(
		create_desc_range(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, num, 0));
	dranges.push_back(
		create_desc_range(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, num, num));
	for (int i = 0 ; i < dranges.size(); i++)
		rparams.push_back(create_root_param(&dranges[i], 1));

	desc.pParameters = rparams.data();
	desc.NumParameters = rparams.size();

	auto hr = D3D12SerializeRootSignature(
		&desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pblob, &perrblob);
	if (hr || perrblob) {
		err("D3D12SerializeRootSignature:\n%s\n",
			(char *)perrblob->GetBufferPointer());
		return nullptr;
	}

	hr = dev->CreateRootSignature(
		0, pblob->GetBufferPointer(), pblob->GetBufferSize(),
		IID_PPV_ARGS(&ret));
	if (perrblob)
		perrblob->Release();
	if (pblob)
		pblob->Release();

	if (hr) {
		err("Failed hr=%08X\n", hr);
		return nullptr;
	}
	return (ret);
}

static D3D12_SHADER_BYTECODE
gen_shader_from_file(std::string fstr, std::string entry,
	std::string profile, std::vector<uint8_t> &shader_code)
{
	ID3DBlob *blob = nullptr;
	ID3DBlob *blob_err = nullptr;
	ID3DBlob *blob_sig = nullptr;

	std::vector<WCHAR> wfname;
	UINT flags = D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
	for (int i = 0; i < fstr.length(); i++)
		wfname.push_back(fstr[i]);
	wfname.push_back(0);

	D3DCompileFromFile(&wfname[0], NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry.c_str(), profile.c_str(), flags, 0, &blob, &blob_err);
	if (blob_err) {
		err("%s\n", (char *)blob_err->GetBufferPointer());
		blob_err->Release();
	}
	if (!blob && !blob_err)
		err("File Not found : %s\n", fstr.c_str());
	if (!blob)
		return {nullptr, 0};
	shader_code.resize(blob->GetBufferSize());
	memcpy(shader_code.data(),
		blob->GetBufferPointer(),
		blob->GetBufferSize());
	if (blob)
		blob->Release();
	return { shader_code.data(), shader_code.size() };
}

ID3D12PipelineState *
create_gpstate_from_file(ID3D12Device *dev, ID3D12RootSignature *root_sig,
	DXGI_FORMAT fmt_color, DXGI_FORMAT fmt_depth, std::string filename)
{
	ID3D12PipelineState *pstate = nullptr;
	std::vector<uint8_t> vs;
	std::vector<uint8_t> gs;
	std::vector<uint8_t> ps;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	D3D12_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "MATID", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	desc.InputLayout.pInputElementDescs = layout;
	desc.InputLayout.NumElements = _countof(layout);
	desc.DepthStencilState.DepthEnable = FALSE;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	auto & rtref = desc.BlendState.RenderTarget;
	for (auto & bs : rtref) {
		bs.BlendEnable = FALSE;
		bs.LogicOpEnable = FALSE;
		bs.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		bs.DestBlend = D3D12_BLEND_INV_DEST_ALPHA;
		bs.BlendOp = D3D12_BLEND_OP_ADD;
		bs.SrcBlendAlpha = D3D12_BLEND_ONE;
		bs.DestBlendAlpha = D3D12_BLEND_ZERO;
		bs.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		bs.LogicOp = D3D12_LOGIC_OP_XOR;
		bs.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}

	auto fname = filename + ".hlsl";

	desc.pRootSignature = root_sig;
	desc.NumRenderTargets = _countof(rtref);
	desc.VS = gen_shader_from_file(fname, "VSMain", "vs_5_1", vs);
	desc.PS = gen_shader_from_file(fname, "PSMain", "ps_5_1", ps);
	desc.SampleDesc.Count = 1;
	desc.SampleMask = UINT_MAX;
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.RasterizerState.DepthClipEnable = TRUE;
	desc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	for (auto & fmt : desc.RTVFormats)
		fmt = fmt_color;
	desc.DSVFormat = fmt_depth;

	if (!vs.empty() && !ps.empty()) {
		auto status = dev->CreateGraphicsPipelineState(
			&desc, IID_PPV_ARGS(&pstate));
		if (!pstate) {
			printf("CreateGraphicsPipelineState:%s:0x%08X\n",
				filename.c_str(), status);
			exit(1);
			return nullptr;
		}
	}
	return (pstate);
}


ID3D12PipelineState *
create_cpstate_from_file(
	ID3D12Device *dev, ID3D12RootSignature *root_sig, std::string filename)
{
	ID3D12PipelineState *pstate = nullptr;
	D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
	std::vector<uint8_t> cs;
	auto fname = filename + ".hlsl";

	desc.pRootSignature = root_sig;
	desc.CS = gen_shader_from_file(fname, "CSMain", "cs_5_1", cs);
	if (cs.empty())
		return nullptr;

	auto status = dev->CreateComputePipelineState(
		&desc, IID_PPV_ARGS(&pstate));
	if (!pstate) {
		printf("CreateComputePipelineState:%s:0x%08X\n",
			filename.c_str(), status);
		return nullptr;
	}
	return (pstate);
}

D3D12_RESOURCE_BARRIER
get_barrier(ID3D12Resource *res, D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER ret = {};

	ret.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	ret.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	ret.Transition.pResource = res;
	ret.Transition.StateBefore = before;
	ret.Transition.StateAfter = after;
	ret.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	return ret;
}

struct VertexFormat {
	float pos[4];
	float uv[4];
	float color[4];
	uint32_t matid;
	uint32_t reserved[3];
};

struct ObjectFormat {
	float pos[4];
	float scale[4];
	float rotate[4];
	float color[4];
	float uvinfo[4];
	uint32_t metadata[4];
};

struct Handles {
	D3D12_CPU_DESCRIPTOR_HANDLE cpu;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu;
};

struct frame_info_t {
	ID3D12CommandAllocator *cmd_alloc = nullptr;
	ID3D12GraphicsCommandList *cmd_list = nullptr;
	ID3D12Fence *fence = nullptr;
	std::vector<Handles> vhandles_rtv;
	std::vector<Handles> vhandles_srv;

	ID3D12Resource *image = nullptr;
	ID3D12Resource *res_vertex_buffer_rect = nullptr;

	struct layer_t {
		std::vector<Handles> vhandles_uav;
		std::vector<Handles> vhandles_cbv;
		ID3D12Resource *image = nullptr;
		ID3D12Resource *res_object_data = nullptr;
		ID3D12Resource *res_object_vertex = nullptr;

		ID3D12Resource *res_object_update_buffer_uav = nullptr;
		ID3D12Resource *res_object_buffer = nullptr;
		ObjectFormat *object_buffer = nullptr;
	};
	std::vector<layer_t> layers;

	uint64_t fence_value = 0;
	void init(ID3D12Device *dev, IDXGISwapChain3 *swapchain, int index)
	{
		swapchain->GetBuffer(index, IID_PPV_ARGS(&image));
		dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
		dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmd_alloc));
		dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmd_alloc, nullptr, IID_PPV_ARGS(&cmd_list));
		dbg("cmd_alloc=%p\n", cmd_alloc);
		dbg("cmd_list=%p\n", cmd_list);
		dbg("fence=%p\n", fence);
		dbg("image=%p\n", image);
	}
};

Handles get_descriptor_handles(ID3D12Device *device,
	ID3D12DescriptorHeap *heap, UINT index)
{
	Handles ret = {};
	auto desc = heap->GetDesc();
	auto size = device->GetDescriptorHandleIncrementSize(desc.Type);
	ret.cpu = heap->GetCPUDescriptorHandleForHeapStart();
	ret.gpu = heap->GetGPUDescriptorHandleForHeapStart();
	ret.cpu.ptr += size * index;
	ret.gpu.ptr += size * index;
	return ret;
}

int main()
{

	enum {
		ScreenWidth = 1024,
		ScreenHeight = 1024,
		Width = 512,
		Height = 512,

		FrameCount = 2,
		LayerMax = 8,
		ObjectMax = 4096,
		MaxDescNum = 32,
		MaxDescSrvNum = 256,
		MaxDescSampler = 32,
		ComputeUpdateGroupSize = 256,
	};

	auto hwnd = win_create("test", ScreenWidth, ScreenHeight);
	auto dev = create_device();

	auto queue = create_queue(dev);
	auto swapchain = create_swap_chain(queue, hwnd, ScreenWidth, ScreenHeight, FrameCount);
	auto root_gsig = create_root_gsig(dev);
	auto root_csig = create_root_csig(dev);
	auto heap_rtv = create_heap(dev, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 256);
	auto heap_dsv = create_heap(dev, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 256);
	auto heap_srv = create_heap(dev, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 256);
	auto heap_sampler = create_heap(dev, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 256);

	UINT index_heap_rtv = 0;
	UINT index_heap_dsv = 0;
	UINT index_heap_srv = 0;
	UINT index_heap_cbv = 0;
	UINT index_heap_sampler = 0;
	auto pstate_update = create_cpstate_from_file(dev, root_csig, "update");
	auto pstate_clear = create_gpstate_from_file(dev, root_gsig, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32_FLOAT, "clear");
	auto pstate_draw_rects = create_gpstate_from_file(dev, root_gsig, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32_FLOAT, "draw_rects");
	auto pstate_present = create_gpstate_from_file(dev, root_gsig, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32_FLOAT, "present");

	VertexFormat vertex_rect[6] = {
		{ {-1, -1, 0, 1}, {0, 0} },
		{ {-1,  1, 0, 1}, {0, 1} },
		{ { 1, -1, 0, 1}, {1, 0} },

		{ { 1,  1, 0, 1}, {1, 1} },
		{ {-1,  1, 0, 1}, {0, 1} },
		{ { 1, -1, 0, 1}, {1, 0} },
	};
	float clear_color[LayerMax][4] = {
		{1, 0, 0, 1},
		{0, 0, 1, 1},
		{0, 1, 1, 1},
		{1, 0, 1, 1},
		{1, 1, 0, 1},
		{0, 1, 1, 1},
		{0, 0, 1, 1},
		{1, 0, 0, 1},
	};
	frame_info_t framedata[FrameCount];

	std::vector<Handles> vhandles_sampler;
	auto hsampler_point = get_descriptor_handles(dev, heap_sampler, index_heap_sampler++);
	auto hsampler_linear = get_descriptor_handles(dev, heap_sampler, index_heap_sampler++);
	vhandles_sampler.push_back(hsampler_point);
	vhandles_sampler.push_back(hsampler_linear);
	create_sampler(dev, D3D12_FILTER_MIN_MAG_MIP_POINT, hsampler_point.cpu);
	create_sampler(dev, D3D12_FILTER_MIN_MAG_MIP_LINEAR, hsampler_linear.cpu);

	for (int i = 0 ; i < FrameCount; i++) {
		auto & ref = framedata[i];
		ref.init(dev, swapchain, i);
		ref.res_vertex_buffer_rect = create_res_buffer(dev, sizeof(vertex_rect));
		upload_data(ref.res_vertex_buffer_rect, vertex_rect, sizeof(vertex_rect));

		auto desc_backbuffer = ref.image->GetDesc();
		auto object_buffer_size = sizeof(ObjectFormat) * ObjectMax;
		auto object_buffer_vertex_size = object_buffer_size * 6;
		object_buffer_size = (object_buffer_size + 255) & ~255;
		object_buffer_vertex_size = (object_buffer_vertex_size + 255) & ~255;
		for (int i = 0 ; i < LayerMax; i++) {
			frame_info_t::layer_t layer;
			auto hrtv = get_descriptor_handles(dev, heap_rtv, index_heap_rtv++);
			auto hsrv = get_descriptor_handles(dev, heap_srv, index_heap_srv++);
			ref.vhandles_rtv.push_back(hrtv);
			ref.vhandles_srv.push_back(hsrv);
			layer.image = create_res_render_target(dev, Width, Height, desc_backbuffer.Format);
			create_rtv(dev, layer.image, hrtv.cpu);
			create_srv(dev, layer.image, hsrv.cpu);
			ref.layers.push_back(layer);
		}

		for (int i = 0 ; i < LayerMax; i++) {
			auto & layer = ref.layers[i];
			auto huav_src = get_descriptor_handles(dev, heap_srv, index_heap_srv++);
			auto huav_dst = get_descriptor_handles(dev, heap_srv, index_heap_srv++);
			layer.vhandles_uav.push_back(huav_src);
			layer.vhandles_uav.push_back(huav_dst);
			layer.res_object_update_buffer_uav = create_res_uav_buffer(dev, object_buffer_size);
			layer.res_object_buffer = create_res_buffer(dev, object_buffer_size);
			layer.res_object_vertex = create_res_uav_buffer(dev, object_buffer_vertex_size);
			layer.object_buffer = (ObjectFormat *)get_data_address(layer.res_object_buffer);

			create_uav(dev, layer.res_object_update_buffer_uav, ObjectMax, sizeof(ObjectFormat), huav_src.cpu);
			create_uav(dev, layer.res_object_vertex, ObjectMax, sizeof(VertexFormat) * 6, huav_dst.cpu);

			printf("res_object_buffer=%p, object_buffer=%p\n", layer.res_object_buffer, layer.object_buffer);
		}

		auto hbackbuffer = get_descriptor_handles(dev, heap_rtv, index_heap_rtv++);
		create_rtv(dev, ref.image, hbackbuffer.cpu);
		ref.vhandles_rtv.push_back(hbackbuffer);

		auto cmd_list = ref.cmd_list;
		auto hsrv = ref.vhandles_srv.data();
		auto hsampler = vhandles_sampler.data();

		std::vector<ID3D12DescriptorHeap *> heaplists = {
			heap_srv,
			heap_sampler,
		};
		D3D12_VERTEX_BUFFER_VIEW view = {
			ref.res_vertex_buffer_rect->GetGPUVirtualAddress(), sizeof(vertex_rect), sizeof(VertexFormat)
		};
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_backbuffer_handles = { hbackbuffer.cpu };

		printf("Create Command\n");
		cmd_list->Close();
		ref.cmd_alloc->Reset();
		cmd_list->Reset(ref.cmd_alloc, 0);
		cmd_list->SetDescriptorHeaps(heaplists.size(), heaplists.data());

		printf("Begin Layer Command\n");
		for (int i = 0 ; i < LayerMax; i++) {
			auto & layer = ref.layers[i];
			auto & h = ref.vhandles_rtv[i];
			auto huav_src = layer.vhandles_uav.data();
			auto huav_dst = huav_src + 1;
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtv_handles = { h.cpu };
			cmd_list->SetComputeRootSignature(root_csig);
			cmd_list->SetComputeRootDescriptorTable(0, huav_src->gpu);
			cmd_list->SetComputeRootDescriptorTable(1, huav_dst->gpu);
			{
				auto desc = layer.res_object_buffer->GetDesc();
				cmd_list->CopyBufferRegion(
					layer.res_object_update_buffer_uav, 0,
					layer.res_object_buffer, 0,
					desc.Width);
			}
			cmd_list->SetPipelineState(pstate_update);
			cmd_list->Dispatch(ObjectMax / ComputeUpdateGroupSize, 1, 1);

			auto barrier = get_barrier(layer.image, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			cmd_list->ResourceBarrier(1, &barrier);
			D3D12_VERTEX_BUFFER_VIEW view_sprite = {
				layer.res_object_vertex->GetGPUVirtualAddress(), sizeof(VertexFormat) * ObjectMax * 6, sizeof(VertexFormat)
			};
			cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd_list->OMSetRenderTargets(rtv_handles.size(), rtv_handles.data(), FALSE, nullptr);
			D3D12_VIEWPORT viewport = {0, 0, Width, Height, 0.0f, 1.0f };
			D3D12_RECT rect = { 0, 0, Width, Height };
			cmd_list->RSSetViewports(1, &viewport);
			cmd_list->RSSetScissorRects(1, &rect);
			cmd_list->SetGraphicsRootSignature(root_gsig);
			cmd_list->SetGraphicsRootDescriptorTable(0, hsrv->gpu);
			cmd_list->SetGraphicsRootDescriptorTable(3, hsampler->gpu);
			cmd_list->SetPipelineState(pstate_clear);
			cmd_list->IASetVertexBuffers(0, 1, &view);
			cmd_list->DrawInstanced(6, 1, 0, 0);

			cmd_list->SetPipelineState(pstate_draw_rects);
			cmd_list->IASetVertexBuffers(0, 1, &view_sprite);
			cmd_list->DrawInstanced(ObjectMax * 6, 1, 0, 0);
		}
		for (int i = 0 ; i < LayerMax; i++) {
			auto & layer = ref.layers[i];
			auto barrier = get_barrier(layer.image, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
			cmd_list->ResourceBarrier(1, &barrier);
		}
		printf("End Layer Command\n");
		auto barrier_present_begin = get_barrier(ref.image, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		auto barrier_present_end = get_barrier(ref.image, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
		cmd_list->ResourceBarrier(1, &barrier_present_begin);
		cmd_list->OMSetRenderTargets(rtv_backbuffer_handles.size(), rtv_backbuffer_handles.data(), FALSE, nullptr);
		cmd_list->ClearRenderTargetView(hbackbuffer.cpu, clear_color[i], 0, NULL);
		D3D12_VIEWPORT viewport = {0, 0, ScreenWidth, ScreenHeight, 0.0f, 1.0f };
		D3D12_RECT rect = { 0, 0, ScreenWidth, ScreenHeight };

		cmd_list->RSSetViewports(1, &viewport);
		cmd_list->RSSetScissorRects(1, &rect);
		cmd_list->SetPipelineState(pstate_present);
		cmd_list->IASetVertexBuffers(0, 1, &view);
		cmd_list->DrawInstanced(6, 1, 0, 0);
		cmd_list->ResourceBarrier(1, &barrier_present_end);
		cmd_list->Close();
	}
	dbg("heap_rtv=%p\n", heap_rtv);
	dbg("heap_dsv=%p\n", heap_dsv);
	dbg("heap_srv=%p\n", heap_srv);
	dbg("heap_sampler=%p\n", heap_sampler);
	dbg("dev=%p\n", dev);
	dbg("swapchain=%p\n", swapchain);
	dbg("root_gsig=%p\n", root_gsig);
	dbg("root_csig=%p\n", root_csig);
	dbg("pstate_clear=%p\n", pstate_clear);

	double a_time = 0.0;
	while (win_update()) {
		a_time += 1.0 / 16.0f;
		auto index = swapchain->GetCurrentBackBufferIndex();
		auto & ref = framedata[index];
		for(int lidx = 0; lidx < LayerMax ; lidx++) {
			auto & layer = ref.layers[lidx];
			srand(0);
			for (int i = 0 ; i < ObjectMax; i++) {
				auto frand = []() {
					return (float(rand()) / 32767.0f) * 2.0f - 1.0f;
				};
				layer.object_buffer[i].pos[0] = cos(frand() * (lidx + i + 1.0 + a_time * 0.03));
				layer.object_buffer[i].pos[1] = sin(frand() * (lidx + i + 1.0 + a_time * 0.04));
				layer.object_buffer[i].scale[0] = 0.01f  + frand() * 0.01;
				layer.object_buffer[i].scale[1] = 0.01f  + frand() * 0.01;

				layer.object_buffer[i].rotate[0] = frand();

				layer.object_buffer[i].color[0] = frand() * 0.5 + 0.5;
				layer.object_buffer[i].color[1] = frand() * 0.5 + 0.5;
				layer.object_buffer[i].color[2] = frand() * 0.5 + 0.5;
				layer.object_buffer[i].color[3] = frand() * 0.5 + 0.5;

				layer.object_buffer[i].metadata[0] = 1;
				layer.object_buffer[i].metadata[1] = i;
			}
		}
		queue->Signal(ref.fence, ref.fence_value);
		if (ref.fence->GetCompletedValue() < ref.fence_value) {
			auto hevent = CreateEvent(NULL, FALSE, FALSE, NULL);
			ref.fence->SetEventOnCompletion(ref.fence_value, hevent);
			WaitForSingleObject(hevent, INFINITE);
			CloseHandle(hevent);
		}
		ref.fence_value++;
		ID3D12CommandList *pplists[] = {
			ref.cmd_list,
		};
		queue->ExecuteCommandLists(1, pplists);
		swapchain->Present(1, 0);
	}
}
