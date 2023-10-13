#include "pch.h"
#include "Core/Tiles.h"
#include "Core/Maze.h"

static std::shared_ptr<Tiles> CreateTilesFromString(const char* szTiles, ff::point_int size, bool bMirror)
{
    assert_ret_val(szTiles && size.x > 0 && size.y > 0, nullptr);
    assert_ret_val(!bMirror || !(size.x % 2), nullptr);

    std::shared_ptr<Tiles> pTiles = std::make_shared<Tiles>();
    pTiles->SetSize(size);

    const char* sz = szTiles;

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

            pTiles->SetContent(ff::point_int(x, y), content);
            pTiles->SetZone(ff::point_int(x, y), zone);

            if (bMirror)
            {
                pTiles->SetContent(ff::point_int(size.x - 1 - x, y), content);
                pTiles->SetZone(ff::point_int(size.x - 1 - x, y), zone);
            }
        }
    }

    return pTiles;
}

std::shared_ptr<Tiles> CreateTilesFromResource(std::string_view name)
{
    ff::auto_resource<ff::resource_values> values_tiles("values_tiles");
    ff::value_ptr rawValue = values_tiles->get_resource_value(name);
    assert_ret_val(rawValue, nullptr);

    ff::value_ptr dictValue = rawValue->try_convert<ff::dict>();
    assert_ret_val(dictValue, nullptr);

    const ff::dict& dict = dictValue->get<ff::dict>();
    rawValue = dict.get("half-tiles");
    assert_ret_val(rawValue, nullptr);

    ff::value_ptr stringsValue = rawValue->try_convert<std::vector<std::string>>();
    assert_ret_val(stringsValue, nullptr);

    const std::vector<std::string>& strings = stringsValue->get<std::vector<std::string>>();
    assert_ret_val(strings.size(), nullptr);

    size_t length = strings[0].size();
    std::string combinedString;
    combinedString.reserve(length * strings.size());

    for (std::string_view str : strings)
    {
        assert_ret_val(str.size() == length, nullptr);
        combinedString += str;
    }

    return CreateTilesFromString(
        combinedString.c_str(),
        ff::point_int((int)length * 2, (int)strings.size()),
        true); // mirror half tiles
}

Tiles::Tiles()
    : _size(0, 0)
{
    ::CoCreateGuid(&_id);
}

Tiles::~Tiles()
{
    assert(!_listeners.size());
}

std::shared_ptr<Tiles> Tiles::Clone()
{
    std::shared_ptr<Tiles> pTiles = std::make_shared<Tiles>();
    pTiles->_size = _size;
    pTiles->_content = _content;
    pTiles->_zone = _zone;
    return pTiles;
}

REFGUID Tiles::GetID() const
{
    return _id;
}

ff::point_int Tiles::GetSize() const
{
    return _size;
}

void Tiles::SetSize(ff::point_int newSize)
{
    if (newSize != _size)
    {
        std::vector<TileContent> newContent;
        std::vector<TileZone> newZone;

        newContent.resize(newSize.x * newSize.y);
        newZone.resize(newSize.x * newSize.y);

        ZeroMemory(newContent.data(), ff::vector_byte_size(newContent));
        ZeroMemory(newZone.data(), ff::vector_byte_size(newZone));

        if (newSize.x && newSize.y && _size.x && _size.y)
        {
            int nCopy = std::min(newSize.x, _size.x);

            for (int y = 0, nNewOffset = 0, nOldOffset = 0;
                y < newSize.y && y < _size.y;
                y++, nNewOffset += newSize.x, nOldOffset += _size.x)
            {
                CopyMemory(newContent.data() + nNewOffset, _content.data() + nOldOffset, nCopy * sizeof(TileContent));
                CopyMemory(newZone.data() + nNewOffset, _zone.data() + nOldOffset, nCopy * sizeof(TileZone));
            }
        }

        _size = newSize;
        _content = newContent;
        _zone = newZone;

        for (size_t i = 0; i < _listeners.size(); i++)
        {
            _listeners[i]->OnAllTilesChanged();
        }
    }
}

void Tiles::Shift(ff::point_int shift)
{
    if (shift.x || shift.y)
    {
        std::vector<TileContent> newContent;
        std::vector<TileZone> newZone;

        newContent.resize(_content.size());
        newZone.resize(_zone.size());

        ZeroMemory(newContent.data(), ff::vector_byte_size(newContent));
        ZeroMemory(newZone.data(), ff::vector_byte_size(newZone));

        ff::point_int copyTiles(_size.x - abs(shift.x), _size.y - abs(shift.y));

        if (copyTiles.x > 0 && copyTiles.y > 0)
        {
            int nOldOffset = (shift.x < 0) ? -shift.x : 0;
            int nNewOffset = (shift.x > 0) ? shift.x : 0;

            nOldOffset += (shift.y < 0) ? (-shift.y * _size.x) : 0;
            nNewOffset += (shift.y > 0) ? (shift.y * _size.x) : 0;

            for (int y = 0; y < copyTiles.y; y++)
            {
                CopyMemory(newContent.data() + nNewOffset, _content.data() + nOldOffset, copyTiles.x * sizeof(TileContent));
                CopyMemory(newZone.data() + nNewOffset, _zone.data() + nOldOffset, copyTiles.x * sizeof(TileZone));
            }
        }

        _content = newContent;
        _zone = newZone;

        for (size_t i = 0; i < _listeners.size(); i++)
        {
            _listeners[i]->OnAllTilesChanged();
        }
    }
}

TileContent Tiles::GetContent(ff::point_int tile) const
{
    if (tile.x >= 0 && tile.x < _size.x &&
        tile.y >= 0 && tile.y < _size.y)
    {
        return _content.at(tile.y * _size.x + tile.x);
    }

    return CONTENT_NOTHING;
}

void Tiles::SetContent(ff::point_int tile, TileContent content)
{
    if (tile.x >= 0 && tile.x < _size.x &&
        tile.y >= 0 && tile.y < _size.y)
    {
        size_t nTile = tile.y * _size.x + tile.x;

        TileContent oldContent = _content[nTile];
        _content[nTile] = content;

        for (size_t i = 0; i < _listeners.size(); i++)
        {
            _listeners[i]->OnTileChanged(tile, oldContent, content);
        }
    }
}

TileZone Tiles::GetZone(ff::point_int tile) const
{
    if (tile.x >= 0 && tile.x < _size.x &&
        tile.y >= 0 && tile.y < _size.y)
    {
        return _zone.at(tile.y * _size.x + tile.x);
    }

    return ZONE_OUT_OF_BOUNDS;
}

void Tiles::SetZone(ff::point_int tile, TileZone zone)
{
    if (tile.x >= 0 && tile.x < _size.x &&
        tile.y >= 0 && tile.y < _size.y)
    {
        size_t nTile = tile.y * _size.x + tile.x;
        _zone[nTile] = zone;

        for (size_t i = 0; i < _listeners.size(); i++)
        {
            _listeners[i]->OnTileChanged(tile, _content[nTile], _content[nTile]);
        }
    }
}

void Tiles::AddListener(IMazeListener* pListener)
{
    if (pListener && std::find(_listeners.begin(), _listeners.end(), pListener) == _listeners.end())
    {
        _listeners.push_back(pListener);
    }
    else
    {
        assert(false);
    }
}

void Tiles::RemoveListener(IMazeListener* pListener)
{
    auto i = std::find(_listeners.begin(), _listeners.end(), pListener);

    if (pListener && i != _listeners.end())
    {
        _listeners.erase(i);
    }
    else
    {
        assert(false);
    }
}
