#include "pch.h"
#include "COM/ComObject.h"
#include "Core/Difficulty.h"
#include "Core/Maze.h"
#include "Core/Tiles.h"
#include "Globals/MetroGlobals.h"
#include "Input/KeyboardDevice.h"
#include "Module/Module.h"

class __declspec(uuid("b88ba5a3-be9e-4023-8c67-32cdaf5cdf40"))
	Maze : public ff::ComBase, public IMaze
{
public:
	DECLARE_HEADER(Maze);

	bool Init(
		CharType type,
		Tiles *pTiles,
		DirectX::XMFLOAT4 colorBorder,
		DirectX::XMFLOAT4 colorFill,
		DirectX::XMFLOAT4 colorDot);

	// IMaze

	virtual REFGUID GetID() const override;
	virtual bool Clone(bool bShareTiles, IMaze **ppMaze) override;

	virtual void AddListener(IMazeListener *pListener) override;
	virtual void RemoveListener(IMazeListener *pListener) override;

	virtual CharType GetCharType() const override;
	virtual void SetCharType(CharType type) override;

	virtual ff::PointInt GetSizeInTiles() const override;
	virtual void SetSizeInTiles(ff::PointInt tileSize) override;
	virtual void ShiftTiles(ff::PointInt tileShift) override;

	virtual TileContent GetTileContent(ff::PointInt tile) const override;
	virtual void SetTileContent(ff::PointInt tile, TileContent content) override;
	virtual TileZone GetTileZone(ff::PointInt tile) const override;
	virtual void SetTileZone(ff::PointInt tile, TileZone zone) override;

	virtual const DirectX::XMFLOAT4 &GetFillColor() const override;
	virtual const DirectX::XMFLOAT4 &GetBorderColor() const override;
	virtual const DirectX::XMFLOAT4 &GetBackgroundColor() const override;
	virtual void SetFillColor(const DirectX::XMFLOAT4 &color) override;
	virtual void SetBorderColor(const DirectX::XMFLOAT4 &color) override;
	virtual void SetBackgroundColor(const DirectX::XMFLOAT4 &color) override;

private:
	GUID _id;
	DirectX::XMFLOAT4 _fillColor;
	DirectX::XMFLOAT4 _borderColor;
	DirectX::XMFLOAT4 _backgroundColor;
	ff::ComPtr<Tiles> _tiles;
	CharType _charType;
};

BEGIN_INTERFACES(Maze)
	HAS_INTERFACE(IMaze)
END_INTERFACES()

static DirectX::XMFLOAT4 VectorFromRect(const ff::RectFloat &rect)
{
	return DirectX::XMFLOAT4(rect.left, rect.top, rect.right, rect.bottom);
}

bool CreateMazeFromResource(ff::StringRef name, IMaze **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ValuePtr rawValue = ff::GetThisModule().GetValue(name);
	assertRetVal(rawValue, false);

	ff::ValuePtr dictValue;
	assertRetVal(rawValue->Convert(ff::Value::Type::Dict, &dictValue), false);

	const ff::Dict &dict = dictValue->AsDict();
	ff::String strType = dict.GetString(ff::String(L"type"), ff::String(L"mr"));
	ff::String strTiles = dict.GetString(ff::String(L"tiles"), ff::String(L"tiles-0"));
	DirectX::XMFLOAT4 borderColor = VectorFromRect(dict.GetRectF(ff::String(L"borderColor"), ff::RectFloat(0, 0, 1, 1)));
	DirectX::XMFLOAT4 fillColor = VectorFromRect(dict.GetRectF(ff::String(L"fillColor"), ff::RectFloat(0, 0, 0, 0)));
	DirectX::XMFLOAT4 backgroundColor = VectorFromRect(dict.GetRectF(ff::String(L"backgroundColor"), ff::RectFloat(0, 0, 0, 0)));

	CharType type = (strType == L"ms") ? CHAR_MS : CHAR_MR;
	ff::ComPtr<Tiles> tiles;
	assertRetVal(CreateTilesFromResource(strTiles, &tiles), false);
	assertRetVal(IMaze::Create(type, tiles, borderColor, fillColor, backgroundColor, obj), false);

	return *obj != nullptr;
}

