// STL
#include <iostream>
#include <cstdlib>
#include <memory>
#include <vector>

// ATL
#include <atlbase.h>

// CUDA
#include "cuda.h"
#include "cuda_runtime_api.h"

#pragma comment(lib, "cudart.lib")

// DXGI
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")

// D3D11
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")


int main(int argc, char** argv)
{
	std::string sDeviceName("GeForce GTX 750 Ti");
	std::wstring sDeviceNameWide(sDeviceName.begin(), sDeviceName.end());
	const size_t nWidth = 1920, nHeight = 1080, nIterations = 1000;
#pragma region Direct3D 11
	CComPtr<IDXGIFactory1> pDXGIFactory1;
	ATLENSURE_SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDXGIFactory1)));
	ULONG nAdapterIndex = 0;
	CComPtr<IDXGIAdapter1> pDXGIAdapter1;
	DXGI_ADAPTER_DESC1 DXGIAdapterDescription1 = {};
	bool bD3D11AdapterFound = false;
	while (SUCCEEDED(pDXGIFactory1->EnumAdapters1(nAdapterIndex++, &pDXGIAdapter1)))
	{
		ATLENSURE_SUCCEEDED(pDXGIAdapter1->GetDesc1(&DXGIAdapterDescription1));
		std::wstring sDescription(DXGIAdapterDescription1.Description);
		if (sDescription.find(sDeviceNameWide) != std::string::npos)
		{
			bD3D11AdapterFound = true;
			break;
		}
	}
	if (bD3D11AdapterFound == false)
	{
		std::cout << "Direct3D 11 compatbile adapter named " << sDeviceName.c_str() << "was not found!" << std::endl;
		return EXIT_FAILURE;
	}
	const D3D_FEATURE_LEVEL RequestedFeatureLevels = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL FeatureLevel;
	UINT nFlags = 0;
