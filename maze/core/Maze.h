#pragma once

class Tiles;
class IMazeListener;
enum CharType;
enum TileContent : BYTE;
enum TileZone : BYTE;

class IMaze
{
public:
    virtual ~IMaze() = default;

    static std::shared_ptr<IMaze> Create(
        CharType type,
        std::shared_ptr<Tiles> pTiles,
        const DirectX::XMFLOAT4& colorBorder,
        const DirectX::XMFLOAT4& colorFill,
        const DirectX::XMFLOAT4& colorBackground);

    virtual REFGUID GetID() const = 0;
    virtual std::shared_ptr<IMaze> Clone(bool bShareTiles) = 0;

    virtual void AddListener(IMazeListener* pListener) = 0;
    virtual void RemoveListener(IMazeListener* pListener) = 0;

    virtual CharType GetCharType() const = 0;
    virtual void SetCharType(CharType type) = 0;

    virtual ff::point_int GetSizeInTiles() const = 0;
    virtual void SetSizeInTiles(ff::point_int tileSize) = 0;
    virtual void ShiftTiles(ff::point_int tileShift) = 0;

    virtual TileContent GetTileContent(ff::point_int tile) const = 0;
    virtual void SetTileContent(ff::point_int tile, TileContent content) = 0;
    virtual TileZone GetTileZone(ff::point_int tile) const = 0;
    virtual void SetTileZone(ff::point_int tile, TileZone zone) = 0;

    virtual const DirectX::XMFLOAT4& GetFillColor() const = 0;
    virtual const DirectX::XMFLOAT4& GetBorderColor() const = 0;
    virtual const DirectX::XMFLOAT4& GetBackgroundColor() const = 0;
    virtual void SetFillColor(const DirectX::XMFLOAT4& color) = 0;
    virtual void SetBorderColor(const DirectX::XMFLOAT4& color) = 0;
    virtual void SetBackgroundColor(const DirectX::XMFLOAT4& color) = 0;
};

class IMazeListener
{
public:
    virtual ~IMazeListener() = default;

    virtual void OnTileChanged(ff::point_int tile, TileContent oldContent, TileContent newContent) = 0;
    virtual void OnAllTilesChanged() = 0;
};

std::shared_ptr<IMaze> CreateMazeFromResource(std::string_view name);
