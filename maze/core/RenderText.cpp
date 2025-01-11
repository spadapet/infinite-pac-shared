#include "pch.h"
#include "Core/GlobalResources.h"
#include "Core/Helpers.h"
#include "Core/RenderText.h"

class RenderText : public IRenderText
{
public:
    RenderText();

    virtual void DrawText(
        ff::dxgi::draw_base& draw,
        const char* szText,
        ff::point_float pos,
        float lineHeight,
        const DirectX::XMFLOAT4* pColor,
        const DirectX::XMFLOAT4* pBgColor,
        const ff::point_float* pScale) override;

    virtual void DrawSmallNumber(
        ff::dxgi::draw_base& draw,
        size_t nPoints,
        ff::point_float center,
        const DirectX::XMFLOAT4* pColor,
        const ff::point_float* pScale) override;

private:
    bool GetSprites();

    ff::auto_resource<ff::sprite_list> _sprites;
    std::vector<const ff::sprite_base*> _font;

    ff::auto_resource<ff::sprite_list> _smallSprites;
    std::vector<const ff::sprite_base*> _smallFont;
};

std::shared_ptr<IRenderText> IRenderText::Create()
{
    return std::make_shared<RenderText>();
}

std::string FormatScoreAsString(size_t nScore)
{
    nScore = std::min<size_t>(nScore, 9999999);

    if (!nScore)
    {
        return std::string("     00");
    }
    else
    {
        char szInt[8];
        _itoa_s((int)nScore, szInt, 10);

        std::string szScore;
        szScore.append(7 - strlen(szInt), ' ');
        szScore += szInt;

        return szScore;
    }
}

std::string FormatHighScoreName(const char* szName)
{
    assert_ret_val(szName, std::string());

    std::string szFormatted(szName, std::min<size_t>(strlen(szName), 9));

    if (szFormatted.empty())
    {
        szFormatted = "---";
    }

    return szFormatted;
}

RenderText::RenderText()
{
    _sprites = GetFontSpritePage();
    _smallSprites = GetSmallFontSpritePage();
}

bool RenderText::GetSprites()
{
    if (!_font.size() && _sprites.object())
    {
        _font.reserve(128);

        for (char i = 0; i <= ' '; i++)
        {
            _font.push_back(nullptr);
        }

        for (int i = ' ' + 1; i < 128; i++)
        {
            int ch = tolower(i);
            std::string szName(1, (char)ch);

            const ff::sprite_base* pSprite = _sprites.object()->get(szName);
            // pSprite can be nullptr

            _font.push_back(pSprite);
        }
    }

    if (!_smallFont.size() && _smallSprites.object())
    {
        _smallFont.reserve(10);

        for (char i = '0'; i <= '9'; i++)
        {
            std::string szName(1, i);

            const ff::sprite_base* pSprite = _smallSprites.object()->get(szName);
            assert(pSprite);

            _smallFont.push_back(pSprite);
        }
    }

    return true;
}

void RenderText::DrawText(
        ff::dxgi::draw_base& draw,
        const char* szText,
        ff::point_float pos,
        float lineHeight,
        const DirectX::XMFLOAT4* pColor,
        const DirectX::XMFLOAT4* pBgColor,
        const ff::point_float* pScale)
{
    if (!szText || !*szText || !GetSprites() || !_font.size())
    {
        return;
    }

    ff::point_float scale(0.125f, 0.125f);

    if (pScale)
    {
        scale *= *pScale;
    }

    ff::point_float curPos = pos;
    ff::point_float tileSize = PixelsPerTileF();

    draw.push_no_overlap();

    for (char ch = *szText; ch; szText++, ch = *szText)
    {
        if (ch == '\n')
        {
            curPos.x = pos.x;
            curPos.y += (lineHeight > 0) ? lineHeight : tileSize.y * scale.y * 8.0f;
        }
        else
        {
            if (ch >= 0 && ch < _font.size() && _font[ch])
            {
                // pBgColor is ignored, it was never needed
                draw.draw_sprite(_font[ch]->sprite_data(), ff::transform(curPos, scale, 0, pColor ? ff::color(*pColor) : ff::color_white()));
            }

            curPos.x += tileSize.x * scale.x * 8.0f;
        }
    }

    draw.pop_no_overlap();
}

void RenderText::DrawSmallNumber(
        ff::dxgi::draw_base& draw,
        size_t nPoints,
        ff::point_float center,
        const DirectX::XMFLOAT4* pColor,
        const ff::point_float* pScale)
{
    assert_ret(nPoints >= 0 && nPoints <= 9999999);

    if (!GetSprites())
    {
        return;
    }

    char str[8];
    _itoa_s((int)nPoints, str, 10);

    // compute the size of the text first

    ff::point_float size(0, 0);
    ff::point_float scale(0.125f, 0.125f);
    ff::sprite_list* smallSprites = _smallSprites.object().get();

    if (pScale)
    {
        scale *= *pScale;
    }

    for (size_t i = 0; i < 8 && str[i]; i++)
    {
        size_t nSprite = str[i] - '0';
        const ff::sprite_base* pSprite = smallSprites->get(nSprite);

        if (!pSprite)
        {
            assert(false);
            return;
        }

        if (size.x > 0)
        {
            size.x++;
        }

        const ff::dxgi::sprite_data& data = pSprite->sprite_data();
        size.x += data.world().width() * scale.x;
        size.y = std::max(size.y, data.world().height() * scale.y);
    }

    // now loop again and draw the sprites

    ff::point_float pos(floorf(center.x - size.x / 2.0f), floorf(center.y - size.y / 2.0f));

    for (size_t i = 0; i < 8 && str[i]; i++)
    {
        size_t nSprite = str[i] - '0';
        const ff::sprite_base* pSprite = smallSprites->get(nSprite);

        draw.draw_sprite(pSprite->sprite_data(), ff::transform(pos, scale, 0, pColor ? ff::color(*pColor) : ff::color_white()));

        const ff::dxgi::sprite_data& data = pSprite->sprite_data();
        pos.x += data.world().width() * scale.x + 1.0f;
    }
}
