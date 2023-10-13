#pragma once

class IPlayingMaze;
enum FruitType;

class IRenderMaze
{
public:
    virtual ~IRenderMaze() = default;

    static std::shared_ptr<IRenderMaze> Create(std::shared_ptr<IMaze> pMaze);

    virtual void Reset() = 0;

    virtual void Advance(bool bPac, bool bGhosts, bool bDots, IPlayingMaze* pPlay) = 0;

    virtual void RenderBackground(ff::dxgi::draw_base& draw) = 0;
    virtual void RenderTheMaze(ff::dxgi::draw_base& draw) = 0;
    virtual void RenderDots(ff::dxgi::draw_base& draw) = 0;
    virtual void RenderActors(ff::dxgi::draw_base& draw, bool bPac, bool bGhosts, bool bCustom, IPlayingMaze* pPlay) = 0;
    virtual void RenderPoints(ff::dxgi::draw_base& draw, IPlayingMaze* pPlay) = 0;

    virtual void RenderFreeLives(ff::dxgi::draw_base& draw, IPlayingMaze* play, size_t nLives, ff::point_float leftPixel) = 0;
    virtual void RenderStatusFruits(ff::dxgi::draw_base& draw, const FruitType* pTypes, size_t nCount, ff::point_float rightPixel) = 0;

    virtual std::shared_ptr<IMaze> GetMaze() const = 0;
};
