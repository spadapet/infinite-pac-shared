#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"
#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/KeyboardDevice.h"
#include "Input/PointerDevice.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "String/StringUtil.h"

static ff::StaticString PROP_DATA(L"data");

namespace ff
{
	class __declspec(uuid("d1dee57f-337b-49d6-9608-c69bbbce4367"))
		InputMapping
			: public ComBase
			, public IInputMapping
			, public IResourceSave
	{
	public:
		DECLARE_HEADER(InputMapping);

		bool Init(IInputDevice **ppDevices, size_t nDevices);

		// IInputMapping

		virtual void Advance(double deltaTime) override;
		virtual void Reset() override;
		virtual bool Clone(IInputDevice **devices, size_t deviceCount, IInputMapping **obj) override;

		virtual hash_t NameToId(StringRef name) const override;
		virtual bool MapEvents(const InputEventMapping *pMappings, size_t nCount) override;
		virtual bool MapValues(const InputValueMapping *pMappings, size_t nCount) override;

		virtual Vector<InputEventMapping> GetMappedEvents() const override;
		virtual Vector<InputValueMapping> GetMappedValues() const override;

		virtual Vector<InputEventMapping> GetMappedEvents(hash_t eventID) const override;
		virtual Vector<InputValueMapping> GetMappedValues(hash_t valueID) const override;

		virtual const Vector<InputEvent> &GetEvents() const override;
		virtual float GetEventProgress(hash_t eventID) const override;
		virtual void ClearEvents() override;
		virtual bool HasStartEvent(hash_t eventID) const override;

		virtual int GetDigitalValue(hash_t valueID) const override;
		virtual float GetAnalogValue(hash_t valueID) const override;
		virtual String GetStringValue(hash_t valueID) const override;

		virtual void AddListener(IInputEventListener *pListener) override;
		virtual bool AddProxyListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy) override;
		virtual bool RemoveListener(IInputEventListener *pListener) override;

		// IResourceSave
		virtual bool LoadResource(const ff::Dict &dict) override;
		virtual bool SaveResource(ff::Dict &dict) override;

	private:
		int GetDigitalValue(const InputAction &action, int *pPressCount) const;
		float GetAnalogValue(const InputAction &action, bool bForDigital) const;
		String GetStringValue(const InputAction &action) const;

		struct InputEventMappingInfo : public InputEventMapping
		{
			double _holdingSeconds;
			int _eventCount;
			bool _holding;
		};

		void PushStartEvent(InputEventMappingInfo &info);
		void PushStopEvent(InputEventMappingInfo &info);

		// Raw user input
		Vector<ComPtr<IKeyboardDevice>> _keys;
		Vector<ComPtr<IPointerDevice>> _mice;
		Vector<ComPtr<IJoystickDevice>> _joys;

		Vector<InputEventMappingInfo> _eventMappings;
		Vector<InputValueMapping> _valueMappings;
		Vector<InputEvent> _currentEvents;

		Map<hash_t, size_t> _eventToInfo;
		Map<hash_t, size_t> _valueToInfo;

		typedef Vector<ComPtr<IInputEventListener>> ListenerVector;
		typedef std::shared_ptr<ListenerVector> ListenerVectorPtr;
		ListenerVectorPtr _listeners;
	};

	class __declspec(uuid("d3bdf49d-1e6b-43ed-95d2-99f924170bfa"))
	CProxyInputEventListener : public ComBase, public IProxyInputEventListener
	{
	public:
		DECLARE_HEADER(CProxyInputEventListener);

		// IProxyInputEventListener
		virtual void SetOwner(IInputEventListener *pOwner) override;

		// IInputEventListener
		virtual void OnInputEvents(IInputMapping *pMapping, const InputEvent *pEvents, size_t nCount) override;

	private:
		IInputEventListener *_owner;
	};
}

BEGIN_INTERFACES(ff::InputMapping)
	HAS_INTERFACE(ff::IInputMapping)
	HAS_INTERFACE(ff::IResourceLoad)
	HAS_INTERFACE(ff::IResourceSave)
END_INTERFACES()

BEGIN_INTERFACES(ff::CProxyInputEventListener)
	HAS_INTERFACE(ff::IInputEventListener)
	HAS_INTERFACE(ff::IProxyInputEventListener)
END_INTERFACES()

bool ff::CreateInputMapping(IInputDevice **ppDevices, size_t nDevices, IInputMapping **ppMapping)
{
	assertRetVal(ppMapping, false);
	*ppMapping = nullptr;

	ComPtr<InputMapping, IInputMapping> pMapping;
	assertHrRetVal(ComAllocator<InputMapping>::CreateInstance(&pMapping), false);
	assertRetVal(pMapping->Init(ppDevices, nDevices), false);

	*ppMapping = pMapping.Detach();
	return true;
}

bool ff::CreateProxyInputEventListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy)
{
	assertRetVal(pListener && ppProxy, false);

	ComPtr<CProxyInputEventListener> pProxy;
	assertHrRetVal(ComAllocator<CProxyInputEventListener>::CreateInstance(&pProxy), false);
	pProxy->SetOwner(pListener);

	*ppProxy = pProxy.Detach();
	return true;
}

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"input");
	module.RegisterClassT<ff::InputMapping>(name, __uuidof(ff::IInputMapping));
});

