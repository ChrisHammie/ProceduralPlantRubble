#include <windows.h>
#include <windowsx.h>
#include <d3dcompiler.h>
#include <d3d11.h>
//#include <D3DX10.h>		//Needed as directx11 is an extension of 10 and uses it's macros, funcitons and classes
#include <dinput.h>
#include <DirectXMath.h>
#include <sstream>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")
//#pragma comment (lib, "d3dx11.lib")
//#pragma comment (lib, "d3dx10.lib")
#pragma comment (lib, "dinput8.lib")
#pragma comment (lib, "dxguid.lib")

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 720

using namespace DirectX;


IDXGISwapChain *swapchain;		//pointer to swap chain interface		The series of buffers which take turns being rendered on		Part of DXGI - Underlies Direct3D
ID3D11Device *dev;				//Pointer to Direct3D device interface
ID3D11DeviceContext *devcon;	//Pointer to Direct3D device context	Responsible for managing GPU and rendering pipeline. 
								//USed to render grpahics and determines how they will be rendered

ID3D11RenderTargetView *backbuffer;	//Pointer to an object that holds all the information about the render target

ID3D11VertexShader *pVS;	//Vertex Shader
ID3D11PixelShader *pPS;		//Pixel Shader

ID3D11Buffer *pVBuffer;		//Allows us to maintain a buffer in both system and video memory
ID3D11Buffer *indexBuffer;

ID3D11SamplerState* sampleState;
ID3D11ShaderResourceView *texture;


HWND hWnd;

//Input
IDirectInputDevice8* DIKeyboard;
IDirectInputDevice8* DIMouse;

DIMOUSESTATE mouseLastState;
LPDIRECTINPUT8 DirectInput;

float rotX = 0.0f;
float rotZ = 0.0f;
float scaleX = 1.0f;
float scaleY = 1.0f;

DirectX::XMMATRIX RotationX;
DirectX::XMMATRIX RotationZ;



ID3D11InputLayout *pLayout;	//Input layout object


							//Camera
ID3D11Buffer* cbPerObjectBuffer;

DirectX::XMMATRIX WVP;
DirectX::XMMATRIX World;
DirectX::XMMATRIX camView;
DirectX::XMMATRIX camProjection;

DirectX::XMMATRIX tri;

DirectX::XMVECTOR camPosition;
DirectX::XMVECTOR camTarget;
DirectX::XMVECTOR camUp;

DirectX::XMMATRIX Rotation;
DirectX::XMMATRIX Scale;
DirectX::XMMATRIX Translation;
float rot = 0.01f;

//First Person Camera
DirectX::XMVECTOR DefaultForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);	//Forward direction of the world
DirectX::XMVECTOR DefaultRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);		//Right direction of the world
																					//Used to calculate the rotation of the camera
DirectX::XMVECTOR camForward = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);		//Forward vector of the camera	Move along this vector to make it look as if the camera is moving
DirectX::XMVECTOR camRight = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);			//Right Vector of the camera

DirectX::XMMATRIX camRotationMatrix;												//USed to rotate the camera;
DirectX::XMMATRIX groundWorld;														//Describes world space of the ground

float moveLeftRight = 0.0f;		//Used to move along the camRight vector
float moveBackForward = 0.0f;	//USed to move along the camForward vector

float camYaw = 0.0f;	//Both used to calculate the rotation around the x and y axis
float camPitch = 0.0f;


struct cbPerObject
{
	DirectX::XMMATRIX World;
	DirectX::XMMATRIX WVP;
};

cbPerObject cbPerObj;


//Timer
double countsPerSecond = 0.0;
__int64 CounterStart = 0;

int frameCount = 0;
int fps = 0;

__int64 frameTimeOld = 0;
double frameTime;

void UpdateScene(double time);

void RenderText(std::wstring text, int inInt);

void StartTimer();
double GetTime();
double GetFrameTime();

void UpdateCamera();


void InitD3D(HWND hWnd);	//sets up and initializes Direct3D
void CleanD3D(void);		//Closes Direct3D and releases the memory

