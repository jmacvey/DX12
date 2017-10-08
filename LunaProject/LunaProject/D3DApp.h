#ifndef D3D_APP_H
#define D3D_APP_H

#include "d3dUtil.h"
#include "GameTimer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")


class D3DApp {
protected:
	
	/// <summary>
	/// Initializes the data members to default values
	/// </summary>
	/// <param name="hInstance">the handle to the application instance</param>
	D3DApp(HINSTANCE hInstance);

	/// <summary>
	/// Copy Constructor (Disallowed) 
	/// </summary>
	/// <param name="rhs"></param>
	D3DApp(const D3DApp& rhs) = delete;

	/// <summary>
	/// Assignment Operator (Disallowed)
	/// </summary>
	/// <param name="rhs"></param>
	/// <returns></returns>
	D3DApp& operator=(const D3DApp& rhs) = delete;

	/// <summary>
	/// Destructor
	/// </summary>
	virtual ~D3DApp();

public:

	/// <summary>
	/// Gets a pointer to the application
	/// </summary>
	/// <returns>Pointer to the application</returns>
	static D3DApp* GetApp();

	/// <summary>
	/// Gets the handle to application instance
	/// </summary>
	/// <returns>handle to the application instance</returns>
	HINSTANCE AppInst() const;

	/// <summary>
	/// Gets the handle to the application window
	/// </summary>
	/// <returns>handle to the application window</returns>
	HWND MainWnd() const;

	/// <summary>
	/// Gets the aspect ratio
	/// </summary>
	/// <returns>the aspect ratio</returns>
	float AspectRatio() const;

	/// <summary>
	/// Gets the state of 4xMsaa
	/// </summary>
	/// <returns></returns>
	bool Get4xMsaaState() const;

	/// <summary>
	/// Sets the state of 4xMsaa
	/// </summary>
	/// <param name="value">Whether 4xMsaa samping is enabled</param>
	void Set4xMsaaState(bool value);

	int Run();

	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lParam);

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void OnRestore();
	virtual void Update(const GameTimer& gt) = 0;
	virtual void Draw(const GameTimer& gt) = 0;

	// Convenience overrides for handling mouse input
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

protected:
	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	/// <summary>
	/// Gets the current back buffer resource
	/// </summary>
	/// <returns></returns>
	ID3D12Resource* CurrentBackBuffer() const;

	/// <summary>
	/// Gets the current back buffer descriptor (RTV)
	/// </summary>
	/// <returns></returns>
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;

	/// <summary>
	/// Returns the depth stencil descriptor (DSV)
	/// </summary>
	/// <returns></returns>
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	/// <summary>
	/// Logs adapters to the debug console
	/// </summary>
	void LogAdapters();

	/// <summary>
	/// Logs outputs for a given adapter to the debug console
	/// </summary>
	/// <param name="adapter">the adapter</param>
	void LogAdapterOutputs(IDXGIAdapter* adapter);

	/// <summary>
	/// Logs output display modes for a given output
	/// </summary>
	/// <param name="output">the output</param>
	/// <param name="format">the format the display mode should match</param>
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

	/// <summary>
	/// Calculates the stats for a given frame
	/// </summary>
	void CalculateFrameStats();
protected:
	static D3DApp* mApp;

	HINSTANCE mhAppInst = nullptr; // application instance handle
	HWND mhMainWnd = nullptr; // main window handle
	bool mAppPaused = false; // is app paused?
	bool mMinimized = false; // is app minimized?
	bool mMaximized = false; // is app mazimized?
	bool mResizing = false; // Are the resize bars being dragged?
	bool mFullScreenState = false; // full screen enabled

	bool m4xMsaaState = false; // 4X MSAA enabled
	UINT m4xMsaaQuality = 0; // quality level of 4X MSAA

	// used to keept rack of the "delta-time" and game time 
	GameTimer mTimer;

	ComPtr<IDXGIFactory4> mdxgiFactory;
	ComPtr<IDXGISwapChain> mSwapChain;
	ComPtr<ID3D12Device> md3dDevice;

	ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence;

	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;

	ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	ComPtr<ID3D12Resource> mDepthStencilBuffer;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvDescriptorSize = 0;

	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;

	/// <summary>
	/// Each element in the back buffer texture format has 
	/// four 8-bit unsigned integers mapped to to [0, 1] range
	/// </summary>
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	/// <summary>
	/// 32 bit unsigned int
	/// First 24 bits accessed as float for depth, and last 8 bits are accessed as UINT for stencil
	/// </summary>
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	/// <summary>
	/// The width of the client window
	/// </summary>
	int mClientWidth = 800;

	/// <summary>
	/// The height of the client window
	/// </summary>
	int mClientHeight = 600;
};

#endif