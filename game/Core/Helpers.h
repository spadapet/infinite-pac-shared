#pragma once

size_t IdealFramesPerSecond();
double IdealFramesPerSecondF();

size_t PacsPerSecond();
double PacsPerSecondF();

int Sign(int num);

ff::PointInt PixelsPerTile();
ff::PointInt PixelToTile(ff::PointInt pixel);
ff::PointInt PixelAndDirToTile(ff::PointInt pixel, ff::PointInt dir);
ff::PointInt TileTopLeftToPixel(ff::PointInt tile);
ff::PointInt TileMiddleRightToPixel(ff::PointInt tile);
ff::PointInt TileBottomRightToPixel(ff::PointInt tile);
ff::PointInt TileCenterToPixel(ff::PointInt tile);

ff::PointFloat PixelsPerTileF();
ff::PointFloat TileTopLeftToPixelF(ff::PointInt tile);
ff::PointFloat TileMiddleRightToPixelF(ff::PointInt tile);
ff::PointFloat TileBottomRightToPixelF(ff::PointInt tile);
ff::PointFloat TileCenterToPixelF(ff::PointInt tile);
