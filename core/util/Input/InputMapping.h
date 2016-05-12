#pragma once

#ifndef VK_GAMEPAD_A
#define VK_GAMEPAD_A 0xC3
#define VK_GAMEPAD_B 0xC4
#define VK_GAMEPAD_X 0xC5
#define VK_GAMEPAD_Y 0xC6
#define VK_GAMEPAD_RIGHT_SHOULDER 0xC7
#define VK_GAMEPAD_LEFT_SHOULDER 0xC8
#define VK_GAMEPAD_LEFT_TRIGGER 0xC9
#define VK_GAMEPAD_RIGHT_TRIGGER 0xCA
#define VK_GAMEPAD_DPAD_UP 0xCB
#define VK_GAMEPAD_DPAD_DOWN 0xCC
#define VK_GAMEPAD_DPAD_LEFT 0xCD
#define VK_GAMEPAD_DPAD_RIGHT 0xCE
#define VK_GAMEPAD_MENU 0xCF
#define VK_GAMEPAD_VIEW 0xD0
#define VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON 0xD1
#define VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON 0xD2
#define VK_GAMEPAD_LEFT_THUMBSTICK_UP 0xD3
#define VK_GAMEPAD_LEFT_THUMBSTICK_DOWN 0xD4
#define VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT 0xD5
#define VK_GAMEPAD_LEFT_THUMBSTICK_LEFT 0xD6
#define VK_GAMEPAD_RIGHT_THUMBSTICK_UP 0xD7
#define VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN 0xD8
#define VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT 0xD9
#define VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT 0xDA
#endif

namespace ff
{
	class IData;
	class IInputDevice;
	class IInputEventListener;
	class IProxyInputEventListener;

	const int VK_GAMEPAD_FIRST = VK_GAMEPAD_A;
	const int VK_GAMEPAD_COUNT = VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT - VK_GAMEPAD_A + 1;

	// Which device is being used?
	enum InputDevice : unsigned char
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_DEVICE_NULL,
		INPUT_DEVICE_KEYBOARD,
		INPUT_DEVICE_MOUSE,
		INPUT_DEVICE_JOYSTICK,
	};

	// What is the user touching on the device?
	enum InputPart : unsigned char
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_PART_NULL,
		INPUT_PART_BUTTON, // keyboard, mouse, or joystick
		INPUT_PART_STICK,
		INPUT_PART_DPAD,
		INPUT_PART_TRIGGER,
		INPUT_PART_TEXT, // on keyboard
		INPUT_PART_SPECIAL_BUTTON,
	};

	enum InputPartValue : unsigned char
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_VALUE_NULL,
		INPUT_VALUE_PRESSED,
		INPUT_VALUE_X_AXIS, // to get the value of the joystick axis
		INPUT_VALUE_Y_AXIS,
		INPUT_VALUE_LEFT, // pressing the joystick in a certain direction
		INPUT_VALUE_RIGHT,
		INPUT_VALUE_UP,
		INPUT_VALUE_DOWN,
		INPUT_VALUE_TEXT, // string typed on the keyboard
	};

	enum InputPartIndex : unsigned char
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_INDEX_LEFT_STICK = 0,
		INPUT_INDEX_RIGHT_STICK = 1,

		INPUT_INDEX_LEFT_TRIGGER = 0,
		INPUT_INDEX_RIGHT_TRIGGER = 1,

		INPUT_INDEX_ANY_BUTTON = 0xFF,
	};

	// A single thing that the user can press
	struct InputAction
	{
		UTIL_API bool IsValid() const;
		UTIL_API bool operator<(const InputAction &rhs) const;

		InputDevice _device;
		InputPart _part;
		InputPartValue _partValue;
		BYTE _partIndex; // which button or stick?
	};

	// Maps actions to an event
	struct InputEventMapping
	{
		InputAction _actions[4]; // up to four (unused actions must be zeroed out)
		hash_t _eventID;
		double _holdSeconds;
		double _repeatSeconds;
	};

	// Allows direct access to any input value
	struct InputValueMapping
	{
		InputAction _action;
		hash_t _valueID;
	};

	// Sent whenever an InputEventMapping gets triggered (or released)
	struct InputEvent
	{
		hash_t _eventID;
		int _count;

		bool IsStart() const { return _count == 1; }
		bool IsRepeat() const { return _count > 1; }
		bool IsStop() const { return _count == 0; }
	};

	class __declspec(uuid("53800686-1b66-4256-836a-bdce1b1b2f0b")) __declspec(novtable)
		IInputMapping : public IUnknown
	{
	public:
		virtual void Advance(double deltaTime) = 0;
		virtual void Reset() = 0;
		virtual bool Clone(IInputDevice **devices, size_t deviceCount, IInputMapping **obj);

		virtual hash_t NameToId(StringRef name) const;
		virtual bool MapEvents(const InputEventMapping *pMappings, size_t nCount) = 0;
		virtual bool MapValues(const InputValueMapping *pMappings, size_t nCount) = 0;

		virtual Vector<InputEventMapping> GetMappedEvents() const = 0;
		virtual Vector<InputValueMapping> GetMappedValues() const = 0;

		virtual Vector<InputEventMapping> GetMappedEvents(hash_t eventID) const = 0;
		virtual Vector<InputValueMapping> GetMappedValues(hash_t valueID) const = 0;

		// These must be called between calls to Advance() to find out what happened
		virtual const Vector<InputEvent> &GetEvents() const = 0;
		virtual float GetEventProgress(hash_t eventID) const = 0; // 1=triggered once, 2=hold time hit twice, etc...
		virtual void ClearEvents() = 0;
		virtual bool HasStartEvent(hash_t eventID) const = 0;

		// Gets immediate values
		virtual int GetDigitalValue(hash_t valueID) const = 0;
		virtual float GetAnalogValue(hash_t valueID) const = 0;
		virtual String GetStringValue(hash_t valueID) const = 0;

		// Listeners
		virtual void AddListener(IInputEventListener *pListener) = 0;
		virtual bool AddProxyListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy) = 0;
		virtual bool RemoveListener(IInputEventListener *pListener) = 0;
	};

	UTIL_API bool CreateInputMapping(IInputDevice **ppDevices, size_t nDevices, IInputMapping **ppMapping);

	class __declspec(uuid("7e363dc0-78b6-49e3-bd7a-0dde743e9393")) __declspec(novtable)
		IInputEventListener : public IUnknown
	{
	public:
		virtual void OnInputEvents(IInputMapping *pMapping, const InputEvent *pEvents, size_t nCount) = 0;
	};

	class __declspec(uuid("063acc05-326a-44a6-b9c8-671d84d567be")) __declspec(novtable)
		IProxyInputEventListener : public IInputEventListener
	{
	public:
		// Must call SetOwner(nullptr) when the owner is destroyed
		virtual void SetOwner(IInputEventListener *pOwner) = 0;
	};

	bool CreateProxyInputEventListener(IInputEventListener *pListener, IProxyInputEventListener **ppProxy);

	UTIL_API int NameToVirtualKey(StringRef name);
	UTIL_API String VirtualKeyToName(int vk);
	UTIL_API String VirtualKeyToDisplayName(int vk);
	UTIL_API InputAction VirtualKeyToInputAction(int vk);
	UTIL_API InputAction NameToInputAction(StringRef name);
	UTIL_API int InputActionToVirtualKey(const InputAction &action);
	UTIL_API String InputActionToName(const InputAction &action);
	UTIL_API String InputActionToDisplayName(const InputAction &action);
}
