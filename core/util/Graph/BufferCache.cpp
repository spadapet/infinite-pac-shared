#include "pch.h"
#include "Graph/BufferCache.h"
#include "Graph/GraphDevice.h"

ff::BufferCache::BufferCache(D3D11_BIND_FLAG binding)
	: _binding(binding)
	, _device(nullptr)
{
	ZeroObject(_allocated);
}

ff::BufferCache::~BufferCache()
{
}

ff::IGraphDevice *ff::BufferCache::GetDevice() const
{
	return _device;
}

void ff::BufferCache::SetDevice(IGraphDevice *pDevice)
{
	_device = pDevice;
}

void ff::BufferCache::Reset()
{
	for (size_t i = 0; i < _countof(_buffers); i++)
	{
		_buffers[i].clear();
	}

	ZeroObject(_allocated);
}

static size_t GetMaxBufferCount()
{
	return 4;
}

static size_t GetBufferIndex(size_t nBytes)
{
	if (nBytes <= 1024)
	{
		return 0;
	}
	else if (nBytes <= 4096)
	{
		return 1;
	}
	else if (nBytes <= 32768)
	{
		return 2;
	}
	else
	{
		return 3;
	}
}

static size_t GetAllocationSize(size_t nMinimumBytes)
{
	if (nMinimumBytes <= 1024)
	{
		return 1024;
	}
	else if (nMinimumBytes <= 4096)
	{
		return 4096;
	}
	else if (nMinimumBytes <= 32768)
	{
		return 32768;
	}
	else
	{
		size_t nBytes = 65536;

		while (nBytes < nMinimumBytes)
		{
			nBytes *= 2;
		}

		return nBytes;
	}
}

static size_t GetBufferSize(ID3D11Buffer *pBuffer)
{
	D3D11_BUFFER_DESC desc;
	pBuffer->GetDesc(&desc);

	return desc.ByteWidth;
}

bool ff::BufferCache::CreateBuffer(size_t nBytes, ID3D11Buffer **ppBuffer)
{
	assertRetVal(nBytes && ppBuffer && _device && _device->Get3d(), false);

	D3D11_BUFFER_DESC desc;
	ZeroObject(desc);

	desc.ByteWidth = (UINT)nBytes;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = _binding;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	assertHrRetVal(_device->Get3d()->CreateBuffer(&desc, nullptr, ppBuffer), false);
	assertRetVal(*ppBuffer, false);

	return true;
}

bool ff::BufferCache::CreateStaticBuffer(const void *pData, size_t nBytes, ID3D11Buffer **ppBuffer)
{
	assertRetVal(ppBuffer && pData && nBytes, false);

	D3D11_BUFFER_DESC desc;
	ZeroObject(desc);

	desc.ByteWidth = (UINT)nBytes;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = _binding;

	D3D11_SUBRESOURCE_DATA data;
	ZeroObject(data);
	data.pSysMem = pData;

	assertHrRetVal(_device->Get3d()->CreateBuffer(&desc, &data, ppBuffer), false);
	assertRetVal(*ppBuffer, false);

	return true;
}

bool ff::BufferCache::BorrowBuffer(size_t nBytes, ID3D11Buffer **ppBuffer)
{
	return BorrowAndMapBuffer(nBytes, ppBuffer, nullptr);
}

void ff::BufferCache::ReturnBuffer(ID3D11Buffer *pBuffer)
{
	assertRet(pBuffer);

	size_t nBytes = GetBufferSize(pBuffer);
	size_t nBuffer = GetBufferIndex(nBytes);

	_buffers[nBuffer].push_back(pBuffer);
}

bool ff::BufferCache::BorrowAndMapBuffer(size_t nBytes, ID3D11Buffer **ppBuffer, void **ppMapped)
{
	assertRetVal(ppBuffer, false);

	size_t nBuffer = GetBufferIndex(nBytes);
	size_t nLargestBuffer = _countof(_buffers) - 1;

	// Try to reuse a cached buffer

	for (auto i = _buffers[nBuffer].begin(); i != _buffers[nBuffer].end(); i++)
	{
		size_t nCurSize = GetBufferSize(*i);

		if (nCurSize >= nBytes)
		{
			if (ppMapped)
			{
				// Try to lock the buffer

				D3D11_MAPPED_SUBRESOURCE map;
				assertHrRetVal(_device->GetContext()->Map(*i, 0, D3D11_MAP_WRITE_DISCARD, 0, &map), false);
				*ppMapped = map.pData;
			}

			*ppBuffer = ff::GetAddRef<ID3D11Buffer>(*i);
			_buffers[nBuffer].erase(i);

			return true;
		}
	}

	// See if one of the large buffers should be thrown away
	// (to make room for a larger one)

	if (nBuffer == nLargestBuffer &&
		_allocated[nLargestBuffer] >= GetMaxBufferCount() &&
		!_buffers[nLargestBuffer].empty())
	{
		// Throw away one of the "large" buffers, it isn't large enough

		for (auto i = _buffers[nLargestBuffer].begin(); i != _buffers[nLargestBuffer].end(); i++)
		{
			size_t nCurSize = GetBufferSize(*i);

			if (nCurSize < nBytes)
			{
				_buffers[nLargestBuffer].erase(i);
				_allocated[nLargestBuffer]--;

				break;
			}
		}
	}

	// Add a few empty buffers for later

	if (nBuffer != nLargestBuffer && !_allocated[nBuffer])
	{
		for (size_t i = 0; i < GetMaxBufferCount() - 1; i++)
		{
			ComPtr<ID3D11Buffer> pBuffer;
			assertRetVal(CreateBuffer(GetAllocationSize(nBytes), &pBuffer), false);
			_buffers[nBuffer].push_back(pBuffer);
			_allocated[nBuffer]++;
		}
	}

	// Can't reuse an existing buffer, so create a new one
	{
		ComPtr<ID3D11Buffer> pBuffer;
		assertRetVal(CreateBuffer(GetAllocationSize(nBytes), &pBuffer), false);

		if (ppMapped)
		{
			D3D11_MAPPED_SUBRESOURCE map;
			assertHrRetVal(_device->GetContext()->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map), false);
			*ppMapped = map.pData;
		}

		*ppBuffer = pBuffer.Detach();

		_allocated[nBuffer]++;

		return true;
	}
}
