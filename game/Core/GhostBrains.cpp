#include "pch.h"
#include "COM/ComObject.h"
#include "Core/Actors.h"
#include "Core/GhostBrains.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/PlayingMaze.h"

class __declspec(uuid("2a242bcf-f45f-43de-8edd-bf382f1c61d4"))
	DefaultGhostBrains : public ff::ComBase, public IGhostBrains
{
public:
	DECLARE_HEADER(DefaultGhostBrains);

	bool Init(size_t nGhost);

	// IGhostBrains
	virtual ff::PointInt Decide(
		IPlayingMaze *pPlay,
		const ff::PointInt *pTiles,
		size_t nTiles) override;

	virtual ff::PointInt GetTargetPixel(IPlayingMaze *pPlay) override;

private:
	ff::PointInt GetScatterPixel(IPlayingMaze *pPlay);
	ff::PointInt GetChasePixel (IPlayingMaze *pPlay);

	size_t _nGhost;
};

// static
bool IGhostBrains::Create(size_t nGhost, IGhostBrains **ppOut)
{
	assertRetVal(ppOut, false);
	*ppOut = nullptr;

	ff::ComPtr<DefaultGhostBrains> pOut = new ff::ComObject<DefaultGhostBrains>;
	assertRetVal(pOut->Init(nGhost), false);

	*ppOut = ff::GetAddRef<IGhostBrains>(pOut);

	return true;
}

DefaultGhostBrains::DefaultGhostBrains()
	: _nGhost(0)
{
}

DefaultGhostBrains::~DefaultGhostBrains()
{
}

BEGIN_INTERFACES(DefaultGhostBrains)
	HAS_INTERFACE(IGhostBrains)
END_INTERFACES()

bool DefaultGhostBrains::Init(size_t nGhost)
{
	assertRetVal(nGhost >= 0 && nGhost < 4, false);

	_nGhost = nGhost;

	return true;
}

ff::PointInt DefaultGhostBrains::Decide(
		IPlayingMaze* pPlay,
		const ff::PointInt* pTiles,
		size_t nTiles)
{
	assertRetVal(pPlay && pPlay->GetMaze() && pTiles && nTiles, ff::PointInt(0, 0));

	GhostState state = pPlay->GetGhostState(_nGhost);

	switch (state)
	{
	default:
	case GHOST_CHASE:
	case GHOST_SCATTER:
	case GHOST_EYES:
		return DecideForTarget(GetTargetPixel(pPlay), pTiles, nTiles);

	case GHOST_SCARED:
	case GHOST_SCARED_FLASH:
		return pTiles[rand() % nTiles];
	}
}

ff::PointInt DefaultGhostBrains::GetTargetPixel(IPlayingMaze *pPlay)
{
	assertRetVal(pPlay && pPlay->GetMaze(), ff::PointInt(0, 0));

	switch (pPlay->GetGhostState(_nGhost))
	{
	default:
	case GHOST_CHASE:
		return GetChasePixel(pPlay);

	case GHOST_SCATTER:
		return GetScatterPixel(pPlay);

	case GHOST_EYES:
		return pPlay->GetGhostStartPixel();
	}
}

ff::PointInt DefaultGhostBrains::GetScatterPixel(IPlayingMaze *pPlay)
{
	ff::PointInt size = pPlay->GetMaze()->GetSizeInTiles();
	size_t nGhost = pPlay->GetDifficulty().HasRandomGhostMovement(pPlay->GetCharType())
		? (rand() % 4)
		: _nGhost;

	switch (nGhost)
	{
	default:
	case 0:
		return TileCenterToPixel(ff::PointInt(size.x - 3, -3));

	case 1:
		return TileCenterToPixel(ff::PointInt(2, -3));

	case 2:
		return TileCenterToPixel(ff::PointInt(size.x - 1, size.y + 1));

	case 3:
		return TileCenterToPixel(ff::PointInt(0, size.y + 1));
	}
}

static const size_t s_nMaxOrangeDist =
	(8 * PixelsPerTile().x) * (8 * PixelsPerTile().x) +
	(8 * PixelsPerTile().y) * (8 * PixelsPerTile().y);

ff::PointInt DefaultGhostBrains::GetChasePixel(IPlayingMaze *pPlay)
{
	ff::PointInt pac = pPlay->GetPac()->GetPixel();

	switch (_nGhost)
	{
		default:
		case 0:
			return pac;

		case 1:
		{
			ff::PointInt offset = pPlay->GetPac()->GetDir();
			offset.x *= PixelsPerTile().x * 4;
			offset.y *= PixelsPerTile().y * 4;

			return pac + offset;
		}

		case 2:
		{
			ff::PointInt offset = pac - pPlay->GetGhost(0)->GetPixel();

			return pac + offset;
		}

		case 3:
		{
			ff::PointInt dist = (pac - pPlay->GetGhost(3)->GetPixel());
			size_t nDist = dist.x * dist.x + dist.y * dist.y;

			return nDist >= s_nMaxOrangeDist ? pac : GetScatterPixel(pPlay);
		}
	}
}

ff::PointInt DecideForTarget(ff::PointInt targetPixel, const ff::PointInt *pTiles, size_t nTiles)
{
	assertRetVal(pTiles && nTiles, ff::PointInt(0, 0));

	// Find the shortest distance to the target pixel

	size_t nBestChoice = 0;
	size_t nBestDist = ff::INVALID_SIZE;

	for (size_t i = 0; i < nTiles; i++)
	{
		ff::PointInt dist = TileCenterToPixel(pTiles[i]) - targetPixel;
		size_t nDist = dist.x * dist.x + dist.y * dist.y;

		if (nDist < nBestDist)
		{
			nBestChoice = i;
			nBestDist = nDist;
		}
	}

	return pTiles[nBestChoice];
}
