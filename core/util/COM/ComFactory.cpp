#include "pch.h"
#include "COM/ComAlloc.h"
#include "COM/ComFactory.h"
#include "Module/Module.h"

class __declspec(uuid("d79aaf15-483c-4040-9836-fc9cc2ff14ac"))
	ClassFactory : public ff::ComBase, public IClassFactory
{
public:
	DECLARE_HEADER(ClassFactory);

	void SetAllocator(REFGUID clsid, const ff::Module *module, ff::ClassFactoryFunc func);

	// IClassFactory
	COM_FUNC CreateInstance(IUnknown *unkOuter, REFIID iid, void **ppv) override;
	COM_FUNC LockServer(BOOL bLock) override;

private:
	const ff::Module *_module;
	ff::ClassFactoryFunc _func;
	GUID _clsid;
};

BEGIN_INTERFACES(ClassFactory)
	HAS_INTERFACE(IClassFactory)
END_INTERFACES()

bool ff::CreateClassFactory(REFGUID clsid, const ff::Module *module, ff::ClassFactoryFunc func, IClassFactory **factory)
{
	assertRetVal(factory && module && func, false);
	*factory = nullptr;

	ff::ComPtr<ClassFactory> myFactory;
	assertHrRetVal(ff::ComAllocator<ClassFactory>::CreateInstance(&myFactory), false);
	myFactory->SetAllocator(clsid, module, func);

	*factory = myFactory.Detach();
	return true;
}

ClassFactory::ClassFactory()
	: _module(nullptr)
	, _func(nullptr)
{
}

ClassFactory::~ClassFactory()
{
}

void ClassFactory::SetAllocator(REFGUID clsid, const ff::Module *module, ff::ClassFactoryFunc func)
{
	_module = module;
	_func = func;
	_clsid = clsid;
}

HRESULT ClassFactory::CreateInstance(IUnknown *unkOuter, REFIID iid, void **ppv)
{
	assertRetVal(ppv, E_INVALIDARG);
	assertRetVal(_func, E_FAIL);

	return _func(unkOuter, _clsid, iid, ppv);
}

HRESULT ClassFactory::LockServer(BOOL bLock)
{
	assertRetVal(_module, E_FAIL);

	if (bLock)
	{
		_module->AddRef();
	}
	else
	{
		_module->Release();
	}

	return S_OK;
}