ff::CProxyInputEventListener::CProxyInputEventListener()
	: _owner(nullptr)
{
}

ff::CProxyInputEventListener::~CProxyInputEventListener()
{
	assert(!_owner);
}

void ff::CProxyInputEventListener::SetOwner(IInputEventListener *pOwner)
{
	_owner = pOwner;
}

void ff::CProxyInputEventListener::OnInputEvents(IInputMapping *pMapping, const InputEvent *pEvents, size_t nCount)
{
	if (_owner)
	{
		_owner->OnInputEvents(pMapping, pEvents, nCount);
	}
}

ff::InputMapping::InputMapping()
{
}

ff::InputMapping::~InputMapping()
{
}

bool ff::InputMapping::Init(IInputDevice **ppDevices, size_t nDevices)
{
	assertRetVal(!nDevices || ppDevices, false);

	for (size_t i = 0; i < nDevices; i++)
	{
		ComPtr<IKeyboardDevice> pKey;
		ComPtr<IPointerDevice> pMouse;
		ComPtr<IJoystickDevice> pJoy;

		if (pKey.QueryFrom(ppDevices[i]))
		{
			_keys.Push(pKey);
		}

		if (pMouse.QueryFrom(ppDevices[i]))
		{
			_mice.Push(pMouse);
		}

		if (pJoy.QueryFrom(ppDevices[i]))
		{
			_joys.Push(pJoy);
		}

		assert(pKey || pMouse || pJoy);
	}

	return true;
}

void ff::InputMapping::Advance(double deltaTime)
{
	ClearEvents();

	for (size_t i = 0; i < _eventMappings.Size(); i++)
	{
		InputEventMappingInfo &info = _eventMappings[i];

		bool bStillHolding = true;
		int nTriggerCount = 0;

		// Check each required button that needs to be pressed to trigger the current action
		for (size_t h = 0; h < _countof(info._actions) && info._actions[h]._device != INPUT_DEVICE_NULL; h++)
		{
			int nCurTriggerCount = 0;
			int nCurValue = GetDigitalValue(info._actions[h], &nCurTriggerCount);

			if (!nCurValue)
			{
				bStillHolding = false;
				nTriggerCount = 0;
			}

			if (bStillHolding && nCurTriggerCount)
			{
				nTriggerCount = std::max(nTriggerCount, nCurTriggerCount);
			}
		}

		for (int h = 0; h < nTriggerCount; h++)
		{
			if (info._eventCount > 0)
			{
				PushStopEvent(info);
			}

			info._holding = true;

			if (deltaTime / nTriggerCount >= info._holdSeconds)
			{
				PushStartEvent(info);
			}
		}

		if (info._holding)
		{
			if (!bStillHolding)
			{
				PushStopEvent(info);
			}
			else if (!nTriggerCount)
			{
				info._holdingSeconds += deltaTime;

				double holdTime = (info._holdingSeconds - info._holdSeconds);

				if (holdTime >= 0)
				{
					int totalEvents = 1;

					if (info._repeatSeconds > 0)
					{
						totalEvents += (int)floor(holdTime / info._repeatSeconds);
					}

					while (info._eventCount < totalEvents)
					{
						PushStartEvent(info);
					}
				}
			}
		}
	}

	if (_currentEvents.Size())
	{
#if !METRO_APP
		// The user is doing something, tell Windows not to go to sleep (mostly for joysticks)
		SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
#endif
		// Tell listeners about the input events
		ListenerVectorPtr listeners = _listeners;
		if (listeners != nullptr)
		{
			for (const auto &listener: *listeners.get())
			{
				listener->OnInputEvents(this, _currentEvents.Data(), _currentEvents.Size());
			}
		}
	}
}

void ff::InputMapping::Reset()
{
	ClearEvents();

	_eventMappings.Clear();
	_valueMappings.Clear();
	_eventToInfo.Clear();
	_valueToInfo.Clear();
}

bool ff::InputMapping::Clone(IInputDevice **devices, size_t deviceCount, IInputMapping **obj)
{
	assertRetVal(obj, false);

	ComPtr<IInputMapping> myObj;
	assertRetVal(ff::CreateInputMapping(devices, deviceCount, &myObj), false);

	Vector<InputEventMapping> events = GetMappedEvents();
	Vector<InputValueMapping> values = GetMappedValues();

	if (events.Size())
	{
		assertRetVal(myObj->MapEvents(events.Data(), events.Size()), false);
	}

	if (values.Size())
	{
		assertRetVal(myObj->MapValues(values.Data(), values.Size()), false);
	}

	*obj = myObj.Detach();
	return true;
}

ff::hash_t ff::InputMapping::NameToId(StringRef name) const
{
	return ff::HashFunc(name);
}

bool ff::InputMapping::MapEvents(const InputEventMapping *pMappings, size_t nCount)
{
	bool status = true;

	_eventMappings.Reserve(_eventMappings.Size() + nCount);

	for (size_t i = 0; i < nCount; i++)
	{
		InputEventMappingInfo info;
		ZeroObject(info);

		::CopyMemory(&info, &pMappings[i], sizeof(pMappings[i]));

		_eventMappings.Push(info);
		_eventToInfo.Insert(info._eventID, _eventMappings.Size() - 1);
	}

	return status;
}

