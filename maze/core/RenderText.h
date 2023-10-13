#pragma once

class IRenderText
{
public:
    virtual ~IRenderText() = default;

    static std::shared_ptr<IRenderText> Create();

    virtual void DrawText(
        ff::dxgi::draw_base& draw,
        const char* szText,
        ff::point_float pos,
        float lineHeight,
        const DirectX::XMFLOAT4* pColor,
        const DirectX::XMFLOAT4* pBgColor,
        const ff::point_float* pScale) = 0;

    virtual void DrawSmallNumber(
        ff::dxgi::draw_base& draw,
        size_t nPoints,
        ff::point_float center,
        const DirectX::XMFLOAT4* pColor,
        const ff::point_float* pScale) = 0;
};

std::string FormatScoreAsString(size_t nScore);
std::string FormatHighScoreName(const char* szName);