void RenderFrame(void);

void InitPipeline(void);

void InitGraphics(void);

void InitCamera(void);

bool InitDirectInput(HINSTANCE hInstance);
void DetectInput(double time);		//Detects if key is pressed or the mouse has moved


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct VERTEX
{
	FLOAT X, Y, Z;	//position
	float* color; //colour
};




int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{

	WNDCLASSEX wc;	//hold information for window class
					//EX is needed to show its the extended version of WNDCLASS				

	ZeroMemory(&wc, sizeof(WNDCLASSEX)); //Clear out window class for use. First param is where to start this block of memory, second param shows how long the block is

										 //fill in struct with information
	wc.cbSize = sizeof(WNDCLASSEX);		//Measurements
	wc.style = CS_HREDRAW | CS_VREDRAW;	//Redraw window if moved vertically or horizontally	
	wc.lpfnWndProc = WindowProc;		//Which function to use when it gets a message from Windows
	wc.hInstance = hInstance;			//Handle copy of application
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);	//What cursor to use
												//wc.hbrBackground = (HBRUSH)COLOR_WINDOW;	//Colour of background
	wc.lpszClassName = L"WindowClass1";			//Name of class

	RegisterClassEx(&wc);					//Registers window class, uses the address of the struct above


	RECT wr = { 0,0,500,400 };	//Sets size of wr not the position
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);	//Adjust size to consider the lcient area, takes a RECT, style, whether using menus or not

	hWnd = CreateWindowEx(NULL, L"WindowClass1",	//Class name
		L"First Windowed Program",	//Title		16-bit Unicode meaning the L is needed at front
		WS_OVERLAPPEDWINDOW,		//Style
		300,						//x pos
		300,						//y pos
		SCREEN_WIDTH,						//width will be the difference between the window left and right
		SCREEN_HEIGHT,						//height will be the difference between the window top and bottom
		NULL,						//no parent
		NULL,						//not using menus
		hInstance,					//application handle
		NULL);						//used with multiple windows

									/*HWND CreateWindowEx(DWORD dwExStyle,
									LPCTSTR lpClassName,
									LPCTSTR lpWindowName,
									DWORD dwStyle,		Allows you to style the window, remove buttons, non resizeable, scroll bars
									int x,		Pos
									int y,		Pos			This is basis for the above
									int nWidth,			Width of window
									int nHeight,		Height of window
									HWND hWndParent,	Tells what parent window created this window
									HMENU hMenu,		Handle to menubar
									HINSTANCE hInstance,
									LPVOID lpParam);	Param used if creating multiple windows*/

	ShowWindow(hWnd, nCmdShow); //nCmdShow determines whether this window is shown 


	InitD3D(hWnd);	//Set up and initialise Direct3D

	if (!InitDirectInput(hInstance))
	{
		MessageBox(0, L"Direct Input Intialisation fail", L"Error", MB_OK);
		return 0;
	}


	MSG msg; //Holds windows event messages

	while (TRUE)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))	//Check if any messages are waiting in queue									
		{												//Instead of waiting for a message with GetMessage(), PeekMessage() will return TRUE if there is a message
														//If there is a message, then translate and disptch the message
			TranslateMessage(&msg);	//Translate into right format
			DispatchMessage(&msg);	//Send mesage to WindowProc

			if (msg.message == WM_QUIT)
			{
				break; //If it's time to quit the program, quit
			}


		}
		else
		{

			frameCount++;
			if (GetTime() > 1.0f)
			{
				fps = frameCount;
				frameCount = 0;
				StartTimer();
			}
			frameTime = GetFrameTime();

			DetectInput(frameTime);

			UpdateScene(frameTime);
			RenderFrame();
		}
	}

	CleanD3D();

	return msg.wParam;
	/*BOOL GetMessage(LPMSG lpMsg,		Pointer to mesage struct
	HWND hWnd,		Handle to the window the message should come from, NULL means get the next message
	UINT wMsgFilterMin,		This and ..Max Limit the message type
	UINT wMsgFilterMax);*/
}

