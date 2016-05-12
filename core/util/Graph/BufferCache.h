#pragma once

namespace ff
{
	class IGraphDevice;

	class BufferCache
	{
	public:
		BufferCache(D3D11_BIND_FLAG binding);
		~BufferCache();

		IGraphDevice *GetDevice() const;
		void SetDevice(IGraphDevice *pDevice);
		void Reset();

		bool CreateBuffer(size_t nBytes, ID3D11Buffer **ppBuffer);
		bool CreateStaticBuffer(const void *pData, size_t nBytes, ID3D11Buffer **ppBuffer);

		bool BorrowBuffer(size_t nBytes, ID3D11Buffer **ppBuffer);
		bool BorrowAndMapBuffer(size_t nBytes, ID3D11Buffer **ppBuffer, void **ppMapped);
		void ReturnBuffer(ID3D11Buffer *pBuffer);

	private:
		IGraphDevice *_device;
		D3D11_BIND_FLAG _binding;
		std::vector<ComPtr<ID3D11Buffer>> _buffers[4];
		size_t _allocated[4];
	};

	// Automatically borrows a buffer from the cache, maps it, and releases it
	template<typename T>
	class AutoBufferMap
	{
	public:
		AutoBufferMap(BufferCache &cache, size_t nCount)
			: _cache(cache), _mem(nullptr), _count(nCount)
		{
			verify(_cache.BorrowAndMapBuffer(_count * sizeof(T), &_buffer, (void**)&_mem));
		}

		~AutoBufferMap()
		{
			Unmap();

			if (_buffer)
			{
				_cache.ReturnBuffer(_buffer);
			}
		}

		ID3D11Buffer *Unmap()
		{
			if (_mem)
			{
				_cache.GetDevice()->GetContext()->Unmap(_buffer, 0);
				_mem = nullptr;
			}
		
			return _buffer;
		}

		ID3D11Buffer *GetBuffer() { return _buffer; }
		T *GetMem() { return _mem; }
		size_t GetSize() { return _count * sizeof(T); }
		size_t GetCount() { return _count; }

	private:
		BufferCache &_cache;
		ComPtr<ID3D11Buffer> _buffer;
		T *_mem;
		size_t _count;
	};
}
