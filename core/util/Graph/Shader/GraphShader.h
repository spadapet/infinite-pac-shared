#pragma once

namespace ff
{
	class __declspec(uuid("4869aaa3-ef89-48b0-870e-ce1662aa48c8")) __declspec(novtable)
		IGraphShader : public IUnknown
	{
	public:
		virtual IData *GetData() const = 0;
		virtual void SetData(IData *pData) = 0;
	};

	UTIL_API bool CreateGraphShader(IGraphShader **ppShader);
}