void UpdateScene(double time)
{

	WVP = DirectX::XMMatrixIdentity();
	DirectX::XMVECTOR rotXAxis = DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR rotYAxis = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR rotZAxis = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

	Rotation = DirectX::XMMatrixRotationAxis(rotYAxis, rot);
	RotationX = DirectX::XMMatrixRotationAxis(rotXAxis, rotX);
	RotationZ = DirectX::XMMatrixRotationAxis(rotZAxis, rotZ);
	Translation = DirectX::XMMatrixTranslation(0.0f, 0.0f, 4.0f);

	tri = Translation * Rotation * RotationX * RotationZ;
}

void StartTimer()
{
	LARGE_INTEGER freqCount;
	QueryPerformanceFrequency(&freqCount);

	countsPerSecond = double(freqCount.QuadPart);

	QueryPerformanceCounter(&freqCount);
	CounterStart = freqCount.QuadPart;
}

double GetTime()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);
	return double(currentTime.QuadPart - CounterStart) / countsPerSecond;
}

double GetFrameTime()
{
	LARGE_INTEGER currentTime;
	__int64 tickCount;
	QueryPerformanceCounter(&currentTime);

	tickCount = currentTime.QuadPart - frameTimeOld;
	frameTimeOld = currentTime.QuadPart;

	if (tickCount < 0.0f)
	{
		tickCount = 0.0f;
	}
	return float(tickCount) / countsPerSecond;
}

void UpdateCamera()
{
	camRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(camPitch/*x axis*/, camYaw/*y axis*/, 0/*z axis*/);	//Allows you to rotate around all axis at the same time

	camTarget = DirectX::XMVector3TransformCoord(DefaultForward, camRotationMatrix);			//Sets target vector, by rotating the defaultforward with the rotation matrix created above
																								//Seeting it the target matrix
	camTarget = DirectX::XMVector3Normalize(camTarget);											//Makes the target vectors a unit length between 1 and -1


	/*DirectX::XMMATRIX RotateYTempMatrix;
	RotateYTempMatrix = DirectX::XMMatrixRotationY(camYaw);

	camRight = DirectX::XMVector3TransformCoord(DefaultRight, RotateYTempMatrix);
	camUp = XMVector3TransformCoord(camUp, RotateYTempMatrix);
	camForward = XMVector3TransformCoord(DefaultForward, RotateYTempMatrix);*/

	camRight = XMVector3TransformCoord(DefaultRight, camRotationMatrix);
	camForward = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	camUp = XMVector3Cross(camForward, camRight);

	camPosition += moveLeftRight*camRight;
	camPosition += moveBackForward*camForward;

	moveLeftRight = 0.0f;
	moveBackForward = 0.0f;

	camTarget = camPosition + camTarget;
	camView = XMMatrixLookAtLH(camPosition, camTarget, camUp);

}

