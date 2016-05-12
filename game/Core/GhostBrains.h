#pragma once

class IPlayingMaze;

class __declspec(uuid("7610f7de-7fe5-4c19-88ae-d2e4c8978aa2")) __declspec(novtable)
	IGhostBrains : public IUnknown
{
public:
	static bool Create(size_t nGhost, IGhostBrains **ppOut);

	virtual ff::PointInt Decide(IPlayingMaze *pPlay, const ff::PointInt *pTiles, size_t nTiles) = 0;
	virtual ff::PointInt GetTargetPixel(IPlayingMaze *pPlay) = 0;
};

ff::PointInt DecideForTarget(ff::PointInt targetPixel, const ff::PointInt *pTiles, size_t nTiles);
