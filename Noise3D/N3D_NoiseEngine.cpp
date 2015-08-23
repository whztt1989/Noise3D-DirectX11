
/***********************************************************************

                           �ࣺNOISE Engine

			��������Ҫ�������ȫ�ֽӿںͱ�������ʼ����

************************************************************************/

#include "Noise3D.h"
static NoiseEngine* static_pEngine;

//���캯��
NoiseEngine::NoiseEngine()
{
	mRenderWindowTitle = L"Noise 3D - Render Window";
	m_pMainLoopFunction = nullptr;
	static_pEngine = this;
}

NoiseEngine::~NoiseEngine()
{
	
}

HWND NoiseEngine::CreateRenderWindow(UINT pixelWidth, UINT pixelHeight, LPCWSTR windowTitle, HINSTANCE hInstance)
{

	WNDCLASS wndclass;
	HWND outHWND;//���
	mRenderWindowHINSTANCE = hInstance;
	mRenderWindowTitle = windowTitle;
	mRenderWindowClassName = L"Noise Engine Render Window";

	//������ע��
	if (mFunction_InitWindowClass(&wndclass) == FALSE)
	{
		DEBUG_MSG1("Window Class ����ʧ��");
		return FALSE;
	};

	//��������

	outHWND = mFunction_InitWindow();
	if (outHWND == 0)
	{
		DEBUG_MSG1("���崴��ʧ��");
		return FALSE;
	};

	return outHWND;
};

