#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "String/StringAlloc.h"

// static
wchar_t *ff::StringAllocator::NewPoolString(size_t count)
{
	return ff::ProcessGlobals::Get()->GetStringManager().New(count);
}

// static
void ff::StringAllocator::DeletePoolString(wchar_t *str)
{
	ff::ProcessGlobals::Get()->GetStringManager().Delete(str);
}

// static
ff::SharedStringVectorAllocator::SharedStringVector
	*ff::SharedStringVectorAllocator::NewSharedStringVector()
{
	return ff::ProcessGlobals::Get()->GetStringManager().NewVector();
}

// static
ff::SharedStringVectorAllocator::SharedStringVector
	*ff::SharedStringVectorAllocator::NewSharedStringVector(const SharedStringVector &rhs)
{
	return ff::ProcessGlobals::Get()->GetStringManager().NewVector(rhs);
}

// static
ff::SharedStringVectorAllocator::SharedStringVector
	*ff::SharedStringVectorAllocator::NewSharedStringVector(SharedStringVector &&rhs)
{
	return ff::ProcessGlobals::Get()->GetStringManager().NewVector(std::move(rhs));
}

// static
void ff::SharedStringVectorAllocator::DeleteSharedStringVector(SharedStringVector *str)
{
	ff::ProcessGlobals::Get()->GetStringManager().DeleteVector(str);
}
