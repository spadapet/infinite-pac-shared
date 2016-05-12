#pragma once

namespace ff
{
	class I2dRenderer;
}

class IPlayingMaze;
enum FruitType;

class __declspec(uuid("8bfc87b3-4a64-4202-b419-fc41aa6bb501")) __declspec(novtable)
	IRenderMaze : public IUnknown
{
public:
	static bool Create(IMaze *pMaze, IRenderMaze **ppRender);

	virtual void Reset() = 0;

	virtual void Advance(bool bPac, bool bGhosts, bool bDots, IPlayingMaze *pPlay) = 0;

	virtual void RenderBackground(ff::I2dRenderer *pRenderer) = 0;
	virtual void RenderTheMaze(ff::I2dRenderer *pRenderer) = 0;
	virtual void RenderDots(ff::I2dRenderer *pRenderer) = 0;
	virtual void RenderActors(ff::I2dRenderer *pRenderer, bool bPac, bool bGhosts, bool bCustom, IPlayingMaze *pPlay) = 0;
	virtual void RenderPoints(ff::I2dRenderer *pRenderer, IPlayingMaze *pPlay) = 0;

	virtual void RenderFreeLives(ff::I2dRenderer *pRenderer, IPlayingMaze *play, size_t nLives, ff::PointFloat leftPixel) = 0;
	virtual void RenderStatusFruits(ff::I2dRenderer *pRenderer, const FruitType *pTypes, size_t nCount, ff::PointFloat rightPixel) = 0;

	virtual IMaze *GetMaze() const = 0;
};
