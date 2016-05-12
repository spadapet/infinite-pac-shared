#pragma once

namespace ff
{
	class __declspec(uuid("ac7d87ef-f6c8-4fd2-b740-d2568462edd8")) __declspec(novtable)
		IInputDevice : public IUnknown
	{
	public:
		virtual void Advance() = 0;
		virtual bool IsConnected() const = 0;
	};
}
