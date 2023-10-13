#include "pch.h"
#include "Core/Actors.h"
#include "Core/GhostBrains.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/PlayingMaze.h"

class DefaultGhostBrains : public IGhostBrains
{
public:
    DefaultGhostBrains(size_t nGhost);

    // IGhostBrains
    virtual ff::point_int Decide(
        IPlayingMaze* pPlay,
        const ff::point_int* pTiles,
        size_t nTiles) override;

    virtual ff::point_int GetTargetPixel(IPlayingMaze* pPlay) override;

private:
    ff::point_int GetScatterPixel(IPlayingMaze* pPlay);
    ff::point_int GetChasePixel(IPlayingMaze* pPlay);

    size_t _nGhost{};
};

// static
std::shared_ptr<IGhostBrains> IGhostBrains::Create(size_t nGhost)
{
    return std::make_shared<DefaultGhostBrains>(nGhost);
}

DefaultGhostBrains::DefaultGhostBrains(size_t nGhost)
    : _nGhost(nGhost)
{
    assert(nGhost >= 0 && nGhost < 4);
}

ff::point_int DefaultGhostBrains::Decide(
        IPlayingMaze* pPlay,
        const ff::point_int* pTiles,
        size_t nTiles)
{
    assert_ret_val(pPlay && pPlay->GetMaze() && pTiles && nTiles, ff::point_int(0, 0));

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

ff::point_int DefaultGhostBrains::GetTargetPixel(IPlayingMaze* pPlay)
{
    assert_ret_val(pPlay && pPlay->GetMaze(), ff::point_int(0, 0));

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

ff::point_int DefaultGhostBrains::GetScatterPixel(IPlayingMaze* pPlay)
{
    ff::point_int size = pPlay->GetMaze()->GetSizeInTiles();
    size_t nGhost = pPlay->GetDifficulty().HasRandomGhostMovement(pPlay->GetCharType())
        ? (rand() % 4)
        : _nGhost;

    switch (nGhost)
    {
        default:
        case 0:
            return TileCenterToPixel(ff::point_int(size.x - 3, -3));

        case 1:
            return TileCenterToPixel(ff::point_int(2, -3));

        case 2:
            return TileCenterToPixel(ff::point_int(size.x - 1, size.y + 1));

        case 3:
            return TileCenterToPixel(ff::point_int(0, size.y + 1));
    }
}

static const size_t s_nMaxOrangeDist =
(8 * PixelsPerTile().x) * (8 * PixelsPerTile().x) +
(8 * PixelsPerTile().y) * (8 * PixelsPerTile().y);

ff::point_int DefaultGhostBrains::GetChasePixel(IPlayingMaze* pPlay)
{
    ff::point_int pac = pPlay->GetPac()->GetPixel();

    switch (_nGhost)
    {
        default:
        case 0:
            return pac;

        case 1:
            {
                ff::point_int offset = pPlay->GetPac()->GetDir();
                offset.x *= PixelsPerTile().x * 4;
                offset.y *= PixelsPerTile().y * 4;

                return pac + offset;
            }

        case 2:
            {
                ff::point_int offset = pac - pPlay->GetGhost(0)->GetPixel();

                return pac + offset;
            }

        case 3:
            {
                ff::point_int dist = (pac - pPlay->GetGhost(3)->GetPixel());
                size_t nDist = dist.x * dist.x + dist.y * dist.y;

                return nDist >= s_nMaxOrangeDist ? pac : GetScatterPixel(pPlay);
            }
    }
}

ff::point_int DecideForTarget(ff::point_int targetPixel, const ff::point_int* pTiles, size_t nTiles)
{
    assert_ret_val(pTiles && nTiles, ff::point_int(0, 0));

    // Find the shortest distance to the target pixel

    size_t nBestChoice = 0;
    size_t nBestDist = ff::constants::invalid_unsigned<size_t>();

    for (size_t i = 0; i < nTiles; i++)
    {
        ff::point_int dist = TileCenterToPixel(pTiles[i]) - targetPixel;
        size_t nDist = dist.x * dist.x + dist.y * dist.y;

        if (nDist < nBestDist)
        {
            nBestChoice = i;
            nBestDist = nDist;
        }
    }

    return pTiles[nBestChoice];
}
