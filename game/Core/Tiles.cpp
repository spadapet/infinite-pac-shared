#include "pch.h"
#include "COM/ComObject.h"
#include "Core/Tiles.h"
#include "Core/Maze.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"

static bool CreateTilesFromString(const wchar_t *szTiles, ff::PointInt size, bool bMirror, Tiles **ppTiles)
{
	assertRetVal(ppTiles, false);
	*ppTiles = nullptr;

	assertRetVal(szTiles && size.x > 0 && size.y > 0, false);
	assertRetVal(!bMirror || !(size.x % 2), false);

	ff::ComPtr<Tiles> pTiles = new ff::ComObject<Tiles>;
	pTiles->SetSize(size);

	const wchar_t *sz = szTiles;

	for (int y = 0; y < size.y; y++)
	{
		int maxX = bMirror ? size.x / 2 : size.x;
		for (int x = 0; x < maxX; x++, sz++)
		{
			TileContent content = CONTENT_NOTHING;
			TileZone zone = ZONE_NORMAL;

			switch (*sz)
			{
				case 'X': content = CONTENT_WALL; break;
				case 'O': content = CONTENT_GHOST_WALL; break;
				case '-': content = CONTENT_GHOST_DOOR; break;
				case '+': content = CONTENT_PAC_START; break;
				case '.': content = CONTENT_DOT; break;
				case ',': content = CONTENT_POWER; break;
				case '=': zone = ZONE_GHOST_SLOW; break;
				case '*': zone = ZONE_GHOST_NO_TURN; break;
				case '|': zone = ZONE_OUT_OF_BOUNDS; break;
			}

			if (*sz == '*' && ((x > 0 && sz[-1] == '.') || (x + 1 < maxX && sz[1] == '.')))
			{
				content = CONTENT_DOT;
			}

			pTiles->SetContent(ff::PointInt(x, y), content);
			pTiles->SetZone(ff::PointInt(x, y), zone);

			if (bMirror)
			{
				pTiles->SetContent(ff::PointInt(size.x - 1 - x, y), content);
				pTiles->SetZone(ff::PointInt(size.x - 1 - x, y), zone);
			}
		}
	}

	*ppTiles = ff::GetAddRef<Tiles>(pTiles);

	return true;
}

bool CreateTilesFromResource(ff::String name, Tiles **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ValuePtr rawValue = ff::GetThisModule().GetValue(name);
	assertRetVal(rawValue, false);

	ff::ValuePtr dictValue;
	assertRetVal(rawValue->Convert(ff::Value::Type::Dict, &dictValue), false);

	const ff::Dict &dict = dictValue->AsDict();
	rawValue = dict.GetValue(ff::String(L"half-tiles"));
	assertRetVal(rawValue, false);

	ff::ValuePtr stringsValue;
	assertRetVal(rawValue->Convert(ff::Value::Type::StringVector, &stringsValue), false);

	const ff::Vector<ff::String> &strings = stringsValue->AsStringVector();
	assertRetVal(strings.Size(), false);

	size_t length = strings[0].size();
	ff::String combinedString;
	combinedString.reserve(length * strings.Size());

	for (ff::StringRef str : strings)
	{
		assertRetVal(str.size() == length, false);
		combinedString += str;
	}

	assertRetVal(CreateTilesFromString(
		combinedString.c_str(),
		ff::PointInt((int)length * 2, (int)strings.Size()),
		true, // mirror half tiles
		obj), false);

	return *obj != nullptr;
}

Tiles::Tiles()
	: _size(0, 0)
{
	::CoCreateGuid(&_id);
}

Tiles::~Tiles()
{
	assert(!_listeners.Size());
}

BEGIN_INTERFACES(Tiles)
END_INTERFACES()

bool Tiles::Clone(Tiles **ppTiles)
{
	assertRetVal(ppTiles, false);
	*ppTiles = nullptr;

	ff::ComPtr<Tiles> pTiles = new ff::ComObject<Tiles>;

	pTiles->_size = _size;
	pTiles->_content = _content;
	pTiles->_zone = _zone;

	*ppTiles = ff::GetAddRef<Tiles>(pTiles);

	return true;
}

REFGUID Tiles::GetID() const
{
	return _id;
}

ff::PointInt Tiles::GetSize() const
{
	return _size;
}