bool ff::InputMapping::MapValues(const InputValueMapping *pMappings, size_t nCount)
{
	bool status = true;

	_valueMappings.Reserve(_valueMappings.Size() + nCount);

	for (size_t i = 0; i < nCount; i++)
	{
		_valueMappings.Push(pMappings[i]);
		_valueToInfo.Insert(pMappings[i]._valueID, _valueMappings.Size() - 1);
	}

	return status;
}

ff::Vector<ff::InputEventMapping> ff::InputMapping::GetMappedEvents() const
{
	Vector<InputEventMapping> mappings;
	mappings.Reserve(_eventMappings.Size());

	for (size_t i = 0; i < _eventMappings.Size(); i++)
	{
		mappings.Push(_eventMappings[i]);
	}

	return mappings;
}

ff::Vector<ff::InputValueMapping> ff::InputMapping::GetMappedValues() const
{
	return _valueMappings;
}

ff::Vector<ff::InputEventMapping> ff::InputMapping::GetMappedEvents(hash_t eventID) const
{
	Vector<InputEventMapping> mappings;

	for (BucketIter i = _eventToInfo.Get(eventID); i != INVALID_ITER; i = _eventToInfo.GetNext(i))
	{
		mappings.Push(_eventMappings[_eventToInfo.ValueAt(i)]);
	}

	return mappings;
}

ff::Vector<ff::InputValueMapping> ff::InputMapping::GetMappedValues(hash_t valueID) const
{
	Vector<InputValueMapping> mappings;

	for (BucketIter i = _valueToInfo.Get(valueID); i != INVALID_ITER; i = _valueToInfo.GetNext(i))
	{
		mappings.Push(_valueMappings[_valueToInfo.ValueAt(i)]);
	}

	return mappings;
}

const ff::Vector<ff::InputEvent> &ff::InputMapping::GetEvents() const
{
	return _currentEvents;
}

float ff::InputMapping::GetEventProgress(hash_t eventID) const
{
	double ret = 0;

	for (BucketIter i = _eventToInfo.Get(eventID); i != INVALID_ITER; i = _eventToInfo.GetNext(i))
	{
		const InputEventMappingInfo &info = _eventMappings[_eventToInfo.ValueAt(i)];

		if (info._holding)
		{
			double val = 1;

			if (info._holdingSeconds < info._holdSeconds)
			{
				// from 0 to 1
				val = info._holdingSeconds / info._holdSeconds;
			}
			else if (info._repeatSeconds > 0)
			{
				// from 1 to N, using repeat count
				val = (info._holdingSeconds - info._holdSeconds) / info._repeatSeconds + 1.0;
			}
			else if (info._holdSeconds > 0)
			{
				// from 1 to N, using the original hold time as the repeat time
				val = info._holdingSeconds / info._holdSeconds;
			}

			// Can only return one value, so choose the largest
			if (val > ret)
			{
				ret = val;
			}
		}
	}

	return (float)ret;
}

void ff::InputMapping::ClearEvents()
{
	_currentEvents.Clear();
}

bool ff::InputMapping::HasStartEvent(hash_t eventID) const
{
	for (const ff::InputEvent &event : _currentEvents)
	{
		if (event._eventID == eventID && event.IsStart())
		{
			return true;
		}
	}

	return false;
}

int ff::InputMapping::GetDigitalValue(hash_t valueID) const
{
	int ret = 0;

	for (BucketIter i = _valueToInfo.Get(valueID); i != INVALID_ITER; i = _valueToInfo.GetNext(i))
	{
		const InputAction &action = _valueMappings[_valueToInfo.ValueAt(i)]._action;

		int val = GetDigitalValue(action, nullptr);

		// Can only return one value, so choose the largest
		if (abs(val) > abs(ret))
		{
			ret = val;
		}
	}

	return ret;
}

float ff::InputMapping::GetAnalogValue(hash_t valueID) const
{
	float ret = 0;

	for (BucketIter i = _valueToInfo.Get(valueID); i != INVALID_ITER; i = _valueToInfo.GetNext(i))
	{
		const InputAction &action = _valueMappings[_valueToInfo.ValueAt(i)]._action;

		float val = GetAnalogValue(action, false);

		// Can only return one value, so choose the largest
		if (fabsf(val) > fabsf(ret))
		{
			ret = val;
		}
	}

	return ret;
}

ff::String ff::InputMapping::GetStringValue(hash_t valueID) const
{
	String ret;

	for (BucketIter i = _valueToInfo.Get(valueID); i != INVALID_ITER; i = _valueToInfo.GetNext(i))
	{
		const InputAction &action = _valueMappings[_valueToInfo.ValueAt(i)]._action;

		ret += GetStringValue(action);
	}

	return ret;
}

void ff::InputMapping::AddListener(IInputEventListener *pListener)
{
	assertRet(pListener);
	ff::MakeUnshared(_listeners);
	_listeners->Push(pListener);
}

bool ff::InputMapping::AddProxyListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy)
{
	assertRetVal(CreateProxyInputEventListener(pListener, ppProxy), false);
	AddListener(*ppProxy);

	return true;
}

bool ff::InputMapping::RemoveListener(IInputEventListener *pListener)
{
	assertRetVal(pListener, false);

	ff::MakeUnshared(_listeners);
	if (_listeners != nullptr)
	{
		for (const auto &listener: *_listeners.get())
		{
			if (listener == pListener)
			{
				_listeners->DeleteItem(listener);
				return true;
			}
		}
	}

	assertRetVal(false, false);
}