void InitD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC scd;	//Holds information about swap chain

	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));	//Clear out the struct		Quickly initalises the scd struct to NULL

	scd.BufferCount = 1;									//One back buffer				Just the count of back buffers on the swap chain
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		//32 bit colour					Each pixel is stored by its colour, this is a coded flag to determine the format
	scd.BufferDesc.Width = SCREEN_WIDTH;
	scd.BufferDesc.Height = SCREEN_HEIGHT;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;		//HOw swap chain will be used	DXGI_USAGE...OUTPUT is used when you want to draw graphics to the back buffer
															//Can also use DXGI_USAGE_SHARED which allows it to be shared across multiple device objects
	scd.OutputWindow = hWnd;								//Window that will be used
	scd.SampleDesc.Count = 4;								//How many multisamples		Tells D3D how to preform MSAA - Multisample anti-aliased rendering.
															//This renders the edges of shapes smoothly by blending each pixel with the surrounding pixels
															//Higher the number, the more level of detail, guaranteed to support up to 4, minimum is 1

	scd.Windowed = TRUE;									//WIll it be windowed or fullscreen
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;		//Allows full-screen switching

	D3D11CreateDeviceAndSwapChain( //Creates a device, device context, and swap chain using the information from the scd struct
		NULL,						//VAlue for what graphics adapter to use, NULL means default
		D3D_DRIVER_TYPE_HARDWARE,	//Whether D3D should use hardware or softweare for rendering
		NULL,						//Not relevant as its used for software rendering
		NULL,						//Flags alter how D3D runs
		NULL,						//Creates list of feature levels, tells D3d what features you expect the program to work with
		NULL,						//Indicates how many feature levels you have in the list of the previous parameter
		D3D11_SDK_VERSION,			//Stays the same with everything	Tells the users DirectX which version the game is developed for
		&scd,						//Pointer to swap chain description struct
		&swapchain,					//Pointer to a pointer of the swap chain object, this function creates that object for us
		&dev,						//Same as above, pointer to a pointer of a device object, this function will create the device and store the address in &dev
		NULL,						//Feature levels again
		&devcon);					//Filled with the address of the device context object


									/*Below vvvv
									First, determine address of the back buffer
									Second, create a COM object using that address to represent the render target
									Third, set the object as the active render target*/

									//COM's are C++ classes or groups of classes that you can call functions from.

									//Render target is a COM (Component Object Model) object that maintains a location in video memory for you to render to

	ID3D11Texture2D *pBackBuffer;		//ID3D11Texture2D is an object that stores a flat image
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);	//Get address of the backbuffer
																				//GetBuffer finds the back buffer on the swap chain and uses it to create the pBackBuffer texture object
																				//First parameter is the number of the back buffer to get, only one back buffer adn thats back buffer #0
																				//Second param is a number identifying the Id3D11Texture2D COM object. 
																				//Each COM has an ID thats used to get information about it
																				//To get this id __uuidof is needed, and we do this so that the GEtBuffer function knows what type of obeject 
																				//its supposed to be creating 
																				//Third parameter, void* is a pointer that points to no type of variable, and can be converted to any type of pointer
																				//The void pointer is filled with the lcoation of the ID3D11Texture2D object, 
																				//is a void as there are other objects we could ask for

	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);		//Creates the render target object.
																		//First param is a pointer to the texture
																		//Second, struct that describes render target, this isnt needed for the back buffer
																		//Third, address of the object pointer
	pBackBuffer->Release();					//Doesnt destroy the back buffer, just closes the texture object used to access it

	devcon->OMSetRenderTargets(1, &backbuffer, NULL);	//First, number of render targets
														//Second, pointer to a list of render target view objects, only have backbuffer

														//The render target is the back buffer itself -> The back buffer is the location in video memory where we render to


	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = SCREEN_WIDTH;
	viewport.Height = SCREEN_HEIGHT;

	devcon->RSSetViewports(1, &viewport);	//Activates viewpoint structs
											//First is the number of viewports being used
											//Second is the address of a list of pointers to the viewport structs

	InitPipeline();
	InitGraphics();
	InitCamera();
}

float* createColour(float red, float green, float blue, float alpha)
{

	float colour[4];
	colour[0] = red;
	colour[1] = green;
	colour[2] = blue;
	colour[3] = alpha;

	return colour;
}

void RenderFrame(void)
{
	devcon->ClearRenderTargetView(backbuffer, createColour(0.0f,0.2f,0.4f,1.0f));	//Fills target buffer with a colour
																					//ClearRenderTargetView fills a render target buffer with a specific colour
																					//First param, address of the render target object
																					//Second param, the colour to fill the back buffer with, 4 values are RGBA

																					// select which vertex buffer to display
	World = DirectX::XMMatrixIdentity();
	WVP = tri * camView * camProjection;
	cbPerObj.World = DirectX::XMMatrixTranspose(tri);
	cbPerObj.WVP = DirectX::XMMatrixTranspose(WVP);
	devcon->UpdateSubresource(cbPerObjectBuffer, 0, NULL, &cbPerObj, 0, 0);
	devcon->VSSetConstantBuffers(0, 1, &cbPerObjectBuffer);
	devcon->PSSetShaderResources(0, 1, &texture);

	devcon->PSSetSamplers(0, 1, &sampleState);


	UINT stride = sizeof(VERTEX);
	UINT offset = 0;
	devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

	devcon->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);

	// select which primtive type we are using
	devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);




	devcon->DrawIndexed(100, 0, 0);
	// draw the vertex buffer to the back buffer
	//devcon->Draw(4, 0);



	swapchain->Present(0, 0);	//Switch the back buffer to the front buffer
								//Basically performs the swap in the swap chain, so the back buffer becomes the front buffer

}

