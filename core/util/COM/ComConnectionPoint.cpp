#include "pch.h"
#include "COM/ComConnectionPoint.h"

BEGIN_INTERFACES(ff::ConnectionPointEnum)
	HAS_INTERFACE(IEnumConnectionPoints)
END_INTERFACES()

ff::ConnectionPointEnum::ConnectionPointEnum()
	: _pos(0)
{
}

ff::ConnectionPointEnum::~ConnectionPointEnum()
{
}

// static
bool ff::ConnectionPointEnum::Create(IComObject *parent, ConnectionPointEnum **value)
{
	assertRetVal(parent && value, false);

	ff::ComPtr<ConnectionPointEnum> myValue;
	assertHrRetVal(ff::ComAllocator<ConnectionPointEnum>::CreateInstance(&myValue), false);
	myValue->_parent = parent;

	*value = myValue.Detach();
	return S_OK;
}

HRESULT ff::ConnectionPointEnum::Next(ULONG cConnections, LPCONNECTIONPOINT *ppCP, ULONG *pcFetched)
{
	assertRetVal(ppCP, E_INVALIDARG);

	if (pcFetched)
	{
		*pcFetched = 0;
	}

	ff::ComPtr<IConnectionPointContainer> container;
	assertRetVal(container.QueryFrom(_parent), S_FALSE);

	for (size_t index = 0; cConnections > 0; cConnections--, _pos++, index++)
	{
		IConnectionPointHolder *holder = _parent->GetComConnectionPointHolder(_pos);
		if (holder != nullptr)
		{
			ppCP[index] = GetAddRef(holder->GetConnectionPoint(container));

			if (pcFetched)
			{
				++*pcFetched;
			}
		}
		else
		{
			return S_FALSE;
		}
	}

	return S_OK;
}

HRESULT ff::ConnectionPointEnum::Skip(ULONG cConnections)
{
	_pos += cConnections;
	return _parent->GetComConnectionPointHolder(_pos) == nullptr ? S_FALSE : S_OK;
}

HRESULT ff::ConnectionPointEnum::Reset()
{
	_pos = 0;
	return S_OK;
}

HRESULT ff::ConnectionPointEnum::Clone(IEnumConnectionPoints **value)
{
	assertRetVal(value, E_INVALIDARG);

	ff::ComPtr<ConnectionPointEnum> clone;
	assertRetVal(Create(_parent, &clone), E_FAIL);
	clone->_pos = _pos;

	*value = clone.Detach();
	return S_OK;
}

class __declspec(uuid("bbf60820-23e9-4a5f-86e9-4cf93041810f"))
	ConnectionEnum
		: public ff::ComBase
		, public IEnumConnections
{
public:
	DECLARE_HEADER(ConnectionEnum);

	static bool Create(ff::ConnectionPointBase::SharedSinksPtr sinks, ConnectionEnum **value);

	// IEnumConnections
	COM_FUNC Next(ULONG cConnections, LPCONNECTDATA rgcd, ULONG *pcFetched) override;
	COM_FUNC Skip(ULONG cConnections) override;
	COM_FUNC Reset() override;
	COM_FUNC Clone(IEnumConnections **value) override;

private:
	ff::ConnectionPointBase::SharedSinksPtr _sinks;
	size_t _pos;
};

BEGIN_INTERFACES(ConnectionEnum)
	HAS_INTERFACE(IEnumConnections)
END_INTERFACES()

ConnectionEnum::ConnectionEnum()
	: _pos(0)
{
}

ConnectionEnum::~ConnectionEnum()
{
}

// static
bool ConnectionEnum::Create(ff::ConnectionPointBase::SharedSinksPtr sinks, ConnectionEnum **value)
{
	assertRetVal(value, false);

	ff::ComPtr<ConnectionEnum> myValue;
	assertHrRetVal(ff::ComAllocator<ConnectionEnum>::CreateInstance(&myValue), false);
	myValue->_sinks = sinks;

	*value = myValue.Detach();
	return S_OK;
}

HRESULT ConnectionEnum::Next(ULONG cConnections, LPCONNECTDATA rgcd, ULONG *pcFetched)
{
	assertRetVal(rgcd, E_INVALIDARG);

	if (pcFetched)
	{
		*pcFetched = 0;
	}

	size_t count = _sinks != nullptr ? _sinks->Size() : 0;
	for (size_t index = 0; cConnections > 0; cConnections--, _pos++, index++)
	{
		if (_pos < count)
		{
			rgcd[index].pUnk = ff::GetAddRef<IUnknown>(_sinks->GetAt(_pos)._sink);
			rgcd[index].dwCookie = _sinks->GetAt(_pos)._cookie;

			if (pcFetched)
			{
				++*pcFetched;
			}
		}
		else
		{
			return S_FALSE;
		}
	}

	return S_OK;
}