bool ff::InputMapping::LoadResource(const ff::Dict &dict)
{
	ComPtr<IData> data = dict.GetData(PROP_DATA);
	noAssertRetVal(data, true);

	ComPtr<IDataReader> reader;
	assertRetVal(CreateDataReader(data, 0, &reader), false);

	DWORD version = 0;
	assertRetVal(LoadData(reader, version), false);
	assertRetVal(version == 0, false);

	DWORD size = 0;
	assertRetVal(LoadData(reader, size), false);

	if (size)
	{
		Vector<InputEventMapping> mappings;
		mappings.Resize(size);

		assertRetVal(LoadBytes(reader, mappings.Data(), mappings.ByteSize()), false);
		assertRetVal(MapEvents(mappings.Data(), mappings.Size()), false);
	}

	assertRetVal(LoadData(reader, size), false);

	if (size)
	{
		Vector<InputValueMapping> mappings;
		mappings.Resize(size);

		assertRetVal(LoadBytes(reader, mappings.Data(), mappings.ByteSize()), false);
		assertRetVal(MapValues(mappings.Data(), mappings.Size()), false);
	}

	return true;
}

bool ff::InputMapping::SaveResource(ff::Dict &dict)
{
	ComPtr<IDataVector> data;
	ComPtr<IDataWriter> writer;
	assertRetVal(CreateDataWriter(&data, &writer), false);

	DWORD version = 0;
	assertRetVal(SaveData(writer, version), false);

	DWORD size = (DWORD)_eventMappings.Size();
	assertRetVal(SaveData(writer, size), false);

	for (size_t i = 0; i < size; i++)
	{
		InputEventMapping info = _eventMappings[i];
		assertRetVal(SaveData(writer, info), false);
	}

	size = (DWORD)_valueMappings.Size();
	assertRetVal(SaveData(writer, size), false);

	if (size)
	{
		assertRetVal(SaveBytes(writer, _valueMappings.Data(), _valueMappings.ByteSize()), false);
	}

	dict.SetData(PROP_DATA, data);

	return true;
}

int ff::InputMapping::GetDigitalValue(const InputAction &action, int *pPressCount) const
{
	int nFakePressCount;
	pPressCount = pPressCount ? pPressCount : &nFakePressCount;
	*pPressCount = 0;

	switch (action._device)
	{
	case INPUT_DEVICE_KEYBOARD:
		if (action._part == INPUT_PART_BUTTON && action._partValue == INPUT_VALUE_PRESSED)
		{
			int nRet = 0;

			for (size_t i = 0; i < _keys.Size(); i++)
			{
				*pPressCount += _keys[i]->GetKeyPressCount(action._partIndex);

				if (_keys[i]->GetKey(action._partIndex))
				{
					nRet = 1;
				}
			}

			return nRet;
		}
		else if (action._part == INPUT_PART_TEXT && action._partValue == INPUT_VALUE_TEXT)
		{
			for (size_t i = 0; i < _keys.Size(); i++)
			{
				if (_keys[i]->GetChars().size())
				{
					*pPressCount = 1;
					return 1;
				}
			}
		}
		break;

	case INPUT_DEVICE_MOUSE:
		if (action._part == INPUT_PART_BUTTON && action._partValue == INPUT_VALUE_PRESSED)
		{
			int nRet = 0;

			for (size_t i = 0; i < _mice.Size(); i++)
			{
				*pPressCount += _mice[i]->GetButtonClickCount(action._partIndex);

				if (_mice[i]->GetButton(action._partIndex))
				{
					nRet = 1;
				}
			}

			return nRet;
		}
		break;

	case INPUT_DEVICE_JOYSTICK:
		if (action._part == INPUT_PART_BUTTON && action._partValue == INPUT_VALUE_PRESSED)
		{
			int nRet = 0;

			for (size_t i = 0; i < _joys.Size(); i++)
			{
				if (_joys[i]->IsConnected())
				{
					size_t nButtonCount = _joys[i]->GetButtonCount();
					bool bAllButtons = (action._partIndex == INPUT_INDEX_ANY_BUTTON);

					if (bAllButtons || action._partIndex < nButtonCount)
					{
						size_t nFirst = bAllButtons ? 0 : action._partIndex;
						size_t nEnd = bAllButtons ? nButtonCount : nFirst + 1;

						for (size_t h = nFirst; h < nEnd; h++)
						{
							*pPressCount += _joys[i]->GetButtonPressCount(h);

							if (_joys[i]->GetButton(h))
							{
								nRet = 1;
							}
						}
					}
				}
			}

			return nRet;
		}
		else if (action._part == INPUT_PART_SPECIAL_BUTTON && action._partValue == INPUT_VALUE_PRESSED)
		{
			int nRet = 0;
			ESpecialJoystickButton type = (ESpecialJoystickButton)action._partIndex;

			for (size_t i = 0; i < _joys.Size(); i++)
			{
				if (_joys[i]->IsConnected() && _joys[i]->HasSpecialButton(type))
				{
					*pPressCount += _joys[i]->GetSpecialButtonPressCount(type);

					if (_joys[i]->GetSpecialButton(type))
					{
						nRet = 1;
					}
				}
			}

			return nRet;
		}
		else if (action._part == INPUT_PART_TRIGGER)
		{
			if (action._partValue == INPUT_VALUE_PRESSED)
			{
				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetTriggerCount())
					{
						*pPressCount += _joys[i]->GetTriggerPressCount(action._partIndex);
					}
				}
			}

			return GetAnalogValue(action, true) >= 0.5f ? 1 : 0;
		}
		else if (action._part == INPUT_PART_STICK)
		{
			if (action._partValue == INPUT_VALUE_LEFT ||
				action._partValue == INPUT_VALUE_RIGHT ||
				action._partValue == INPUT_VALUE_UP ||
				action._partValue == INPUT_VALUE_DOWN)
			{
				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetStickCount())
					{
						RectInt dirs = _joys[i]->GetStickPressCount(action._partIndex);

						switch (action._partValue)
						{
						case INPUT_VALUE_LEFT: *pPressCount += dirs.left; break;
						case INPUT_VALUE_RIGHT: *pPressCount += dirs.right; break;
						case INPUT_VALUE_UP: *pPressCount += dirs.top; break;
						case INPUT_VALUE_DOWN: *pPressCount += dirs.bottom; break;
						}
					}
				}
			}

			return GetAnalogValue(action, true) >= 0.5f ? 1 : 0;
		}
		else if (action._part == INPUT_PART_DPAD)
		{
			if (action._partValue == INPUT_VALUE_LEFT ||
				action._partValue == INPUT_VALUE_RIGHT ||
				action._partValue == INPUT_VALUE_UP ||
				action._partValue == INPUT_VALUE_DOWN)
			{
				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetDPadCount())
					{
						RectInt dirs = _joys[i]->GetDPadPressCount(action._partIndex);

						switch (action._partValue)
						{
						case INPUT_VALUE_LEFT: *pPressCount += dirs.left; break;
						case INPUT_VALUE_RIGHT: *pPressCount += dirs.right; break;
						case INPUT_VALUE_UP: *pPressCount += dirs.top; break;
						case INPUT_VALUE_DOWN: *pPressCount += dirs.bottom; break;
						}
					}
				}
			}

			return GetAnalogValue(action, true) >= 0.5f ? 1 : 0;
		}
		break;
	}

	return 0;
}