void InitPipeline(void)
{
	ID3D10Blob *VS, *PS;
	D3DCompileFromFile(L"VS.shader"/*file containing code*/, 0, 0, "ColorVertexShader"/*starting function*/, "vs_5_0", 0, 0, &VS, NULL /*blob containing the shader*/);
	//Loads contents from the file, compiles as a version 4.0 shader, and stores it in the blob VS
	D3DCompileFromFile(L"PS.shader", 0, 0, "ColorPixelShader", "ps_5_0", 0, 0, &PS, NULL);

	

	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	dev->CreatePixelShader(PS->GetBufferPointer()/*Address of compiled data*/, PS->GetBufferSize()/*Size of the data*/, NULL, &pPS/*Address of shader object*/);


	//Sets the shaders to be active
	devcon->VSSetShader(pVS, 0, 0);	//First, Address of the shader object to set
	devcon->PSSetShader(pPS, 0, 0);



	
	// creates input layout object
	D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "POSITION"/* Semantic used to tell the GPU what the value will be used for*/,
		0,
		DXGI_FORMAT_R32G32B32_FLOAT		/* Format, make sure it matches that ofthe vertices*/,
		0,
		0 ,								//Figures out many bytes into the struct that the element is
		D3D11_INPUT_PER_VERTEX_DATA,	//What the element is used as
		0								//Not used with the previous paramter, so is 0
		},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12 , D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	dev->CreateInputLayout(ied,							//Pointer to element description array
		2,							//Number of elements in array
		VS->GetBufferPointer(),		//Pointer to first shader in pipeline
		VS->GetBufferSize(),		//Size of the shader file
		&pLayout);					//Pointer to input layout object
	devcon->IASetInputLayout(pLayout);		//Sets input layout

	D3D11_SAMPLER_DESC samplerDesc;

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.BorderColor[0] = 0;
	samplerDesc.BorderColor[1] = 0;
	samplerDesc.BorderColor[2] = 0;
	samplerDesc.BorderColor[3] = 0;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	dev->CreateSamplerState(&samplerDesc, &sampleState);
}