#ifdef _DEBUG
	nFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	CComPtr<ID3D11Device> pDevice;
	CComPtr<ID3D11DeviceContext> pDeviceContext;
	ATLENSURE_SUCCEEDED(D3D11CreateDevice(pDXGIAdapter1, D3D_DRIVER_TYPE_UNKNOWN, NULL, nFlags, &RequestedFeatureLevels, 1, D3D11_SDK_VERSION, &pDevice, &FeatureLevel, &pDeviceContext));
	std::unique_ptr<unsigned char[]> pFrame(new unsigned char[nWidth * nHeight * 3 / 2]);
	D3D11_TEXTURE2D_DESC TextureDescription = {};
	TextureDescription.Width = nWidth;
	TextureDescription.Height = nHeight;
	TextureDescription.Format = DXGI_FORMAT_NV12;
	TextureDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	TextureDescription.Usage = D3D11_USAGE_DYNAMIC;
	TextureDescription.MipLevels = 1;
	TextureDescription.ArraySize = 1;
	TextureDescription.SampleDesc.Count = 1;
	TextureDescription.BindFlags = D3D11_BIND_DECODER;
	CComPtr<ID3D11Texture2D> pTexture;
	ATLENSURE_SUCCEEDED(pDevice->CreateTexture2D(&TextureDescription, NULL, &pTexture));
	CComQIPtr<ID3D11Resource> pResource(pTexture);
	D3D11_MAPPED_SUBRESOURCE MappedSubresource = {};
	{
		FILETIME StartFileTime = {};
		::GetSystemTimeAsFileTime(&StartFileTime);
		for (size_t nIteration = 0; nIteration < nIterations; ++nIteration)
		{
			ATLENSURE_SUCCEEDED(pDeviceContext->Map(pResource, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource));
			_ASSERT(nWidth == MappedSubresource.RowPitch);
			{
				memcpy(MappedSubresource.pData, pFrame.get(), nWidth * nHeight * 3 / 2);
			}
			pDeviceContext->Unmap(pResource, 0);
		}
		FILETIME EndFileTime = {};
		::GetSystemTimeAsFileTime(&EndFileTime);
		ULARGE_INTEGER StartTime = { StartFileTime.dwLowDateTime, StartFileTime.dwHighDateTime }, EndTime = { EndFileTime.dwLowDateTime, EndFileTime.dwHighDateTime };
		double fElapsedMiliseconds = static_cast<double>((EndTime.QuadPart - StartTime.QuadPart) / 10000.0f);
		std::cout << "Map/memcpy/Unmap total time: " << fElapsedMiliseconds << " ms, " << fElapsedMiliseconds / nIterations << " per call" << std::endl;
	}
	{
		FILETIME StartFileTime = {};
		::GetSystemTimeAsFileTime(&StartFileTime);
		for (size_t nIteration = 0; nIteration < nIterations; ++nIteration)
		{
			ATLENSURE_SUCCEEDED(pDeviceContext->Map(pResource, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource));
			pDeviceContext->Unmap(pResource, 0);
		}
		FILETIME EndFileTime = {};
		::GetSystemTimeAsFileTime(&EndFileTime);
		ULARGE_INTEGER StartTime = { StartFileTime.dwLowDateTime, StartFileTime.dwHighDateTime }, EndTime = { EndFileTime.dwLowDateTime, EndFileTime.dwHighDateTime };
		double fElapsedMiliseconds = static_cast<double>((EndTime.QuadPart - StartTime.QuadPart) / 10000.0f);
		std::cout << "Map/Unmap total time: " << fElapsedMiliseconds << " ms, " << fElapsedMiliseconds / nIterations << " per call" << std::endl;
	}
	TextureDescription.Usage = D3D11_USAGE_DEFAULT;
	TextureDescription.CPUAccessFlags = 0;
	pTexture.Release();
	ATLENSURE_SUCCEEDED(pDevice->CreateTexture2D(&TextureDescription, NULL, &pTexture));
	pResource = pTexture;
	{
		FILETIME StartFileTime = {};
		::GetSystemTimeAsFileTime(&StartFileTime);
		for (size_t nIteration = 0; nIteration < nIterations; ++nIteration)
		{
			pDeviceContext->UpdateSubresource(pResource, 0, NULL, pFrame.get(), 1920, 0);
		}
		FILETIME EndFileTime = {};
		::GetSystemTimeAsFileTime(&EndFileTime);
		ULARGE_INTEGER StartTime = { StartFileTime.dwLowDateTime, StartFileTime.dwHighDateTime }, EndTime = { EndFileTime.dwLowDateTime, EndFileTime.dwHighDateTime };
		double fElapsedMiliseconds = static_cast<double>((EndTime.QuadPart - StartTime.QuadPart) / 10000.0f);
		std::cout << "UpdateSubresource total time: " << fElapsedMiliseconds << " ms, " << fElapsedMiliseconds / nIterations << " per call" << std::endl;
	}