float ff::InputMapping::GetAnalogValue(const InputAction &action, bool bForDigital) const
{
	switch (action._device)
	{
	case INPUT_DEVICE_KEYBOARD:
	case INPUT_DEVICE_MOUSE:
		return GetDigitalValue(action, nullptr) ? 1.0f : 0.0f;

	case INPUT_DEVICE_JOYSTICK:
		if (action._part == INPUT_PART_BUTTON)
		{
			return GetDigitalValue(action, nullptr) ? 1.0f : 0.0f;
		}
		else if (action._part == INPUT_PART_TRIGGER && action._partValue == INPUT_VALUE_PRESSED)
		{
			float val = 0;

			for (size_t i = 0; i < _joys.Size(); i++)
			{
				if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetTriggerCount())
				{
					float newVal = _joys[i]->GetTrigger(action._partIndex, bForDigital);

					if (fabsf(newVal) > fabsf(val))
					{
						val = newVal;
					}
				}
			}

			return val;
		}
		else if (action._part == INPUT_PART_STICK)
		{
			if (action._partValue == INPUT_VALUE_X_AXIS ||
				action._partValue == INPUT_VALUE_Y_AXIS ||
				action._partValue == INPUT_VALUE_LEFT ||
				action._partValue == INPUT_VALUE_RIGHT ||
				action._partValue == INPUT_VALUE_UP ||
				action._partValue == INPUT_VALUE_DOWN)
			{
				PointFloat posXY(0, 0);

				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetStickCount())
					{
						PointFloat newXY = _joys[i]->GetStickPos(action._partIndex, bForDigital);

						if (fabsf(newXY.x) > fabsf(posXY.x))
						{
							posXY.x = newXY.x;
						}

						if (fabsf(newXY.y) > fabsf(posXY.y))
						{
							posXY.y = newXY.y;
						}
					}
				}

				switch (action._partValue)
				{
				case INPUT_VALUE_X_AXIS: return posXY.x;
				case INPUT_VALUE_Y_AXIS: return posXY.y;
				case INPUT_VALUE_LEFT: return (posXY.x < 0) ? -posXY.x : 0.0f;
				case INPUT_VALUE_RIGHT: return (posXY.x > 0) ? posXY.x : 0.0f;
				case INPUT_VALUE_UP: return (posXY.y < 0) ? -posXY.y : 0.0f;
				case INPUT_VALUE_DOWN: return (posXY.y > 0) ? posXY.y : 0.0f;
				}
			}
		}
		else if (action._part == INPUT_PART_DPAD)
		{
			if (action._partValue == INPUT_VALUE_X_AXIS ||
				action._partValue == INPUT_VALUE_Y_AXIS ||
				action._partValue == INPUT_VALUE_LEFT ||
				action._partValue == INPUT_VALUE_RIGHT ||
				action._partValue == INPUT_VALUE_UP ||
				action._partValue == INPUT_VALUE_DOWN)
			{
				PointInt posXY(0, 0);

				for (size_t i = 0; i < _joys.Size(); i++)
				{
					if (_joys[i]->IsConnected() && action._partIndex < _joys[i]->GetDPadCount())
					{
						PointInt newXY = _joys[i]->GetDPadPos(action._partIndex);

						if (abs(newXY.x) > abs(posXY.x))
						{
							posXY.x = newXY.x;
						}

						if (abs(newXY.y) > abs(posXY.y))
						{
							posXY.y = newXY.y;
						}
					}
				}

				switch (action._partValue)
				{
				case INPUT_VALUE_X_AXIS: return (float)posXY.x;
				case INPUT_VALUE_Y_AXIS: return (float)posXY.y;
				case INPUT_VALUE_LEFT: return (posXY.x < 0) ? 1.0f : 0.0f;
				case INPUT_VALUE_RIGHT: return (posXY.x > 0) ? 1.0f : 0.0f;
				case INPUT_VALUE_UP: return (posXY.y < 0) ? 1.0f : 0.0f;
				case INPUT_VALUE_DOWN: return (posXY.y > 0) ? 1.0f : 0.0f;
				}
			}
		}
		break;
	}

	return 0;
}

