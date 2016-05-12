#pragma once

namespace ff
{
	class ISavedData;
}

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

class __declspec(uuid("5eb833bf-2940-4448-960e-fca3f09c0c72"))
	Tiles : public IUnknown, public ff::ComBase
{
public:
	DECLARE_HEADER(Tiles);

	bool Clone(Tiles **ppTiles);

	REFGUID GetID() const;
	ff::PointInt GetSize() const;
	void SetSize(ff::PointInt size);
	void Shift(ff::PointInt shift);

	TileContent GetContent(ff::PointInt tile) const;
	void SetContent(ff::PointInt tile, TileContent content);
	TileZone GetZone(ff::PointInt tile) const;
	void SetZone(ff::PointInt tile, TileZone zone);

	void AddListener(IMazeListener *pListener);
	void RemoveListener(IMazeListener *pListener);

private:
	GUID _id;
	ff::PointInt _size;
	ff::Vector<TileZone> _zone;
	ff::Vector<TileContent> _content;
	ff::Vector<IMazeListener*> _listeners;
};

bool CreateTilesFromResource(ff::String name, Tiles **obj);
