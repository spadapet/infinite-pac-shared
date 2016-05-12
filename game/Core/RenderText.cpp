#include "pch.h"
#include "COM/ComObject.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/RenderText.h"
#include "Graph/2D/2dRenderer.h"
#include "Graph/2D/Sprite.h"
#include "Graph/2D/SpriteList.h"
#include "Resource/ResourceValue.h"

class __declspec(uuid("1bfdc75e-3eb9-4915-a9c8-1cdbb60c4cab"))
	CRenderText : public ff::ComBase, public IRenderText
{
public:
	DECLARE_HEADER(CRenderText);

	bool Init();

	// IRenderText

	virtual void DrawText(
		ff::I2dRenderer *pRenderer,
		const wchar_t *szText,
		ff::PointFloat pos,
		float lineHeight,
		const DirectX::XMFLOAT4 *pColor,
		const DirectX::XMFLOAT4 *pBgColor,
		const ff::PointFloat *pScale) override;

	virtual void DrawSmallNumber(
		ff::I2dRenderer *pRenderer,
		size_t nPoints,
		ff::PointFloat center,
		const DirectX::XMFLOAT4 *pColor,
		const ff::PointFloat *pScale) override;

private:
	bool GetSprites();

	ff::TypedResource<ff::ISpriteList> _sprites;
	ff::Vector<ff::ComPtr<ff::ISprite>> _font;

	ff::TypedResource<ff::ISpriteList> _smallSprites;
	ff::Vector<ff::ComPtr<ff::ISprite>> _smallFont;
};

BEGIN_INTERFACES(CRenderText)
	HAS_INTERFACE(IRenderText)
END_INTERFACES()

// static
bool IRenderText::Create(IRenderText **ppOut)
{
	assertRetVal(ppOut, false);
	*ppOut = nullptr;

	ff::ComPtr<CRenderText> pRender = new ff::ComObject<CRenderText>;
	assertRetVal(pRender->Init(), false);

	*ppOut = ff::GetAddRef<IRenderText>(pRender);

	return true;
}

ff::String FormatScoreAsString(size_t nScore)
{
	nScore = std::min<size_t>(nScore, 9999999);

	if (!nScore)
	{
		return ff::String(L"     00");
	}
	else
	{
		wchar_t szInt[8];
		_itot_s((int)nScore, szInt, 10);

		ff::String szScore;
		szScore.append(7 - _tcslen(szInt), ' ');
		szScore += szInt;

		return szScore;
	}
}

ff::String FormatHighScoreName(const wchar_t *szName)
{
	assertRetVal(szName, ff::String());

	ff::String szFormatted(szName, std::min<size_t>(_tcslen(szName), 9));

	if (szFormatted.empty())
	{
		szFormatted = L"---";
	}

	return szFormatted;
}

CRenderText::CRenderText()
{
}

CRenderText::~CRenderText()
{
}

bool CRenderText::Init()
{
	_sprites = GetFontSpritePage();
	_smallSprites = GetSmallFontSpritePage();

	return true;
}

bool CRenderText::GetSprites()
{
	if (!_font.Size() && _sprites.GetObject())
	{
		_font.Reserve(128);

		for (wchar_t i = 0; i <= ' '; i++)
		{
			_font.Push(ff::ComPtr<ff::ISprite>());
		}

		for (wchar_t i = ' ' + 1; i < 128; i++)
		{
			wchar_t ch = tolower(i);
			ff::String szName(1, ch);

			ff::ComPtr<ff::ISprite> pSprite = _sprites.GetObject()->Get(szName);
			// pSprite can be nullptr

			_font.Push(pSprite);
		}
	}

	if (!_smallFont.Size() && _smallSprites.GetObject())
	{
		_smallFont.Reserve(10);

		for (wchar_t i = '0'; i <= '9'; i++)
		{
			ff::String szName(1, i);

			ff::ComPtr<ff::ISprite> pSprite = _smallSprites.GetObject()->Get(szName);
			assert(pSprite);

			_smallFont.Push(pSprite);
		}
	}

	return true;
}

void CRenderText::DrawText(
		ff::I2dRenderer *pRenderer,
		const wchar_t *szText,
		ff::PointFloat pos,
		float lineHeight,
		const DirectX::XMFLOAT4 *pColor,
		const DirectX::XMFLOAT4 *pBgColor,
		const ff::PointFloat *pScale)
{
	assertRet(pRenderer);

	if (!szText || !*szText || !GetSprites() || !_font.Size())
	{
		return;
	}

	ff::PointFloat scale(0.125f, 0.125f);

	if (pScale)
	{
		scale *= *pScale;
	}

	ff::PointFloat curPos = pos;
	ff::PointFloat tileSize = PixelsPerTileF();

	for (wchar_t ch = *szText; ch; szText++, ch = *szText)
	{
		if (ch == '\n')
		{
			curPos.x = pos.x;
			curPos.y += (lineHeight > 0) ? lineHeight : tileSize.y * scale.y * 8.0f;
		}
		else
		{
			if (ch >= 0 && ch < _font.Size() && _font[ch])
			{
				pRenderer->DrawSprite(_font[ch], &curPos, &scale, 0, pColor);

				if (pBgColor)
				{
					static DirectX::XMFLOAT4 s_colorBlack(0, 0, 0, 1);

					pRenderer->DrawFilledRectangle(
						&ff::RectFloat(curPos, curPos + scale * 8.0f * PixelsPerTileF()),
						&s_colorBlack, 1);
				}
			}

			curPos.x += tileSize.x * scale.x * 8.0f;
		}
	}
}

void CRenderText::DrawSmallNumber(
		ff::I2dRenderer *pRenderer,
		size_t nPoints,
		ff::PointFloat center,
		const DirectX::XMFLOAT4 *pColor,
		const ff::PointFloat *pScale)
{
	assertRet(pRenderer);

	assertRet(nPoints >= 0 && nPoints <= 9999999);

	if (!GetSprites())
	{
		return;
	}

	wchar_t str[8];
	_itot_s((int)nPoints, str, 10);

	// compute the size of the text first

	ff::PointFloat size(0, 0);
	ff::PointFloat scale(0.125f, 0.125f);
	ff::ISpriteList *smallSprites = _smallSprites.GetObject();

	if (pScale)
	{
		scale *= *pScale;
	}

	for (size_t i = 0; i < 8 && str[i]; i++)
	{
		size_t nSprite = str[i] - '0';
		ff::ISprite *pSprite = smallSprites->Get(nSprite);

		if (!pSprite)
		{
			assert(false);
			return;
		}

		if (size.x > 0)
		{
			size.x++;
		}

		const ff::SpriteData &data = pSprite->GetSpriteData();
		size.x += data._worldRect.Width() * scale.x;
		size.y = std::max(size.y, data._worldRect.Height() * scale.y);
	}

	// now loop again and draw the sprites

	ff::PointFloat pos(floorf(center.x - size.x / 2.0f), floorf(center.y - size.y / 2.0f));

	for (size_t i = 0; i < 8 && str[i]; i++)
	{
		size_t nSprite = str[i] - '0';
		ff::ISprite *pSprite = smallSprites->Get(nSprite);

		pRenderer->DrawSprite(pSprite, &pos, &scale, 0, pColor);

		const ff::SpriteData &data = pSprite->GetSpriteData();
		pos.x += data._worldRect.Width() * scale.x + 1.0f;
	}
}