ff::String ff::InputMapping::GetStringValue(const InputAction &action) const
{
	String szText;

	if (action._device == INPUT_DEVICE_KEYBOARD && action._part == INPUT_PART_TEXT)
	{
		for (size_t i = 0; i < _keys.Size(); i++)
		{
			if (szText.empty())
			{
				szText = _keys[i]->GetChars();
			}
			else
			{
				szText += _keys[i]->GetChars();
			}
		}

		return szText;
	}

	return szText;
}

void ff::InputMapping::PushStartEvent(InputEventMappingInfo &info)
{
	InputEvent ev;
	ev._eventID = info._eventID;
	ev._count = ++info._eventCount;

	_currentEvents.Push(ev);
}

void ff::InputMapping::PushStopEvent(InputEventMappingInfo &info)
{
	bool bEvent = (info._eventCount > 0);

	info._holdingSeconds = 0;
	info._eventCount = 0;
	info._holding = false;

	if (bEvent)
	{
		InputEvent ev;
		ev._eventID = info._eventID;
		ev._count = 0;

		_currentEvents.Push(ev);
	}
}

// Big static table of all virtual keys
struct VirtualKeyInfo
{
	int vk;
	ff::StaticString name;
	ff::InputAction action;
};

static const VirtualKeyInfo s_vksRaw[] =
{
	{ VK_LBUTTON, { L"mouse-left" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_LBUTTON } },
	{ VK_RBUTTON, { L"mouse-right" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_RBUTTON } },
	{ VK_MBUTTON, { L"mouse-middle" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_MBUTTON } },
	{ VK_XBUTTON1, { L"mouse-x1" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_XBUTTON1 } },
	{ VK_XBUTTON2, { L"mouse-x2" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_XBUTTON2 } },
	{ VK_BACK, { L"backspace" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_BACK } },
	{ VK_TAB, { L"tab" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_TAB } },
	{ VK_RETURN, { L"return" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_RETURN } },
	{ VK_SHIFT, { L"shift" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_SHIFT } },
	{ VK_CONTROL, { L"control" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_CONTROL } },
	{ VK_MENU, { L"alt" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_MENU } },
	{ VK_ESCAPE, { L"escape" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_ESCAPE } },
	{ VK_SPACE, { L"space" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_SPACE } },
	{ VK_PRIOR, { L"pageup" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_PRIOR } },
	{ VK_NEXT, { L"pagedown" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NEXT } },
	{ VK_END, { L"end" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_END } },
	{ VK_HOME, { L"home" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_HOME } },
	{ VK_LEFT, { L"left" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_LEFT } },
	{ VK_UP, { L"up" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_UP } },
	{ VK_RIGHT, { L"right" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_RIGHT } },
	{ VK_DOWN, { L"down" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_DOWN } },
	{ VK_SNAPSHOT, { L"printscreen" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_SNAPSHOT } },
	{ VK_INSERT, { L"insert" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_INSERT } },
	{ VK_DELETE, { L"delete" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_DELETE } },
	{ '0', { L"0" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '0' } },
	{ '1', { L"1" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '1' } },
	{ '2', { L"2" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '2' } },
	{ '3', { L"3" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '3' } },
	{ '4', { L"4" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '4' } },
	{ '5', { L"5" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '5' } },
	{ '6', { L"6" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '6' } },
	{ '7', { L"7" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '7' } },
	{ '8', { L"8" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '8' } },
	{ '9', { L"9" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, '9' } },
	{ 'A', { L"a" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'A' } },
	{ 'B', { L"b" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'B' } },
	{ 'C', { L"c" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'C' } },
	{ 'D', { L"d" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'D' } },
	{ 'E', { L"e" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'E' } },
	{ 'F', { L"f" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'F' } },
	{ 'G', { L"g" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'G' } },
	{ 'H', { L"h" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'H' } },
	{ 'I', { L"i" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'I' } },
	{ 'J', { L"j" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'J' } },
	{ 'K', { L"k" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'K' } },
	{ 'L', { L"l" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'L' } },
	{ 'M', { L"m" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'M' } },
	{ 'N', { L"n" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'N' } },
	{ 'O', { L"o" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'O' } },
	{ 'P', { L"p" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'P' } },
	{ 'Q', { L"q" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'Q' } },
	{ 'R', { L"r" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'R' } },
	{ 'S', { L"s" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'S' } },
	{ 'T', { L"t" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'T' } },
	{ 'U', { L"u" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'U' } },
	{ 'V', { L"v" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'V' } },
	{ 'W', { L"w" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'W' } },
	{ 'X', { L"x" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'X' } },
	{ 'Y', { L"y" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'Y' } },
	{ 'Z', { L"z" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, 'Z' } },
	{ VK_LWIN, { L"lwin" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_LWIN } },
	{ VK_RWIN, { L"rwin" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_RWIN } },
	{ VK_NUMPAD0, { L"num0" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD0 } },
	{ VK_NUMPAD1, { L"num1" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD1 } },
	{ VK_NUMPAD2, { L"num2" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD2 } },
	{ VK_NUMPAD3, { L"num3" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD3 } },
	{ VK_NUMPAD4, { L"num4" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD4 } },
	{ VK_NUMPAD5, { L"num5" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD5 } },
	{ VK_NUMPAD6, { L"num6" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD6 } },
	{ VK_NUMPAD7, { L"num7" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD7 } },
	{ VK_NUMPAD8, { L"num8" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD8 } },
	{ VK_NUMPAD9, { L"num9" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_NUMPAD9 } },
	{ VK_MULTIPLY, { L"multiply" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_MULTIPLY } },
	{ VK_ADD, { L"add" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_ADD } },
	{ VK_SUBTRACT, { L"subtract" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_SUBTRACT } },
	{ VK_DECIMAL, { L"decimal" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_DECIMAL } },
	{ VK_DIVIDE, { L"divide" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_DIVIDE } },
	{ VK_F1, { L"f1" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F1 } },
	{ VK_F2, { L"f2" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F2 } },
	{ VK_F3, { L"f3" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F3 } },
	{ VK_F4, { L"f4" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F4 } },
	{ VK_F5, { L"f5" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F5 } },
	{ VK_F6, { L"f6" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F6 } },
	{ VK_F7, { L"f7" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F7 } },
	{ VK_F8, { L"f8" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F8 } },
	{ VK_F9, { L"f9" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F9 } },
	{ VK_F10, { L"f10" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F10 } },
	{ VK_F11, { L"f11" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F11 } },
	{ VK_F12, { L"f12" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_F12 } },
	{ VK_LSHIFT, { L"lshift" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_LSHIFT } },
	{ VK_RSHIFT, { L"rshift" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_RSHIFT } },
	{ VK_LCONTROL, { L"lcontrol" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_LCONTROL } },
	{ VK_RCONTROL, { L"rcontrol" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_RCONTROL } },
	{ VK_LMENU, { L"lalt" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_LMENU } },
	{ VK_RMENU, { L"ralt" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_RMENU } },
	{ VK_OEM_1, { L"colon" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_1 } },
	{ VK_OEM_PLUS, { L"plus" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_PLUS } },
	{ VK_OEM_COMMA, { L"comma" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_COMMA } },
	{ VK_OEM_MINUS, { L"minus" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_MINUS } },
	{ VK_OEM_PERIOD, { L"period" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_PERIOD } },
	{ VK_OEM_2, { L"question" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_2 } },
	{ VK_OEM_3, { L"tilde" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_3 } },
	{ VK_GAMEPAD_A, { L"joy-a" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_A } },
	{ VK_GAMEPAD_B, { L"joy-b" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_B } },
	{ VK_GAMEPAD_X, { L"joy-x" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_X } },
	{ VK_GAMEPAD_Y, { L"joy-y" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_Y } },
	{ VK_GAMEPAD_RIGHT_SHOULDER, { L"joy-rbumper" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_RIGHT_BUMPER } },
	{ VK_GAMEPAD_LEFT_SHOULDER, { L"joy-lbumper" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_LEFT_BUMPER } },
	{ VK_GAMEPAD_LEFT_TRIGGER, { L"joy-rtrigger" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_TRIGGER, ff::INPUT_VALUE_PRESSED, ff::INPUT_INDEX_LEFT_TRIGGER } },
	{ VK_GAMEPAD_RIGHT_TRIGGER, { L"joy-ltrigger" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_TRIGGER, ff::INPUT_VALUE_PRESSED, ff::INPUT_INDEX_RIGHT_TRIGGER } },
	{ VK_GAMEPAD_DPAD_UP, { L"joy-dup" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_DPAD, ff::INPUT_VALUE_UP, 0 } },
	{ VK_GAMEPAD_DPAD_DOWN, { L"joy-ddown" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_DPAD, ff::INPUT_VALUE_DOWN, 0 } },
	{ VK_GAMEPAD_DPAD_LEFT, { L"joy-dleft" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_DPAD, ff::INPUT_VALUE_LEFT, 0 } },
	{ VK_GAMEPAD_DPAD_RIGHT, { L"joy-dright" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_DPAD, ff::INPUT_VALUE_RIGHT, 0 } },
	{ VK_GAMEPAD_MENU, { L"joy-start" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_START } },
	{ VK_GAMEPAD_VIEW, { L"joy-back" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_BACK } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON, { L"joy-lthumb" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_LEFT_STICK } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON, { L"joy-rthumb" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_SPECIAL_BUTTON, ff::INPUT_VALUE_PRESSED, ff::JOYSTICK_BUTTON_RIGHT_STICK } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_UP, { L"joy-lup" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_UP, ff::INPUT_INDEX_LEFT_STICK } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_DOWN, { L"joy-ldown" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_DOWN, ff::INPUT_INDEX_LEFT_STICK } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT, { L"joy-lright" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_RIGHT, ff::INPUT_INDEX_LEFT_STICK } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_LEFT, { L"joy-lleft" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_LEFT, ff::INPUT_INDEX_LEFT_STICK } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_UP, { L"joy-rup" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_UP, ff::INPUT_INDEX_RIGHT_STICK } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN, { L"joy-rdown" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_DOWN, ff::INPUT_INDEX_RIGHT_STICK } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT, { L"joy-rright" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_RIGHT, ff::INPUT_INDEX_RIGHT_STICK } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT, { L"joy-rleft" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_LEFT, ff::INPUT_INDEX_RIGHT_STICK } },
	{ VK_OEM_4, { L"opencurly" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_4 } },
	{ VK_OEM_5, { L"pipe" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_5 } },
	{ VK_OEM_6, { L"closecurly" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_6 } },
	{ VK_OEM_7, { L"quote" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_PRESSED, VK_OEM_7 } },
};