BOOL NoiseEngine::InitD3D(HWND RenderHWND, UINT BufferWidth, UINT BufferHeight, BOOL IsWindowed)
{

	g_MainBufferPixelWidth = BufferWidth;
	g_MainBufferPixelHeight = BufferHeight;

	HRESULT hr = S_OK;
#pragma region InitDevice
	//�������жϼ����ؽ��

	//Ӳ����������
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,	//HAL Ӳ������
		D3D_DRIVER_TYPE_REFERENCE,	//REF�ο��豸
		D3D_DRIVER_TYPE_WARP,		//Windows Advanced Rasterization Platformֻ֧��DX10.1
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	//D3D���Եİ汾
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	//�豸������ǩ 
	UINT createDeviceFlags = 0;
#if defined(DEBUG)||defined(_DEBUG)		//D3D����ģʽ
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif


	//���оٳ�����Ӳ����ʽ ���Գ�ʼ�� ֱ���ɹ�
	UINT driverTypeIndex = 0;
	for (driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		//D3D_DRIVER_TYPE 
		g_Device_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDevice(
			NULL,				//null��ʾʹ������ʾ��
			g_Device_driverType,		//�������� HAL/REF
			NULL,
			D3D11_CREATE_DEVICE_DEBUG,//createDeviceFlags,	//�ǲ��ǵ���ģʽ	
			featureLevels,		//��D3Dѡ������Եİ汾
			numFeatureLevels,
			D3D11_SDK_VERSION,
			&g_pd3dDevice,		//����D3D�豸ָ��
			&g_Device_featureLevel,	//��������ʹ�õ����Եİ汾
			&g_pImmediateContext//����
			);
		//�����ɹ��˾Ͳ��ü������Դ���
		if (SUCCEEDED(hr))
		{
			break;
		};
	};
	//���Դ����豸ʧ��
	HR_DEBUG(hr, "d3d�豸����ʧ��");


	//�����ز���
	g_pd3dDevice->CheckMultisampleQualityLevels(
		DXGI_FORMAT_R8G8B8A8_UNORM, 4, &g_Device_MSAA4xQuality);//4x�Ӿ��һ�㶼֧�֣��������ֵһ������¶�����0
	if (g_Device_MSAA4xQuality > 0)
	{
		g_Device_MSAA4xEnabled = TRUE;	//4x����ݿ��Կ���
	};



	/*��佻����������
	�����������ڹ���BUFEER�Ľ�������Ҫ����back��front
	�������ڶര����Ⱦ
	DESC = Description*/
	DXGI_SWAP_CHAIN_DESC SwapChainParam;
	ZeroMemory(&SwapChainParam, sizeof(SwapChainParam));
	SwapChainParam.BufferCount = 1;
	SwapChainParam.BufferDesc.Width = g_MainBufferPixelWidth;
	SwapChainParam.BufferDesc.Height = g_MainBufferPixelHeight;
	SwapChainParam.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainParam.BufferDesc.RefreshRate.Numerator = 60;//	����= =��
	SwapChainParam.BufferDesc.RefreshRate.Denominator = 1;//��ĸ
	SwapChainParam.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;//BACKBUFFER��ô��ʹ��
	SwapChainParam.OutputWindow = RenderHWND;
	SwapChainParam.Windowed = IsWindowed;
	SwapChainParam.SampleDesc.Count = (g_Device_MSAA4xEnabled = TRUE ? 4 : 1);//���ز�������
	SwapChainParam.SampleDesc.Quality = (g_Device_MSAA4xEnabled = TRUE ? g_Device_MSAA4xQuality - 1 : 0);//quality֮ǰ��ȡ��

	 //�����COM��QueryInterface ��һ���ӿڲ�ѯ��һ���ӿ�
	IDXGIDevice *dxgiDevice = 0;
	g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	IDXGIAdapter *dxgiAdapter = 0;
	dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
	IDXGIFactory *dxgiFactory = 0;
	dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

	//���ڴ�����һ��������
	hr = dxgiFactory->CreateSwapChain(
		g_pd3dDevice,		//�豸��ָ��
		&SwapChainParam,	//������������
		&g_pSwapChain);		//���صĽ�����ָ��
	HR_DEBUG(hr, "SwapChain����ʧ�ܣ�");
	
	dxgiFactory->Release();
	dxgiDevice->Release();
	dxgiAdapter->Release();
#pragma endregion InitDevice

	//��������������Ⱦ�ӿڣ����/ģ�� �ӿ�
	//��ЩViews�������󶨵�pipeline��
#pragma region CreateViews

	// ����һ��(���Զ��)��Ⱦ��ͼRENDER TARGET VIEW
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return FALSE;

	hr = g_pd3dDevice->CreateRenderTargetView(
		pBackBuffer,
		NULL,					//�������һ��D3D11_RENDERTARGETVIEW_DESC
		&g_pRenderTargetView);	//����һ����Ⱦ�ӿ�

	pBackBuffer->Release();		//�Ѿ������˵���ʱ�ӿ�- -

	HR_DEBUG(hr, "����RENDER TARGET VIEWʧ��");



	//����depth/stencil view
	D3D11_TEXTURE2D_DESC DSBufferDesc;
	DSBufferDesc.Width = BufferWidth;
	DSBufferDesc.Height = BufferHeight;
	DSBufferDesc.MipLevels = 1;
	DSBufferDesc.ArraySize = 1;
	DSBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DSBufferDesc.SampleDesc.Count = (g_Device_MSAA4xEnabled = TRUE ? 4 : 1);
	DSBufferDesc.SampleDesc.Quality = (g_Device_MSAA4xEnabled = TRUE ? g_Device_MSAA4xQuality - 1 : 0);
	DSBufferDesc.Usage = D3D11_USAGE_DEFAULT;	//��������DYNAMIC��STAGING
	DSBufferDesc.CPUAccessFlags = 0;	//CPU�������� GPU���� �����ܹ��ӿ�
	DSBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;//��PIPELINE�İ�
	DSBufferDesc.MiscFlags = 0;

	ID3D11Texture2D* pDepthStencilBuffer;
	g_pd3dDevice->CreateTexture2D(&DSBufferDesc, 0, &pDepthStencilBuffer);//����һ��������
	hr = g_pd3dDevice->CreateDepthStencilView(
		pDepthStencilBuffer,
		0,
		&g_pDepthStencilView);	//����һ��depth/stencil�ӿ�ָ��

	pDepthStencilBuffer->Release();
	if (FAILED(hr))
	{
		return FALSE;
	};


	//������Ⱦ���󣺸ոմ�������Ⱦ�ӿں�depth/stencil��
	//����ǰ󶨵�pipeline
	g_pImmediateContext->OMSetRenderTargets(
		1,
		&g_pRenderTargetView,
		g_pDepthStencilView);

#pragma endregion CreateViews


	//XY����-1��1�����Z��0��1��DX11����Ĭ�ϴ����ӿڣ�DX9�ͻ�
#pragma region CreateViewPort

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)BufferWidth;		//�ӿ�WIDTH ���󻺳���һ��
	vp.Height = (FLOAT)BufferHeight;	//�ӿ�Height
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	//SetViewport ����1���ӿڵĸ��� ����2���ӿ�������׵�ַ
	g_pImmediateContext->RSSetViewports(1, &vp);

#pragma endregion CreateViewPort

	return TRUE;

};

