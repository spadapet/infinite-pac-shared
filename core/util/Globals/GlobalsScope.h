#pragma once

namespace ff
{
	class ThreadGlobals;

	class GlobalsScope
	{
	public:
		UTIL_API GlobalsScope(ThreadGlobals &globals);
		UTIL_API ~GlobalsScope();

	private:
		ThreadGlobals &_globals;
	};

	// T must be ThreadGlobals or something derived from it
	template<typename T>
	class ThreadGlobalsScope
	{
	public:
		ThreadGlobalsScope();
		~ThreadGlobalsScope();

		T &GetGlobals();

	private:
		T _globals;
		GlobalsScope _scope;
	};
}

template<typename T>
ff::ThreadGlobalsScope<T>::ThreadGlobalsScope()
	: _scope(_globals)
{
}

template<typename T>
ff::ThreadGlobalsScope<T>::~ThreadGlobalsScope()
{
}

template<typename T>
T &ff::ThreadGlobalsScope<T>::GetGlobals()
{
	return _globals;
}
