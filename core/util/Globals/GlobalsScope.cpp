#include "pch.h"
#include "Globals/GlobalsScope.h"
#include "Globals/ThreadGlobals.h"

ff::GlobalsScope::GlobalsScope(ThreadGlobals &globals)
	: _globals(globals)
{
	assert(!_globals.IsValid());
	_globals.Startup();
}

ff::GlobalsScope::~GlobalsScope()
{
	_globals.Shutdown();
}