#pragma endregion
#pragma region Cuda
	int nCudaDeviceCount = 0;
	auto nCudaError = cudaGetDeviceCount(&nCudaDeviceCount);
	_ASSERT(nCudaError == CUDA_SUCCESS);
	std::vector<cudaDeviceProp> Devices;
	Devices.resize(nCudaDeviceCount);
	bool bCudaDeviceFound = false;
	int nCudaDevice = 0;
	for (; nCudaDevice < nCudaDeviceCount; ++nCudaDevice)
	{
		nCudaError = cudaGetDeviceProperties(&Devices[nCudaDevice], nCudaDevice);
		_ASSERT(nCudaError == CUDA_SUCCESS);
		if (Devices[nCudaDevice].name == sDeviceName)
		{
			bCudaDeviceFound = true;
			break;
		}
	}
	if (bCudaDeviceFound == false)
	{
		std::cout << "Cuda compatbile adapter named " << sDeviceName.c_str() << "was not found!" << std::endl;
		return EXIT_FAILURE;
	}
	nCudaError = cudaSetDevice(nCudaDevice);
	_ASSERT(nCudaError == CUDA_SUCCESS);
	void *pHostMemory = NULL, *pDeviceMemory = NULL;
	nCudaError = cudaMalloc(&pDeviceMemory, nWidth * nHeight * 3 / 2);
	_ASSERT(nCudaError == CUDA_SUCCESS);
	nCudaError = cudaMallocHost(&pHostMemory, nWidth * nHeight * 3 / 2);
	_ASSERT(nCudaError == CUDA_SUCCESS);
	{
		FILETIME StartFileTime = {};
		::GetSystemTimeAsFileTime(&StartFileTime);
		for (size_t nIteration = 0; nIteration < nIterations; ++nIteration)
		{
			nCudaError = cudaMemcpy(pDeviceMemory, pFrame.get(), nWidth * nHeight * 3 / 2, cudaMemcpyHostToDevice);
			_ASSERT(nCudaError == CUDA_SUCCESS);
		}
		FILETIME EndFileTime = {};
		::GetSystemTimeAsFileTime(&EndFileTime);
		ULARGE_INTEGER StartTime = { StartFileTime.dwLowDateTime, StartFileTime.dwHighDateTime }, EndTime = { EndFileTime.dwLowDateTime, EndFileTime.dwHighDateTime };
		double fElapsedMiliseconds = static_cast<double>((EndTime.QuadPart - StartTime.QuadPart) / 10000.0f);
		std::cout << "cudaMemcpy total time: " << fElapsedMiliseconds << " ms, " << fElapsedMiliseconds / nIterations << " per call" << std::endl;

	}
	{
		FILETIME StartFileTime = {};
		::GetSystemTimeAsFileTime(&StartFileTime);
		for (size_t nIteration = 0; nIteration < nIterations; ++nIteration)
		{
			nCudaError = cudaMemcpyAsync(pDeviceMemory, pFrame.get(), nWidth * nHeight * 3 / 2, cudaMemcpyHostToDevice);
			_ASSERT(nCudaError == CUDA_SUCCESS);
		}
		cudaDeviceSynchronize();
		FILETIME EndFileTime = {};
		::GetSystemTimeAsFileTime(&EndFileTime);
		ULARGE_INTEGER StartTime = { StartFileTime.dwLowDateTime, StartFileTime.dwHighDateTime }, EndTime = { EndFileTime.dwLowDateTime, EndFileTime.dwHighDateTime };
		double fElapsedMiliseconds = static_cast<double>((EndTime.QuadPart - StartTime.QuadPart) / 10000.0f);
		std::cout << "cudaMemcpyAsync total time: " << fElapsedMiliseconds << " ms, " << fElapsedMiliseconds / nIterations << " per call" << std::endl;
	}
	{
		FILETIME StartFileTime = {};
		::GetSystemTimeAsFileTime(&StartFileTime);
		for (size_t nIteration = 0; nIteration < nIterations; ++nIteration)
		{
			nCudaError = cudaMemcpyAsync(pDeviceMemory, pHostMemory, nWidth * nHeight * 3 / 2, cudaMemcpyHostToDevice);
			_ASSERT(nCudaError == CUDA_SUCCESS);
		}
		cudaDeviceSynchronize();
		FILETIME EndFileTime = {};
		::GetSystemTimeAsFileTime(&EndFileTime);
		ULARGE_INTEGER StartTime = { StartFileTime.dwLowDateTime, StartFileTime.dwHighDateTime }, EndTime = { EndFileTime.dwLowDateTime, EndFileTime.dwHighDateTime };
		double fElapsedMiliseconds = static_cast<double>((EndTime.QuadPart - StartTime.QuadPart) / 10000.0f);
		std::cout << "cudaMemcpyAsync with cudaMalloc'ed input memory total time: " << fElapsedMiliseconds << " ms, " << fElapsedMiliseconds / nIterations << " per call" << std::endl;
	}
	cudaFree(pDeviceMemory);
	cudaFree(pHostMemory);
#pragma endregion
	return EXIT_SUCCESS;
}