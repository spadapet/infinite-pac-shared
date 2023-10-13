#include "pch.h"
#include "Core/Helpers.h"

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

ff::point_int PixelsPerTile()
{
    return ff::point_int(8, 8);
}

ff::point_float PixelsPerTileF()
{
    return ff::point_float(8, 8);
}

ff::point_int PixelToTile(ff::point_int pixel)
{
    return ff::point_int(pixel.x >> 3, pixel.y >> 3);
}

ff::point_int PixelAndDirToTile(ff::point_int pixel, ff::point_int dir)
{
    // This helps decide which tile an actor is really
    // in because it takes the direction into account and
    // avoid ambiguity at the edge of a tile.

    return PixelToTile(ff::point_int(
        ((pixel.x << 1) + dir.x) >> 1,
        ((pixel.y << 1) + dir.y) >> 1));
}

ff::point_int TileTopLeftToPixel(ff::point_int tile)
{
    return ff::point_int(tile.x << 3, tile.y << 3);
}

ff::point_float TileTopLeftToPixelF(ff::point_int tile)
{
    return ff::point_float((float)(tile.x << 3), (float)(tile.y << 3));
}

ff::point_int TileMiddleRightToPixel(ff::point_int tile)
{
    return ff::point_int((tile.x << 3) + 8, (tile.y << 3) + 4);
}

ff::point_float TileMiddleRightToPixelF(ff::point_int tile)
{
    return ff::point_float((tile.x << 3) + 8.0f, (tile.y << 3) + 4.0f);
}

ff::point_int TileBottomRightToPixel(ff::point_int tile)
{
    return ff::point_int((tile.x << 3) + 8, (tile.y << 3) + 8);
}

ff::point_float TileBottomRightToPixelF(ff::point_int tile)
{
    return ff::point_float((tile.x << 3) + 8.0f, (tile.y << 3) + 8.0f);
}

ff::point_int TileCenterToPixel(ff::point_int tile)
{
    return ff::point_int((tile.x << 3) + 4, (tile.y << 3) + 4);
}

ff::point_float TileCenterToPixelF(ff::point_int tile)
{
    return ff::point_float((tile.x << 3) + 4.0f, (tile.y << 3) + 4.0f);
}
