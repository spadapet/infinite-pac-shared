#pragma once

class IMazeListener;

enum TileContent : BYTE
{
    CONTENT_NOTHING,
    CONTENT_WALL,
    CONTENT_GHOST_WALL,
    CONTENT_GHOST_DOOR,
    CONTENT_DOT,
    CONTENT_POWER,
    CONTENT_PAC_START,
    CONTENT_FRUIT_START,
};

enum TileZone : BYTE
{
    ZONE_NORMAL,
    ZONE_GHOST_SLOW, // ghosts go half speed over this tile
    ZONE_GHOST_NO_TURN, // ghosts can't change direction when on this tile
    ZONE_OUT_OF_BOUNDS, // outside the maze
};

class Tiles
{
public:
    Tiles();
    ~Tiles();

    std::shared_ptr<Tiles> Clone();

    REFGUID GetID() const;
    ff::point_int GetSize() const;
    void SetSize(ff::point_int size);
    void Shift(ff::point_int shift);

    TileContent GetContent(ff::point_int tile) const;
    void SetContent(ff::point_int tile, TileContent content);
    TileZone GetZone(ff::point_int tile) const;
    void SetZone(ff::point_int tile, TileZone zone);

    void AddListener(IMazeListener* pListener);
    void RemoveListener(IMazeListener* pListener);

private:
    GUID _id;
    ff::point_int _size;
    std::vector<TileZone> _zone;
    std::vector<TileContent> _content;
    std::vector<IMazeListener*> _listeners;
};

std::shared_ptr<Tiles> CreateTilesFromResource(std::string_view name);
