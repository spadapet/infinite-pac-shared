#pragma once

namespace ff
{
	class IThreadDispatch;

	class ThreadGlobals
	{
	public:
		UTIL_API ThreadGlobals(bool allowDispatch = true);
		UTIL_API virtual ~ThreadGlobals();

		UTIL_API static ff::ThreadGlobals *Get();
		UTIL_API static bool Exists();

		UTIL_API virtual bool Startup();
		UTIL_API virtual void Shutdown();

		UTIL_API unsigned int ThreadId() const;
		UTIL_API IThreadDispatch *GetDispatch() const;

		UTIL_API bool IsValid() const;
		UTIL_API bool IsShuttingDown() const;
		UTIL_API void AtShutdown(std::function<void()> func);

	protected:
		void CallShutdownFunctions();

		enum class State
		{
			UNSTARTED,
			STARTED,
			FAILED,
			SHUT_DOWN,
		} _state;

	private:
		Mutex _cs;
		unsigned int _id;
		bool _allowDispatch;
		ComPtr<IThreadDispatch> _dispatch;
		std::list<std::function<void()>> _shutdownFunctions;
	};

	UTIL_API void AtThreadShutdown(std::function<void()> func);
}
