#pragma once

#include <d3d11.h>
#include <d3dx11.h>
#include <D3DX10.h>		//Needed as directx11 is an extension of 10 and uses it's macros, funcitons and classes
#include <dinput.h>
#include <DirectXMath.h>

#include <sstream>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 720

using namespace DirectX;

class Application 
{
	Application() {};
	~Application() {};



};
