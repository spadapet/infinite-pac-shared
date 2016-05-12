#pragma once

namespace ff
{
	class __declspec(uuid("691c1238-aaf7-439a-af66-d9bfdb897543")) __declspec(novtable)
		IThreadPool : public IUnknown
	{
	public:
		virtual void Add(std::function<void()> runFunc, std::function<void()> completeFunc = nullptr) = 0;
		virtual void Flush() = 0; // wait for everything to complete
		virtual void Destroy() = 0;
	};

	UTIL_API bool CreateThreadPool(IThreadPool **obj);
	UTIL_API IThreadPool *GetThreadPool();
}
