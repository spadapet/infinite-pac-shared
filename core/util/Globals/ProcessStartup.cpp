#include "pch.h"
#include "Globals/ProcessStartup.h"

static ff::ProcessStartup *s_programStartup = nullptr;
static ff::ProcessShutdown *s_programShutdown = nullptr;

ff::ProcessStartup::ProcessStartup(FuncType func)
	: _func(func)
	, _next(s_programStartup)
{
	s_programStartup = this;
}

void ff::ProcessStartup::OnStartup(ProcessGlobals &program)
{
	for (const ff::ProcessStartup *func = s_programStartup; func != nullptr; func = func->_next)
	{
		if (func->_func != nullptr)
		{
			func->_func(program);
		}
	}
}

ff::ProcessShutdown::ProcessShutdown(FuncType func)
	: _func(func)
	, _next(s_programShutdown)
{
	s_programShutdown = this;
}

void ff::ProcessShutdown::OnShutdown()
{
	for (const ff::ProcessShutdown *func = s_programShutdown; func != nullptr; func = func->_next)
	{
		if (func->_func != nullptr)
		{
			func->_func();
		}
	}
}
