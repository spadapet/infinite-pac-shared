#include "pch.h"
#include "COM/ComAlloc.h"
#include "COM/ComObject.h"
#include "Globals/GlobalsScope.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"

static __declspec(thread) ff::IThreadDispatch *s_dispatchOverride = nullptr;

class __declspec(uuid("58c5d38c-72f2-4b50-a92b-c92f14682f9c"))
	ThreadPool : public ff::ComBase, public ff::IThreadPool
{
public:
	DECLARE_HEADER(ThreadPool);

	bool Init();

	// IThreadPool functions
	virtual void Add(std::function<void()> runFunc, std::function<void()> completeFunc) override;
	virtual void Flush() override;
	virtual void Destroy() override;

private:
	struct WorkEntry
	{
		std::function<void()> _runFunc;
		std::function<void()> _completeFunc;
		ff::ComPtr<ff::IThreadDispatch> _dispatch;
	};

	void RunWork(WorkEntry *work);

	ff::Mutex _cs;
	ff::WinHandle _eventNoWork;
	ff::Condition _workCondition;
	ff::PoolAllocator<WorkEntry> _workAllocator;
	bool _destroyed;

#if !METRO_APP
	static unsigned int WINAPI WorkerThread(void *context);
	void WorkerThread();

	std::queue<WorkEntry *> _workQueue;
#endif
};

BEGIN_INTERFACES(ThreadPool)
	HAS_INTERFACE(ff::IThreadPool)
END_INTERFACES()

bool ff::CreateThreadPool(IThreadPool **obj)
{
	assertRetVal(obj, false);

	if (ff::GetThreadPool())
	{
		*obj = ff::GetAddRef(ff::GetThreadPool());
	}
	else
	{
		ff::ComPtr<ThreadPool> myObj;
		assertHrRetVal(ff::ComAllocator<ThreadPool>::CreateInstance(&myObj), false);
		assertRetVal(myObj->Init(), false);
		*obj = myObj.Detach();
	}

	return true;
}

ff::IThreadPool *ff::GetThreadPool()
{
	return ff::ProcessGlobals::Exists()
		? ff::ProcessGlobals::Get()->GetThreadPool()
		: nullptr;
}

ThreadPool::ThreadPool()
	: _destroyed(false)
	, _workAllocator(false)
{
	_eventNoWork = ff::CreateEvent(true);
}

ThreadPool::~ThreadPool()
{
	assertSz(_destroyed, L"The owner of ThreadPool should've called Destroy() by now");
}

bool ThreadPool::Init()
{
#if !METRO_APP
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	for (UINT i = 0, threadId = 0; i < std::min<UINT>(si.dwNumberOfProcessors, 8); i++)
	{
		AddRef();
		ff::WinHandle handle = (HANDLE)::_beginthreadex(nullptr, 0, ThreadPool::WorkerThread, this, 0, &threadId);
		ff::String threadName = ff::String::format_new(L"Pool thread %u", i);
		ff::SetDebuggerThreadName(threadName, threadId);
	}
#endif
	return true;
}

void ThreadPool::Add(std::function<void()> runFunc, std::function<void()> completeFunc)
{
	static std::function<void()> s_emptyFunc = []{};
	std::function<void()> &realRunFunc = runFunc ? runFunc : s_emptyFunc;
	std::function<void()> &realCompleteFunc = completeFunc ? completeFunc : s_emptyFunc;
	WorkEntry *work = nullptr;
	{
		ff::LockMutex crit(_cs);
		if (!_destroyed && ff::GetThreadDispatch())
		{
			work = _workAllocator.New();
			work->_runFunc = realRunFunc;
			work->_completeFunc = realCompleteFunc;
			work->_dispatch = ff::GetThreadDispatch();
#if METRO_APP
			ff::ComPtr<ThreadPool, ff::IThreadPool> keepAlive = this;
			Windows::System::Threading::ThreadPool::RunAsync(ref new Windows::System::Threading::WorkItemHandler(
				[work, keepAlive](Windows::Foundation::IAsyncAction ^action)
			{
				keepAlive->RunWork(work);
			}));
#else
			_workQueue.push(work);
#endif
			::ResetEvent(_eventNoWork);
		}
	}

	if (work)
	{
		_workCondition.WakeOne();
	}
	else // Just run it now, can't use a thread
	{
		realRunFunc();
		realCompleteFunc();
	}
}

void ThreadPool::Flush()
{
	ff::WaitForHandle(_eventNoWork);
}

void ThreadPool::Destroy()
{
	_cs.Enter();
	_destroyed = true;
	_cs.Leave();

	Flush();
	_workCondition.WakeAll();
}

void ThreadPool::RunWork(WorkEntry *work)
{
	noAssertRet(work);

	ff::IThreadDispatch *oldDispatch = s_dispatchOverride;
	s_dispatchOverride = work->_dispatch;
	work->_runFunc();
	s_dispatchOverride = oldDispatch;

	ff::ComPtr<ThreadPool, ff::IThreadPool> keepAlive = this;
	work->_dispatch->Post([work, keepAlive]()
	{
		work->_completeFunc();

		ff::LockMutex crit(keepAlive->_cs);
		keepAlive->_workAllocator.Delete(work);
		if (!keepAlive->_workAllocator.GetCurAlloc())
		{
			::SetEvent(keepAlive->_eventNoWork);
		}
	});
}

#if !METRO_APP

unsigned int ThreadPool::WorkerThread(void *context)
{
	ff::ThreadGlobals threadGlobals(false);
	ff::GlobalsScope threadScope(threadGlobals);
	assertRetVal(context && threadGlobals.IsValid(), 1);

	ThreadPool *pool = (ThreadPool *)context;
	pool->WorkerThread();
	pool->Release();
	return 0;
}

void ThreadPool::WorkerThread()
{
	while (true)
	{
		WorkEntry *work = nullptr;
		{
			ff::LockMutex crit(_cs);
			while (!_destroyed && _workQueue.empty())
			{
				_cs.WaitForCondition(_workCondition);
			}

			if (_destroyed && _workQueue.empty())
			{
				break;
			}

			if (!_workQueue.empty())
			{
				work = _workQueue.front();
				_workQueue.pop();
			}
			else
			{
				work = nullptr;
			}
		}

		RunWork(work);
	}
}

#endif