static bool s_initVirtualKeys;
static const VirtualKeyInfo *s_vkToInfo[0xFF];
static ff::Map<ff::String, const VirtualKeyInfo *> s_nameToInfo;
static ff::Map<ff::InputAction, const VirtualKeyInfo *> s_actionToInfo;

static void InitVirtualKeyInfo()
{
	if (!s_initVirtualKeys)
	{
		ff::LockMutex lock(ff::GCS_INPUT_MAPPING);

		if (!s_initVirtualKeys)
		{
			for (const VirtualKeyInfo &info : s_vksRaw)
			{
				assert(info.vk > 0 && info.vk < _countof(s_vkToInfo));
				assert(info.name.GetString().size());
				assert(!s_vkToInfo[info.vk]);
				assert(!s_nameToInfo.Exists(info.name.GetString()));
				assert(!s_actionToInfo.Exists(info.action));

				s_vkToInfo[info.vk] = &info;
				s_nameToInfo.SetKey(info.name.GetString(), &info);
				s_actionToInfo.SetKey(info.action, &info);

				ff::AtProgramShutdown([]()
				{
					ff::ZeroObject(s_vkToInfo);
					s_nameToInfo.Clear();
					s_actionToInfo.Clear();
					s_initVirtualKeys = false;
				});

				s_initVirtualKeys = true;
			}
		}
	}
}