void InitGraphics(void)
{
	VERTEX OurVertices[]	//Creates a triangle using VERTEX Struct
	{
		{ -0.45f, 0.5f, 0.0f, createColour(1.0f, 0.0f, 0.0f, 1.0f) },		//Builds vertex at the point of the first 3 numbers, then chooses the colour
		{ 0.45f, 0.5f, 0.0f, createColour(0.0f, 1.0f, 0.0f, 1.0f) },
		{ -0.45f, -0.5f, 0.0f, createColour(0.0f, 0.0f, 1.0f, 1.0f) },
		{ 0.45f, -0.5f, 0.0f, createColour(0.0f, 1.0f, .0f, 1.0f) },

		{ -0.45f, 0.5f, 0.95f, createColour(1.0f, 0.0f, 0.0f, 1.0f) },		//Builds vertex at the point of the first 3 numbers, then chooses the colour
		{ 0.45f, 0.5f, 0.95f, createColour(0.0f, 1.0f, 0.0f, 1.0f) },
		{ -0.45f, -0.5f, 0.95f, createColour(0.0f, 0.0f, 1.0f, 1.0f) },
		{ 0.45f, -0.5f, 0.95f, createColour(0.0f, 1.0f, .0f, 1.0f) },

	};

	short indices[] =	//Defines 2 triangles
	{
		0, 1, 2,    // side 1
		2, 1, 3,
		4, 0, 6,    // side 2
		6, 0, 2,
		7, 5, 6,    // side 3
		6, 5, 4,
		3, 1, 7,    // side 4
		7, 1, 5,
		4, 5, 0,    // side 5
		0, 5, 1,
		3, 7, 2,    // side 6
		2, 7, 6,

	};

	D3D11_BUFFER_DESC indexBufferDesc;						//Fills out an object of D3D11_BUFFER_DESC		
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));	//Clears enough memory to hold indexBufferDesc

	indexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;			//
	indexBufferDesc.ByteWidth = sizeof(indices) * 12;		//Each element in the index array is a DWORD type, get size of this type, multiple it by the amount of faces/triangles, then by 3 (vertices)
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;	//Tells IA of the pipeline that this is an index buffer
	indexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;

	iinitData.pSysMem = indices;
	dev->CreateBuffer(&indexBufferDesc, &iinitData, &indexBuffer);

	D3D11_MAPPED_SUBRESOURCE indexMS;

	devcon->Map(indexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &indexMS);	//Maps buffer
	memcpy(indexMS.pData, indices, sizeof(indices));							//Copies the data
	devcon->Unmap(indexBuffer, NULL);											//Unmaps the buffer and reallows the GPU access to it

	D3D11_BUFFER_DESC bd;				//Struct containing properties of the buffer
	ZeroMemory(&bd, sizeof(bd));		//Clear memory starting at bd, ending at the end of bd

	bd.Usage = D3D11_USAGE_DYNAMIC;						//CPU can write to it, while the GPU can only read it
	bd.ByteWidth = sizeof(VERTEX) * 8;					//Size is the vertex struct times 3
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;			//USe as a vertex buffer		Tells Direct3D what kind of buffer to make 
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;			//Allow the cpu to write in the buffer		Allows us to copy data from system memory into the buffer

	dev->CreateBuffer(&bd, NULL, &pVBuffer);	//Creates the buffer	First, Address of the description struct
												//						Second, can create the buffer with certain data when it's created
												//						Third, address of the buffer object, pointer to vertex buffer


	D3D11_MAPPED_SUBRESOURCE ms;	//Struct that will be filled with information about the buffer once its mapped
									//This info includes the pointer to the buffers location, accessed using "ms.pData"
	devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);		//maps buffer
																			//First, address of the buffer object
																			//Third, Controls the CPUs access to the buffer while its mapped 
																			//D3D11_MAP_WRITE_DISCARD erases previous contents of buffer, and the new buffer is open for writing
																			//Fourth, cAn be D3D11_MAP_FLAG_DO_NOT_WAIT, whihc forces the program to contiune even if the GPU is still working with the buffer

	memcpy(ms.pData, OurVertices, sizeof(OurVertices));						//Copies data
																			//ms.pData is the destination
																			//OurVertices is the source
																			//sizeof(OurVertices) is the size

	devcon->Unmap(pVBuffer, NULL);											//unmap buffer
																			//Reallows the GPU access to the buffer, reblocks CPU.
																			//First, the buffer, second is advanced

}

void InitCamera(void)
{
	D3D11_BUFFER_DESC cbbd;
	ZeroMemory(&cbbd, sizeof(D3D11_BUFFER_DESC));

	cbbd.Usage = D3D11_USAGE_DEFAULT;
	cbbd.ByteWidth = sizeof(cbPerObject);
	cbbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbbd.CPUAccessFlags = 0;
	cbbd.MiscFlags = 0;

	dev->CreateBuffer(&cbbd, NULL, &cbPerObjectBuffer);

	camPosition = DirectX::XMVectorSet(0.0f, 0.0f, -10.0f, 0.0f);
	camTarget = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	camUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	camView = DirectX::XMMatrixLookAtLH(camPosition, camTarget, camUp);	//Initialises the camers matrix

																		/*XMMATRIX XMMatrixPerspectiveFovLH
																		(
																		FLOAT FovAngleY,		Field of view in radians along the y axis
																		FLOAT AspectRatio,		Aspect Ratio
																		FLOAT NearZ,			Distance from camera to near z plane
																		FLOAT FarZ				Distance from camera to far plane		If object is further than this distance, it won't be rendered
																		)*/

	camProjection = DirectX::XMMatrixPerspectiveFovLH(0.4f*3.14f, (float)SCREEN_WIDTH / SCREEN_HEIGHT, 1.0f, 1000.0f);


}

