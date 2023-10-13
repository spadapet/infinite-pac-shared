#pragma once

class IPlayingMaze;

class IGhostBrains
{
public:
    static std::shared_ptr<IGhostBrains> Create(size_t nGhost);

    virtual ff::point_int Decide(IPlayingMaze* pPlay, const ff::point_int* pTiles, size_t nTiles) = 0;
    virtual ff::point_int GetTargetPixel(IPlayingMaze* pPlay) = 0;
};

ff::point_int DecideForTarget(ff::point_int targetPixel, const ff::point_int* pTiles, size_t nTiles);