void NoiseEngine::ReleaseAll()//�������ڹ��캯����Ū��AddToReleaseList��
{
	g_Debug_MsgString.clear();
	//g_pImmediateContext->Flush();
	//g_pImmediateContext->ClearState();

	ReleaseCOM(g_pRenderTargetView);
	ReleaseCOM(g_pSwapChain);
	ReleaseCOM(g_pVertexLayout_Default);
	ReleaseCOM(g_pVertexLayout_Simple);
	ReleaseCOM(g_pDepthStencilView);
	ReleaseCOM(g_pImmediateContext);
	//g_pd3dDevice->Release();
	ReleaseCOM(g_pd3dDevice);

}

void NoiseEngine::Mainloop()
{
	MSG msg;//��Ϣ��
	ZeroMemory(&msg, sizeof(msg));

	//�ڳ���û�н��յ����˳�����Ϣ��ʱ��
	while (msg.message != WM_QUIT)
	{
		/*PM_REMOVE  PeekMessage��������Ϣ�Ӷ����������
		������PeekMesssge���Բ鿴�ķ�ʽ��ϵͳ�л�ȡ��Ϣ��
		���Բ�����Ϣ��ϵͳ���Ƴ����Ƿ�����������
		��ϵͳ����Ϣʱ������FALSE������ִ�к������롣*/

		//����Ϣ��ʱ��Ͻ������� ��Peek����TRUE��ʱ��
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);//������Ϣ�����͵�windows��Ϣ����
			DispatchMessage(&msg);//������Ϣ
		}
		else
			//��Ϣ����û���������� �Ǿ͸���ѭ��
		{
			switch (mMainLoopStatus)
			{
			case NOISE_MAINLOOP_STATUS_BUSY:
				//�����ѭ��������Ч
				if (m_pMainLoopFunction)
				{
					(*m_pMainLoopFunction)();
				}
					break;

			case NOISE_MAINLOOP_STATUS_QUIT_LOOP:
				return;
				break;

			default:
				break;

			}//switch
		}
	};

	ReleaseAll();
	UnregisterClass(mRenderWindowClassName, mRenderWindowHINSTANCE);
}

void NoiseEngine::SetMainLoopFunction( void (*pFunction)(void) )
{
	m_pMainLoopFunction =pFunction;
}

void NoiseEngine::SetMainLoopStatus(NOISE_MAINLOOP_STATUS loopStatus)
{
	mMainLoopStatus = loopStatus;
};


/************************************************************************
										 PRIVATE                               
************************************************************************/

//windows message handle
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)//��Ϣ�Ĵ�������
{
	/*HDC			hdc ;
	PAINTSTRUCT ps ;
	RECT        rect ;*/
	switch (message)
	{
	case WM_CREATE:
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);

}

BOOL NoiseEngine::mFunction_InitWindowClass(WNDCLASS* wc)
{
	wc->style = CS_HREDRAW | CS_VREDRAW; //��ʽ
	wc->cbClsExtra = 0;
	wc->cbWndExtra = 0;
	wc->hInstance = mRenderWindowHINSTANCE;//����ʵ��������windows�Զ��ַ�
	wc->hIcon = LoadIcon(NULL, IDI_APPLICATION);//��ʾ�����ͼ��titlte
	wc->hCursor = LoadCursor(NULL, IDC_ARROW);//���ڹ��
	wc->hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//����ˢ
	wc->lpszMenuName = NULL;
	wc->lpfnWndProc = WndProc;//���ô������windws��Ϣ����
	wc->lpszClassName = mRenderWindowClassName;//��������

	if (!RegisterClass(wc))//ע�ᴰ����
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"), TEXT("R"), MB_ICONERROR);
		return FALSE;
	}
	else
	{
		return TRUE;
	};

};

HWND NoiseEngine::mFunction_InitWindow()
{
	HWND hwnd;
	hwnd = CreateWindow(
		mRenderWindowClassName,      // window class name
		mRenderWindowTitle,   // window caption
		WS_OVERLAPPEDWINDOW, // window style
		CW_USEDEFAULT,// initial x position
		CW_USEDEFAULT,// initial y position
		640,// initial x size
		480,// initial y size
		NULL, // parent window handle
		NULL, // window menu handle
		mRenderWindowHINSTANCE, // program instance handle
		NULL);

	if (hwnd == 0)
	{
		//����ʧ��
		return 0;
	}
	else
	{
		//�����ɹ�
		ShowWindow(hwnd, SW_RESTORE);//��ʾ����
		UpdateWindow(hwnd);//���´���
		return hwnd;
	};

};