bool InitDirectInput(HINSTANCE hInstance)
{
	DirectInput8Create(hInstance,		//Handle to the instance of application
		DIRECTINPUT_VERSION,			//Version fo direct input to use
		IID_IDirectInput8,				//Identifer to the interface of direct input 
		(void**)&DirectInput,			//Returned pointer to the direct input object
		NULL);							//Used for COM aggregation


	DirectInput->CreateDevice(GUID_SysKeyboard,	//Flag for the device to use, ie The keyboard
		&DIKeyboard,	//Returns pointer to the created device
		NULL);

	DirectInput->CreateDevice(GUID_SysMouse,	//Same as above, except for the mouse
		&DIMouse,
		NULL);

	//HRESULT IDirectInputDevice8::SetDataFormat(LPCDIDATAFORMAT lpdf);
	DIKeyboard->SetDataFormat(&c_dfDIKeyboard);	//Standard keyboard structure
	DIKeyboard->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

	DIMouse->SetDataFormat(&c_dfDIMouse);	//Standard mouse structure
	DIMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_NOWINKEY | DISCL_FOREGROUND);

	return true;
}

void DetectInput(double time)
{
	DIMOUSESTATE mouseCurrentState;

	BYTE keyboardState[256];

	DIKeyboard->Acquire();	//Gives program control over devices ->Acquire()
	DIMouse->Acquire();

	//typedef struct DIMOUSESTATE {
	//	LONG lX;	//Respective axis of the mouse	
	//	LONG lY;	//x and y are used if the mouse is moved
	//	LONG lZ;	//mouse wheel
	//	BYTE rgbButtons[4];	//Possible 4 buttons
	//} DIMOUSESTATE, *LPDIMOUSESTATE;

	DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrentState);
	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);

	float speed = 15.0f * time;
	if (keyboardState[DIK_ESCAPE] & 0x80)
	{
		PostMessage(hWnd, WM_DESTROY, 0, 0);
	}

	//Rotate
	if (keyboardState[DIK_A] & 0x80)
	{
		moveLeftRight -= speed;
	}
	if (keyboardState[DIK_D] & 0x80)
	{
		moveLeftRight += speed;
	}
	if (keyboardState[DIK_W] & 0x80)
	{
		moveBackForward += speed;
	}
	if (keyboardState[DIK_S] & 0x80)
	{
		moveBackForward -= speed;
	}

	//
	if ((mouseCurrentState.lX != mouseLastState.lX) || (mouseCurrentState.lY != mouseLastState.lY))
	{
		camYaw += mouseLastState.lX * 0.01f;

		camPitch += mouseCurrentState.lY * 0.01f;

		mouseLastState = mouseCurrentState;
	}

	UpdateCamera();

	return;
}

void CleanD3D(void)
{
	swapchain->SetFullscreenState(FALSE, NULL); //Switches to windowed mode		FALSE indicates windowed		NULL is the video adapater used, so NULL will choose the correct adapter

	sampleState->Release();
	indexBuffer->Release();
	DIKeyboard->Unacquire();
	DIMouse->Unacquire();
	DirectInput->Release();
	cbPerObjectBuffer->Release();
	pLayout->Release();
	pVBuffer->Release();
	pVS->Release();
	pPS->Release();
	swapchain->Release();
	backbuffer->Release();
	dev->Release();
	devcon->Release();
	//Clean up the COM objects or they'll stick around even after the program closes
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:	//This message is sent when the window is closing
	{
		PostQuitMessage(0);	//Sends message WM_QUIT, integer value of 0
							//GetMessage returns false when a 0 is sent, meaning the WinMain function can end
		return 0;
	}
	break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam); //Handles any messages the switch statement didnt
}