void Tiles::SetSize(ff::PointInt newSize)
{
	if (newSize != _size)
	{
		ff::Vector<TileContent> newContent;
		ff::Vector<TileZone> newZone;

		newContent.Resize(newSize.x * newSize.y);
		newZone.Resize(newSize.x * newSize.y);

		ZeroMemory(newContent.Data(), newContent.ByteSize());
		ZeroMemory(newZone.Data(), newZone.ByteSize());

		if (newSize.x && newSize.y && _size.x && _size.y)
		{
			int nCopy = std::min(newSize.x, _size.x);

			for (int y = 0, nNewOffset = 0, nOldOffset = 0;
				y < newSize.y && y < _size.y;
				y++, nNewOffset += newSize.x, nOldOffset += _size.x)
			{
				CopyMemory(newContent.Data(nNewOffset), _content.Data(nOldOffset), nCopy * sizeof(TileContent));
				CopyMemory(newZone.Data(nNewOffset), _zone.Data(nOldOffset), nCopy * sizeof(TileZone));
			}
		}

		_size = newSize;
		_content = newContent;
		_zone = newZone;

		for (size_t i = 0; i < _listeners.Size(); i++)
		{
			_listeners[i]->OnAllTilesChanged();
		}
	}
}

void Tiles::Shift(ff::PointInt shift)
{
	if (shift.x || shift.y)
	{
		ff::Vector<TileContent> newContent;
		ff::Vector<TileZone> newZone;

		newContent.Resize(_content.Size());
		newZone.Resize(_zone.Size());

		ZeroMemory(newContent.Data(), newContent.ByteSize());
		ZeroMemory(newZone.Data(), newZone.ByteSize());

		ff::PointInt copyTiles(_size.x - abs(shift.x), _size.y - abs(shift.y));

		if (copyTiles.x > 0 && copyTiles.y > 0)
		{
			int nOldOffset = (shift.x < 0) ? -shift.x : 0;
			int nNewOffset = (shift.x > 0) ? shift.x : 0;

			nOldOffset += (shift.y < 0) ? (-shift.y * _size.x) : 0;
			nNewOffset += (shift.y > 0) ? ( shift.y * _size.x) : 0;

			for (int y = 0; y < copyTiles.y; y++)
			{
				CopyMemory(newContent.Data(nNewOffset), _content.Data(nOldOffset), copyTiles.x * sizeof(TileContent));
				CopyMemory(newZone.Data(nNewOffset), _zone.Data(nOldOffset), copyTiles.x * sizeof(TileZone));
			}
		}

		_content = newContent;
		_zone = newZone;

		for (size_t i = 0; i < _listeners.Size(); i++)
		{
			_listeners[i]->OnAllTilesChanged();
		}
	}
}

TileContent Tiles::GetContent(ff::PointInt tile) const
{
	if (tile.x >= 0 && tile.x < _size.x &&
		tile.y >= 0 && tile.y < _size.y)
	{
		return _content.GetAt(tile.y * _size.x + tile.x);
	}

	return CONTENT_NOTHING;
}

void Tiles::SetContent(ff::PointInt tile, TileContent content)
{
	if (tile.x >= 0 && tile.x < _size.x &&
		tile.y >= 0 && tile.y < _size.y)
	{
		size_t nTile = tile.y * _size.x + tile.x;

		TileContent oldContent = _content[nTile];
		_content[nTile] = content;

		for (size_t i = 0; i < _listeners.Size(); i++)
		{
			_listeners[i]->OnTileChanged(tile, oldContent, content);
		}
	}
}

TileZone Tiles::GetZone(ff::PointInt tile) const
{
	if (tile.x >= 0 && tile.x < _size.x &&
		tile.y >= 0 && tile.y < _size.y)
	{
		return _zone.GetAt(tile.y * _size.x + tile.x);
	}

	return ZONE_OUT_OF_BOUNDS;
}

void Tiles::SetZone(ff::PointInt tile, TileZone zone)
{
	if (tile.x >= 0 && tile.x < _size.x &&
		tile.y >= 0 && tile.y < _size.y)
	{
		size_t nTile = tile.y * _size.x + tile.x;
		_zone.SetAt(nTile, zone);

		for (size_t i = 0; i < _listeners.Size(); i++)
		{
			_listeners[i]->OnTileChanged(tile, _content[nTile], _content[nTile]);
		}
	}
}

void Tiles::AddListener(IMazeListener *pListener)
{
	if (pListener && _listeners.Find(pListener) == ff::INVALID_SIZE)
	{
		_listeners.Push(pListener);
	}
	else
	{
		assert(false);
	}
}

void Tiles::RemoveListener(IMazeListener *pListener)
{
	size_t i = _listeners.Find(pListener);

	if (pListener && i != ff::INVALID_SIZE)
	{
		_listeners.Delete(i);
	}
	else
	{
		assert(false);
	}
}
