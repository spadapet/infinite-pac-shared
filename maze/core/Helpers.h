#pragma once

size_t IdealFramesPerSecond();
double IdealFramesPerSecondF();

size_t PacsPerSecond();
double PacsPerSecondF();

int Sign(int num);

ff::point_int PixelsPerTile();
ff::point_int PixelToTile(ff::point_int pixel);
ff::point_int PixelAndDirToTile(ff::point_int pixel, ff::point_int dir);
ff::point_int TileTopLeftToPixel(ff::point_int tile);
ff::point_int TileMiddleRightToPixel(ff::point_int tile);
ff::point_int TileBottomRightToPixel(ff::point_int tile);
ff::point_int TileCenterToPixel(ff::point_int tile);

ff::point_float PixelsPerTileF();
ff::point_float TileTopLeftToPixelF(ff::point_int tile);
ff::point_float TileMiddleRightToPixelF(ff::point_int tile);
ff::point_float TileBottomRightToPixelF(ff::point_int tile);
ff::point_float TileCenterToPixelF(ff::point_int tile);