int ff::NameToVirtualKey(StringRef name)
{
	InitVirtualKeyInfo();

	ff::BucketIter iter = s_nameToInfo.Get(name);
	if (iter != ff::INVALID_ITER)
	{
		return s_nameToInfo.ValueAt(iter)->vk;
	}

	assertRetVal(false, 0);
}

ff::String ff::VirtualKeyToName(int vk)
{
	InitVirtualKeyInfo();

	if (vk > 0 && vk < _countof(s_vkToInfo) && s_vkToInfo[vk])
	{
		return s_vkToInfo[vk]->name.GetString();
	}

	assertRetVal(false, ff::GetEmptyString());
}

ff::String ff::VirtualKeyToDisplayName(int vk)
{
	if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9'))
	{
		return ff::String(1, (wchar_t)vk);
	}

	ff::String name = ff::VirtualKeyToName(vk);

	ff::String id = name;
	ff::UpperCaseInPlace(id);
	ff::ReplaceAll(id, '-', '_');
	id.insert(0, L"VK_");

	ff::String displayName = ff::GetThisModule().GetString(id);
	assert(displayName.size());

	return displayName.empty() ? name : displayName;
}

ff::InputAction ff::VirtualKeyToInputAction(int vk)
{
	InitVirtualKeyInfo();

	if (vk > 0 && vk < _countof(s_vkToInfo) && s_vkToInfo[vk])
	{
		return s_vkToInfo[vk]->action;
	}

	InputAction action;
	ff::ZeroObject(action);
	assertRetVal(false, action);
}

ff::InputAction ff::NameToInputAction(StringRef name)
{
	int vk = NameToVirtualKey(name);
	return VirtualKeyToInputAction(vk);
}

int ff::InputActionToVirtualKey(const InputAction &action)
{
	InitVirtualKeyInfo();

	ff::BucketIter iter = s_actionToInfo.Get(action);
	if (iter != ff::INVALID_ITER)
	{
		return s_actionToInfo.ValueAt(iter)->vk;
	}

	assertRetVal(false, 0);
}

ff::String ff::InputActionToName(const InputAction &action)
{
	int vk = InputActionToVirtualKey(action);
	return VirtualKeyToName(vk);
}

ff::String ff::InputActionToDisplayName(const InputAction &action)
{
	int vk = InputActionToVirtualKey(action);
	return VirtualKeyToDisplayName(vk);
}

bool ff::InputAction::IsValid() const
{
	return _device && _part && _partValue;
}

bool ff::InputAction::operator<(const ff::InputAction &rhs) const
{
	return memcmp(this, &rhs, sizeof(ff::InputAction)) < 0;
}
