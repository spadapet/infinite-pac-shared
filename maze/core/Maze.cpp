#include "pch.h"
#include "Core/Difficulty.h"
#include "Core/Maze.h"
#include "Core/Tiles.h"

class Maze : public IMaze
{
public:
    Maze(
        CharType type,
        std::shared_ptr<Tiles> pTiles,
        DirectX::XMFLOAT4 colorBorder,
        DirectX::XMFLOAT4 colorFill,
        DirectX::XMFLOAT4 colorBackground);

    // IMaze

    virtual REFGUID GetID() const override;
    virtual std::shared_ptr<IMaze> Clone(bool bShareTiles) override;

    virtual void AddListener(IMazeListener* pListener) override;
    virtual void RemoveListener(IMazeListener* pListener) override;

    virtual CharType GetCharType() const override;
    virtual void SetCharType(CharType type) override;

    virtual ff::point_int GetSizeInTiles() const override;
    virtual void SetSizeInTiles(ff::point_int tileSize) override;
    virtual void ShiftTiles(ff::point_int tileShift) override;

    virtual TileContent GetTileContent(ff::point_int tile) const override;
    virtual void SetTileContent(ff::point_int tile, TileContent content) override;
    virtual TileZone GetTileZone(ff::point_int tile) const override;
    virtual void SetTileZone(ff::point_int tile, TileZone zone) override;

    virtual const DirectX::XMFLOAT4& GetFillColor() const override;
    virtual const DirectX::XMFLOAT4& GetBorderColor() const override;
    virtual const DirectX::XMFLOAT4& GetBackgroundColor() const override;
    virtual void SetFillColor(const DirectX::XMFLOAT4& color) override;
    virtual void SetBorderColor(const DirectX::XMFLOAT4& color) override;
    virtual void SetBackgroundColor(const DirectX::XMFLOAT4& color) override;

private:
    GUID _id;
    DirectX::XMFLOAT4 _fillColor;
    DirectX::XMFLOAT4 _borderColor;
    DirectX::XMFLOAT4 _backgroundColor;
    std::shared_ptr<Tiles> _tiles;
    CharType _charType;
};

static DirectX::XMFLOAT4 VectorFromRect(const ff::rect_float& rect)
{
    return DirectX::XMFLOAT4(rect.left, rect.top, rect.right, rect.bottom);
}

std::shared_ptr<IMaze> CreateMazeFromResource(std::string_view name)
{
    static std::shared_ptr<ff::resource_values> values_maze;
    if (!values_maze)
    {
        values_maze = ff::auto_resource<ff::resource_values>("values_maze").object();
    }

    ff::value_ptr rawValue = values_maze->get_resource_value(name);
    assert_ret_val(rawValue, nullptr);

    ff::value_ptr dictValue = rawValue->try_convert<ff::dict>();
    assert_ret_val(dictValue, nullptr);

    const ff::dict& dict = dictValue->get<ff::dict>();
    std::string strType = dict.get<std::string>("type", std::string("mr"));
    std::string strTiles = dict.get<std::string>("tiles", std::string("tiles-0"));
    DirectX::XMFLOAT4 borderColor = VectorFromRect(dict.get<ff::rect_float>("borderColor", ff::rect_float(0, 0, 1, 1)));
    DirectX::XMFLOAT4 fillColor = VectorFromRect(dict.get<ff::rect_float>("fillColor", ff::rect_float(0, 0, 0, 0)));
    DirectX::XMFLOAT4 backgroundColor = VectorFromRect(dict.get<ff::rect_float>("backgroundColor", ff::rect_float(0, 0, 0, 0)));

    CharType type = (strType == "ms") ? CHAR_MS : CHAR_MR;
    std::shared_ptr<Tiles> tiles = CreateTilesFromResource(strTiles);
    assert_ret_val(tiles, nullptr);

    return std::make_shared<Maze>(type, tiles, borderColor, fillColor, backgroundColor);
}

std::shared_ptr<IMaze> IMaze::Create(
    CharType type,
    std::shared_ptr<Tiles> pTiles,
    const DirectX::XMFLOAT4& colorBorder,
    const DirectX::XMFLOAT4& colorFill,
    const DirectX::XMFLOAT4& colorBackground)
{
    return std::make_shared<Maze>(type, pTiles, colorBorder, colorFill, colorBackground);
}

Maze::Maze(
    CharType type,
    std::shared_ptr<Tiles> pTiles,
    DirectX::XMFLOAT4 colorBorder,
    DirectX::XMFLOAT4 colorFill,
    DirectX::XMFLOAT4 colorBackground)
    : _tiles(pTiles)
    , _fillColor(colorFill)
    , _borderColor(colorBorder)
    , _backgroundColor(colorBackground)
    , _charType(type)
{
    CoCreateGuid(&_id);
}

REFGUID Maze::GetID() const
{
    return _id;
}

std::shared_ptr<IMaze> Maze::Clone(bool bShareTiles)
{
    std::shared_ptr<Tiles> tiles = bShareTiles ? _tiles : _tiles->Clone();
    return std::make_shared<Maze>(_charType, tiles, _borderColor, _fillColor, _backgroundColor);
}

void Maze::AddListener(IMazeListener* pListener)
{
    if (_tiles)
    {
        _tiles->AddListener(pListener);
    }
}

void Maze::RemoveListener(IMazeListener* pListener)
{
    if (_tiles)
    {
        _tiles->RemoveListener(pListener);
    }
}

CharType Maze::GetCharType() const
{
    return _charType;
}

void Maze::SetCharType(CharType type)
{
    _charType = type;
}

ff::point_int Maze::GetSizeInTiles() const
{
    return _tiles->GetSize();
}

void Maze::SetSizeInTiles(ff::point_int tileSize)
{
    _tiles->SetSize(tileSize);
}

void Maze::ShiftTiles(ff::point_int tileShift)
{
    _tiles->Shift(tileShift);
}

TileContent Maze::GetTileContent(ff::point_int tile) const
{
    return _tiles->GetContent(tile);
}

void Maze::SetTileContent(ff::point_int tile, TileContent content)
{
    _tiles->SetContent(tile, content);
}

TileZone Maze::GetTileZone(ff::point_int tile) const
{
    return _tiles->GetZone(tile);
}

void Maze::SetTileZone(ff::point_int tile, TileZone zone)
{
    _tiles->SetZone(tile, zone);
}

const DirectX::XMFLOAT4& Maze::GetFillColor() const
{
    return _fillColor;
}

const DirectX::XMFLOAT4& Maze::GetBorderColor() const
{
    return _borderColor;
}

const DirectX::XMFLOAT4& Maze::GetBackgroundColor() const
{
    return _backgroundColor;
}

void Maze::SetFillColor(const DirectX::XMFLOAT4& color)
{
    _fillColor = color;
}

void Maze::SetBorderColor(const DirectX::XMFLOAT4& color)
{
    _borderColor = color;
}

void Maze::SetBackgroundColor(const DirectX::XMFLOAT4& color)
{
    _backgroundColor = color;
}
