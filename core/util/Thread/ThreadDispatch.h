#pragma once

namespace ff
{
	class __declspec(uuid("f42db5ff-728e-4717-9792-b6733e875021")) __declspec(novtable)
		IThreadDispatch : public IUnknown
	{
	public:
		virtual void Post(std::function<void()> func, bool runIfCurrentThread = false) = 0;
		virtual void Send(std::function<void()> func) = 0;
		virtual bool IsCurrentThread() const = 0;
		virtual DWORD GetThreadId() const = 0;

		virtual void Flush() = 0;
		virtual void Destroy() = 0;

		virtual size_t WaitForAnyHandle(const HANDLE *handles, size_t count) = 0;
	};

	UTIL_API bool CreateCurrentThreadDispatch(IThreadDispatch **obj);
	UTIL_API IThreadDispatch *GetThreadDispatch();
	UTIL_API IThreadDispatch *GetCurrentThreadDispatch();
	UTIL_API IThreadDispatch *GetMainThreadDispatch();
	UTIL_API IThreadDispatch *GetGameThreadDispatch();
}