HRESULT ConnectionEnum::Skip(ULONG cConnections)
{
	size_t count = _sinks != nullptr ? _sinks->Size() : 0;
	_pos += cConnections;

	return _pos > count ? S_FALSE : S_OK;
}

HRESULT ConnectionEnum::Reset()
{
	_pos = 0;
	return S_OK;
}

HRESULT ConnectionEnum::Clone(IEnumConnections **value)
{
	assertRetVal(value, E_INVALIDARG);

	ff::ComPtr<ConnectionEnum> clone;
	assertRetVal(Create(_sinks, &clone), E_FAIL);
	clone->_pos = _pos;

	*value = clone.Detach();
	return S_OK;
}

bool ff::ConnectionPointBase::Sink::operator==(const Sink &rhs) const
{
	return _knownSink == rhs._knownSink && _cookie == rhs._cookie;
}

BEGIN_INTERFACES(ff::ConnectionPointBase)
	HAS_INTERFACE(IConnectionPoint)
	HAS_INTERFACE(ff::IConnectionPointBase)
END_INTERFACES()

ff::ConnectionPointBase::ConnectionPointBase()
	: _parent(nullptr)
{
}

ff::ConnectionPointBase::~ConnectionPointBase()
{
	DestroySink();

	assert(!_sinks || _sinks->IsEmpty());
}

HRESULT ff::ConnectionPointBase::GetConnectionPointContainer(IConnectionPointContainer **value)
{
	assertRetVal(value, E_INVALIDARG);
	assertRetVal(_parent, E_FAIL);
	*value = GetAddRef(_parent);
	return S_OK;
}

static DWORD s_nextCookie = 0;

HRESULT ff::ConnectionPointBase::Advise(IUnknown *sink, DWORD *cookie)
{
	assertRetVal(sink && cookie, false);

	GUID iid = GUID_NULL;
	assertHrRetVal(GetConnectionInterface(&iid), E_FAIL);

	Sink mySink;
	mySink._sink = sink;
	assertHrRetVal(sink->QueryInterface(iid, &mySink._knownSink), CONNECT_E_CANNOTCONNECT);
	mySink._sink->Release();
	mySink._cookie = InterlockedIncrement(&s_nextCookie);

	ff::MakeUnshared(_sinks);
	_sinks->Push(mySink);

	*cookie = mySink._cookie;
	return true;
}

HRESULT ff::ConnectionPointBase::Unadvise(DWORD cookie)
{
	ff::MakeUnshared(_sinks);
	for (Sink &sink: *_sinks)
	{
		if (sink._cookie == cookie)
		{
			_sinks->DeleteItem(sink);
			return S_OK;
		}
	}

	return E_POINTER;
}

HRESULT ff::ConnectionPointBase::EnumConnections(IEnumConnections **value)
{
	assertRetVal(value, E_INVALIDARG);
	ff::ComPtr<ConnectionEnum> myValue;
	assertRetVal(ConnectionEnum::Create(_sinks, &myValue), E_FAIL);

	*value = myValue.Detach();
	return S_OK;
}

void ff::ConnectionPointBase::DestroySink()
{
	_parent = nullptr;
	// _sinks = nullptr; // the listeners still need to remove themselves
}

size_t ff::ConnectionPointBase::GetSinkCount() const
{
	return _sinks != nullptr ? _sinks->Size() : 0;
}

IUnknown *ff::ConnectionPointBase::GetSink(size_t index) const
{
	SharedSinksPtr sinks = _sinks;
	size_t count = sinks != nullptr ? sinks->Size() : 0;
	assertRetVal(index < count, nullptr);

	return sinks->GetAt(index)._sink;
}

DWORD ff::ConnectionPointBase::GetCookie(size_t index) const
{
	SharedSinksPtr sinks = _sinks;
	size_t count = sinks != nullptr ? sinks->Size() : 0;
	assertRetVal(index < count, 0);

	return sinks->GetAt(index)._cookie;
}

bool ff::ConnectionPointBase::Init(IConnectionPointContainer *parent)
{
	assertRetVal(parent, false);
	_parent = parent;

	return true;
}

ff::ConnectionPointBase::SharedSinksPtr ff::ConnectionPointBase::GetSinks() const
{
	return _sinks;
}
