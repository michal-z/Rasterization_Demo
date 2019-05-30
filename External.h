#pragma once

#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <dxgi1_4.h>
#include <d3d12.h>

#include <DirectXMath.h>
#include "d3dx12.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define VHR(hr) if (FAILED(hr)) { assert(0); }
#define SAFE_RELEASE(obj) if ((obj)) { (obj)->Release(); (obj) = nullptr; }
#define float2 XMFLOAT2
#define float3 XMFLOAT3
#define float4 XMFLOAT4

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;
typedef size_t usize;
typedef ptrdiff_t isize;

using namespace DirectX;
