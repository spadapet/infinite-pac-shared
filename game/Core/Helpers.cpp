#include "pch.h"

// These are the arcade pac speeds

static double s_fFps = 60.0;
static double s_fPps = 75.5;

static size_t s_nFps = 60;
static size_t s_nPps = 75;

size_t IdealFramesPerSecond()
{
	return s_nFps;
}

double IdealFramesPerSecondF()
{
	return s_fFps;
}

size_t PacsPerSecond()
{
	return s_nPps;
}

double PacsPerSecondF()
{
	return s_fPps;
}

int Sign(int num)
{
	if (num < 0)
	{
		return -1;
	}

	if (num > 0)
	{
		return 1;
	}

	return 0;
}

ff::PointInt PixelsPerTile()
{
	return ff::PointInt(8, 8);
}

ff::PointFloat PixelsPerTileF()
{
	return ff::PointFloat(8, 8);
}

ff::PointInt PixelToTile(ff::PointInt pixel)
{
	return ff::PointInt(pixel.x >> 3, pixel.y >> 3);
}

ff::PointInt PixelAndDirToTile(ff::PointInt pixel, ff::PointInt dir)
{
	// This helps decide which tile an actor is really
	// in because it takes the direction into account and
	// avoid ambiguity at the edge of a tile.

	return PixelToTile(ff::PointInt(
		((pixel.x << 1) + dir.x) >> 1,
		((pixel.y << 1) + dir.y) >> 1));
}

ff::PointInt TileTopLeftToPixel(ff::PointInt tile)
{
	return ff::PointInt(tile.x << 3, tile.y << 3);
}

ff::PointFloat TileTopLeftToPixelF(ff::PointInt tile)
{
	return ff::PointFloat((float)(tile.x << 3), (float)(tile.y << 3));
}

ff::PointInt TileMiddleRightToPixel(ff::PointInt tile)
{
	return ff::PointInt((tile.x << 3) + 8, (tile.y << 3) + 4);
}

ff::PointFloat TileMiddleRightToPixelF(ff::PointInt tile)
{
	return ff::PointFloat((tile.x << 3) + 8.0f, (tile.y << 3) + 4.0f);
}

ff::PointInt TileBottomRightToPixel(ff::PointInt tile)
{
	return ff::PointInt((tile.x << 3) + 8, (tile.y << 3) + 8);
}

ff::PointFloat TileBottomRightToPixelF(ff::PointInt tile)
{
	return ff::PointFloat((tile.x << 3) + 8.0f, (tile.y << 3) + 8.0f);
}

ff::PointInt TileCenterToPixel(ff::PointInt tile)
{
	return ff::PointInt((tile.x << 3) + 4, (tile.y << 3) + 4);
}

ff::PointFloat TileCenterToPixelF(ff::PointInt tile)
{
	return ff::PointFloat((tile.x << 3) + 4.0f, (tile.y << 3) + 4.0f);
}
