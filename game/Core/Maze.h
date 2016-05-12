#pragma once

class Tiles;
class IMazeListener;
enum CharType;
enum TileContent : BYTE;
enum TileZone : BYTE;

class __declspec(uuid("cd774d0c-94ef-4ce2-a989-e21030a9a22d")) __declspec(novtable)
	IMaze : public IUnknown
{
public:
	static bool Create(
		CharType type,
		Tiles *pTiles,
		const DirectX::XMFLOAT4 &colorBorder,
		const DirectX::XMFLOAT4 &colorFill,
		const DirectX::XMFLOAT4 &colorBackground,
		IMaze **ppMaze);

	virtual REFGUID GetID() const = 0;
	virtual bool Clone(bool bShareTiles, IMaze **ppMaze) = 0;

	virtual void AddListener(IMazeListener *pListener) = 0;
	virtual void RemoveListener(IMazeListener *pListener) = 0;

	virtual CharType GetCharType() const = 0;
	virtual void SetCharType(CharType type) = 0;

	virtual ff::PointInt GetSizeInTiles() const = 0;
	virtual void SetSizeInTiles(ff::PointInt tileSize) = 0;
	virtual void ShiftTiles(ff::PointInt tileShift) = 0;

	virtual TileContent GetTileContent(ff::PointInt tile) const = 0;
	virtual void SetTileContent(ff::PointInt tile, TileContent content) = 0;
	virtual TileZone GetTileZone(ff::PointInt tile) const = 0;
	virtual void SetTileZone(ff::PointInt tile, TileZone zone) = 0;

	virtual const DirectX::XMFLOAT4 &GetFillColor() const = 0;
	virtual const DirectX::XMFLOAT4 &GetBorderColor() const = 0;
	virtual const DirectX::XMFLOAT4 &GetBackgroundColor() const = 0;
	virtual void SetFillColor(const DirectX::XMFLOAT4 &color) = 0;
	virtual void SetBorderColor(const DirectX::XMFLOAT4 &color) = 0;
	virtual void SetBackgroundColor(const DirectX::XMFLOAT4 &color) = 0;
};

class __declspec(uuid("59bba370-a8f2-434e-9724-0c4c92bd1926")) __declspec(novtable)
	IMazeListener // not ref counted
{
public:
	virtual void OnTileChanged(ff::PointInt tile, TileContent oldContent, TileContent newContent) = 0;
	virtual void OnAllTilesChanged() = 0;
};

bool CreateMazeFromResource(ff::StringRef name, IMaze **obj);
