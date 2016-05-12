#include "pch.h"

GUID ff::CreateGuid()
{
	GUID guid;
	CoCreateGuid(&guid);
	return guid;
}
