#pragma once

namespace ff
{
	class I2dRenderer;
}

class __declspec(uuid("cfaaf861-a468-424f-b4ea-b325f7f0dc6c")) __declspec(novtable)
	IRenderText : public IUnknown
{
public:
	static bool Create(IRenderText **ppOut);

	virtual void DrawText(
		ff::I2dRenderer *pRenderer,
		const wchar_t *szText,
		ff::PointFloat pos,
		float lineHeight,
		const DirectX::XMFLOAT4 *pColor,
		const DirectX::XMFLOAT4 *pBgColor,
		const ff::PointFloat *pScale) = 0;

	virtual void DrawSmallNumber(
		ff::I2dRenderer *pRenderer,
		size_t nPoints,
		ff::PointFloat center,
		const DirectX::XMFLOAT4 *pColor,
		const ff::PointFloat *pScale) = 0;
};

ff::String FormatScoreAsString(size_t nScore);
ff::String FormatHighScoreName(const wchar_t *szName);