// static
bool IMaze::Create(
	CharType type,
	Tiles* pTiles,
	const DirectX::XMFLOAT4 &colorBorder,
	const DirectX::XMFLOAT4 &colorFill,
	const DirectX::XMFLOAT4 &colorDot,
	IMaze** ppMaze)
{
	assertRetVal(ppMaze, false);
	*ppMaze = nullptr;

	ff::ComPtr<Maze> pMaze = new ff::ComObject<Maze>;
	assertRetVal(pMaze->Init(type, pTiles, colorBorder, colorFill, colorDot), false);

	*ppMaze = ff::GetAddRef<IMaze>(pMaze);

	return true;
}

Maze::Maze()
	: _fillColor(0, 0, 0, 0)
	, _borderColor(0, 0, 0, 0)
	, _backgroundColor(0, 0, 0, 0)
	, _charType(CHAR_DEFAULT)
{
	CoCreateGuid(&_id);
}

Maze::~Maze()
{
}

bool Maze::Init(
	CharType type,
	Tiles *pTiles,
	DirectX::XMFLOAT4 colorBorder,
	DirectX::XMFLOAT4 colorFill,
	DirectX::XMFLOAT4 colorBackground)
{
	assertRetVal(pTiles, false);

	_charType = type;
	_tiles = pTiles;
	_fillColor = colorFill;
	_borderColor = colorBorder;
	_backgroundColor = colorBackground;

	return true;
}

REFGUID Maze::GetID() const
{
	return _id;
}

bool Maze::Clone(bool bShareTiles, IMaze **ppMaze)
{
	assertRetVal(ppMaze, false);
	*ppMaze = nullptr;

	ff::ComPtr<Maze> pMaze = new ff::ComObject<Maze>;

	pMaze->_charType = _charType;
	pMaze->_fillColor = _fillColor;
	pMaze->_borderColor = _borderColor;
	pMaze->_backgroundColor = _backgroundColor;

	assertRetVal(_tiles, false);

	if (bShareTiles)
	{
		pMaze->_tiles = _tiles;
	}
	else
	{
		pMaze->_tiles = nullptr;
		assertRetVal(_tiles->Clone(&pMaze->_tiles), false);
	}

	*ppMaze = ff::GetAddRef<IMaze>(pMaze);

	return true;
}

void Maze::AddListener(IMazeListener *pListener)
{
	if (_tiles)
	{
		_tiles->AddListener(pListener);
	}
}

void Maze::RemoveListener(IMazeListener *pListener)
{
	if (_tiles)
	{
		_tiles->RemoveListener(pListener);
	}
}

static bool s_bSwapping = false;

CharType Maze::GetCharType() const
{
	return _charType;
}

void Maze::SetCharType(CharType type)
{
	_charType = type;
}

ff::PointInt Maze::GetSizeInTiles() const
{
	return _tiles->GetSize();
}

void Maze::SetSizeInTiles(ff::PointInt tileSize)
{
	_tiles->SetSize(tileSize);
}

void Maze::ShiftTiles(ff::PointInt tileShift)
{
	_tiles->Shift(tileShift);
}

TileContent Maze::GetTileContent(ff::PointInt tile) const
{
	return _tiles->GetContent(tile);
}

void Maze::SetTileContent(ff::PointInt tile, TileContent content)
{
	_tiles->SetContent(tile, content);
}

TileZone Maze::GetTileZone(ff::PointInt tile) const
{
	return _tiles->GetZone(tile);
}

void Maze::SetTileZone(ff::PointInt tile, TileZone zone)
{
	_tiles->SetZone(tile, zone);
}

const DirectX::XMFLOAT4 &Maze::GetFillColor() const
{
	return _fillColor;
}

const DirectX::XMFLOAT4 &Maze::GetBorderColor() const
{
	return _borderColor;
}

const DirectX::XMFLOAT4 &Maze::GetBackgroundColor() const
{
	return _backgroundColor;
}

void Maze::SetFillColor(const DirectX::XMFLOAT4 &color)
{
	_fillColor = color;
}

void Maze::SetBorderColor(const DirectX::XMFLOAT4 &color)
{
	_borderColor = color;
}

void Maze::SetBackgroundColor(const DirectX::XMFLOAT4 &color)
{
	_backgroundColor = color;
}
