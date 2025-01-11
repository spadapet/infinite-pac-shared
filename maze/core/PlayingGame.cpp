#include "pch.h"
#include "Core/Actors.h"
#include "Core/Audio.h"
#include "Core/Helpers.h"
#include "Core/Maze.h"
#include "Core/Mazes.h"
#include "Core/PlayingGame.h"
#include "Core/PlayingMaze.h"
#include "Core/RenderMaze.h"
#include "Core/RenderText.h"

static const size_t INITIAL_LIVES = 3;

class Player : public IPlayer, public IPlayingMazeHost
{
public:
    Player(size_t nPlayer, std::shared_ptr<IMazes> pMazes);

    bool Advance();
    const std::vector<FruitType>& GetDisplayFruits();

    // IPlayer
    virtual size_t GetLevel() const override;
    virtual void SetLevel(size_t nLevel) override;
    virtual std::shared_ptr<IPlayingMaze> GetPlayingMaze() override;
    virtual size_t GetScore() const override;
    virtual size_t GetLives() const override;
    virtual bool IsGameOver() const override;
    virtual bool DidCheat() const override;
    virtual void AddStats(Stats& stats) const override;

    // IPlayingMazeHost
    virtual void OnStateChanged(GameState oldState, GameState newState) override;
    virtual size_t GetMazePlayer() override;
    virtual bool IsPlayingLevel() override;
    virtual bool IsEffectEnabled(AudioEffect effect) override;
    virtual void OnPacUsingTunnel() override;

private:
    void OnPacWon();
    void OnPacDied();
    void CheckFreeLife();

    bool _isGameOver{};
    Stats _stats{};
    size_t _level{};
    size_t _lives{ INITIAL_LIVES };
    size_t _nextFreeLife{ 10000 };
    size_t _freeLifeRepeat{};
    size_t _freeLivesLeft{ 1 };
    size_t _player{};
    std::shared_ptr<IPlayingMaze> _playMaze;
    std::shared_ptr<ISoundEffects> _sounds;
    std::shared_ptr<IMazes> _mazes;
    std::vector<FruitType> _displayFruits;
};

Player::Player(size_t nPlayer, std::shared_ptr<IMazes> pMazes)
    : _player(nPlayer)
    , _mazes(pMazes)
{
    _lives = _mazes->GetStartingLives();
    _nextFreeLife = _mazes->GetFreeLifeScore();
    _freeLifeRepeat = _mazes->GetFreeLifeRepeat();
    _freeLivesLeft = _mazes->GetMaxFreeLives();

    _stats._gamesStarted++;

    SetLevel(0);
}

bool Player::Advance()
{
    bool bDied = false;

    if (_playMaze)
    {
        _playMaze->Advance();

        CheckFreeLife();

        switch (_playMaze->GetGameState())
        {
            case GS_DIED:
                bDied = true;
                OnPacDied();
                break;

            case GS_WON:
                OnPacWon();
                break;
        }
    }

    return !_isGameOver && !bDied;
}

const std::vector<FruitType>& Player::GetDisplayFruits()
{
    return _displayFruits;
}

size_t Player::GetLevel() const
{
    return _level;
}

void Player::SetLevel(size_t nLevel)
{
    assert_ret(_mazes);

    _isGameOver = false;
    _level = nLevel;

    _displayFruits.clear();

    std::shared_ptr<IPlayingMaze> pPlayMaze;
    std::shared_ptr<ISoundEffects> pSounds;

    if (_mazes && _mazes->GetMazeCount())
    {
        size_t nMazeCount = _mazes->GetMazeCount();
        std::shared_ptr<IMaze> pMaze = _mazes->GetMaze(_level % nMazeCount);
        const Difficulty& diff = _mazes->GetDifficulty(_level);

        pPlayMaze = IPlayingMaze::Create(pMaze, diff, this);
        pSounds = ISoundEffects::Create(pMaze->GetCharType());

        FruitType prevFruit = FRUIT_NONE;

        for (size_t i = _level;
            i != ff::constants::invalid_unsigned<size_t>() && _displayFruits.size() < 7;
            i = ff::constants::previous_unsigned<size_t>(i))
        {
            FruitType fruit = _mazes->GetDifficulty(i).GetFruit();

            // Don't show two random fruits in a row (looks dumb)
            if (fruit != FRUIT_NONE && (fruit != FRUIT_RANDOM || fruit != prevFruit))
            {
                _displayFruits.push_back(fruit);
            }

            prevFruit = fruit;
        }
    }

    assert(pPlayMaze && pSounds);

    if (_playMaze)
    {
        // Save the old stats before they go away
        _stats += _playMaze->GetStats();
    }

    _playMaze = pPlayMaze;
    _sounds = pSounds;
}

std::shared_ptr<IPlayingMaze> Player::GetPlayingMaze()
{
    return _playMaze;
}

size_t Player::GetScore() const
{
    return _stats._score + (_playMaze ? _playMaze->GetStats()._score : 0);
}

size_t Player::GetLives() const
{
    return _lives;
}

bool Player::IsGameOver() const
{
    return _isGameOver;
}

bool Player::DidCheat() const
{
    return _stats._cheated || (_playMaze && _playMaze->GetStats()._cheated);
}

void Player::AddStats(Stats& stats) const
{
    stats += _stats;

    if (_playMaze)
    {
        stats += _playMaze->GetStats();
    }
}

void Player::OnPacWon()
{
    _stats._levelsBeaten++;

    SetLevel(_level + 1);
}

void Player::OnPacDied()
{
    if (_lives > 1)
    {
        _lives--;

        if (_playMaze)
        {
            _playMaze->Reset();
        }
    }
    else
    {
        _isGameOver = true;
    }
}

void Player::CheckFreeLife()
{
    if (_nextFreeLife && _freeLivesLeft && GetScore() >= _nextFreeLife)
    {
        if (_sounds && IsEffectEnabled(EFFECT_FREE_LIFE))
        {
            _sounds->Play(EFFECT_FREE_LIFE);
        }

        if (_lives <= INITIAL_LIVES)
        {
            _lives++;
            _freeLivesLeft--;
        }

        _nextFreeLife = _freeLifeRepeat ? _nextFreeLife + _freeLifeRepeat : 0;
    }

    if constexpr (ff::constants::debug_build)
    {
        static bool s_bCheating = false;

        if (ff::input::keyboard().pressing('0'))
        {
            _stats._cheated = true;

            if (!s_bCheating)
            {
                s_bCheating = true;

                _lives = std::min<size_t>(5, _lives + 1);
            }
        }
        else
        {
            s_bCheating = false;
        }

        if (ff::input::keyboard().pressing('9'))
        {
            _stats._cheated = true;
            _stats._score += 100;
        }

        if (ff::input::keyboard().pressing('8'))
        {
            // Doesn't count as cheating
            _lives = 1;
        }
    }
}

// IPlayingMazeHost
size_t Player::GetMazePlayer()
{
    return _player;
}

// IPlayingMazeHost
bool Player::IsPlayingLevel()
{
    return true;
}

// IPlayingMazeHost
bool Player::IsEffectEnabled(AudioEffect effect)
{
    if (effect == EFFECT_INTRO)
    {
        return !_level && !_player;
    }

    return true;
}

// IPlayingMazeHost
void Player::OnStateChanged(GameState oldState, GameState newState)
{
}

// IPlayingMazeHost
void Player::OnPacUsingTunnel()
{
}

class PlayingGame : public IPlayingGame
{
public:
    PlayingGame(std::shared_ptr<IMazes> pMazes, size_t nPlayers, IPlayingGameHost* pHost);

    // IPlayingGame

    virtual void Advance() override;
    virtual void Render(ff::dxgi::draw_base& draw) override;

    virtual std::shared_ptr<IMazes> GetMazes() override;
    virtual ff::point_int GetSizeInTiles() const override;
    virtual size_t GetHighScore() const override;

    virtual size_t GetPlayers() const override;
    virtual size_t GetCurrentPlayer() const override;
    virtual std::shared_ptr<IPlayer> GetPlayer(size_t nPlayer) override;

    virtual bool IsGameOver() const override;
    virtual bool IsPaused() const override;
    virtual void TogglePaused() override;
    virtual void PausedAdvance() override;

private:
    void InternalAdvance(bool bForce);
    void InternalAdvanceOne();

    IPlayingGameHost* _host{};

    std::shared_ptr<IMazes> _mazes;
    std::shared_ptr<IRenderText> _renderText;
    std::shared_ptr<Player> _players[2];

    bool _isGameOver{};
    bool _paused{};
    bool _singleAdvance{};
    bool _switchPlayer{};
    size_t _player{};
    size_t _counter{};
    size_t _gameOverCounter{};

    static const int _nScoreTiles = 3;
    static const int _nStatusTiles = 2;
};

std::shared_ptr<IPlayingGame> IPlayingGame::Create(std::shared_ptr<IMazes> pMazes, size_t nPlayers, IPlayingGameHost* pHost)
{
    return std::make_shared<PlayingGame>(pMazes, nPlayers, pHost);
}

PlayingGame::PlayingGame(std::shared_ptr<IMazes> pMazes, size_t nPlayers, IPlayingGameHost* pHost)
    : _host(pHost)
    , _mazes(pMazes)
    , _renderText(IRenderText::Create())
{
    assert(pMazes && nPlayers >= 1 && nPlayers <= _countof(_players));

    for (size_t i = 0; i < nPlayers; i++)
    {
        _players[i] = std::make_shared<Player>(i, pMazes);
    }
}

void PlayingGame::InternalAdvanceOne()
{
    std::shared_ptr<Player> pPlayer = _players[_player];

    if (_switchPlayer)
    {
        _switchPlayer = false;

        size_t nNewPlayer = !_player ? 1 : 0;
        Player* pOtherPlayer = _players[nNewPlayer].get();

        if (pOtherPlayer && !pOtherPlayer->IsGameOver())
        {
            _player = nNewPlayer;
            pPlayer = _players[_player];
        }
        else if (pPlayer->IsGameOver())
        {
            _isGameOver = true;
            _gameOverCounter = 1;
        }
    }

    if (_gameOverCounter)
    {
        _gameOverCounter++;

        if (!_isGameOver && _gameOverCounter > 120)
        {
            _gameOverCounter = 0;
            _switchPlayer = true; // switch during the NEXT advance

            if (_host)
            {
                _host->OnPlayerGameOver(this, pPlayer);
            }
        }
    }
    else if (pPlayer)
    {
        if (!pPlayer->Advance())
        {
            if (pPlayer->IsGameOver())
            {
                _gameOverCounter++;
            }
            else
            {
                _switchPlayer = true;
            }
        }
    }

    _counter++;
}

void PlayingGame::InternalAdvance(bool bForce)
{
    if (bForce || !_paused)
    {
        size_t speed = 1;

        if (ff::constants::debug_build && ff::input::keyboard().pressing(VK_INSERT))
        {
            // Is this cheating? If so, set _bCheated in the stats
            speed = 4;
        }

        for (; speed > 0; speed--)
        {
            InternalAdvanceOne();
        }
    }
}

void PlayingGame::Advance()
{
    InternalAdvance(false);
}

static const DirectX::XMFLOAT4 s_colorText(0.8706f, 0.8706f, 1, 1);
static const DirectX::XMFLOAT4 s_colorPaused(1, 0, 0, 0.5f);
static const DirectX::XMFLOAT4 s_colorGameOver(1, 0, 0, 1);
static const DirectX::XMFLOAT4 s_colorWhite(1, 1, 1, 1);
static const DirectX::XMFLOAT4 s_colorBlack(0, 0, 0, 1);
static const DirectX::XMFLOAT4 s_pausedFade(0, 0, 0, 0.5f);
static const DirectX::XMFLOAT4 s_colorPlayerText(0, 1, 1, 1);
static const DirectX::XMFLOAT4 s_colorReadyText(1, 1, 0, 1);

void PlayingGame::Render(ff::dxgi::draw_base& draw)
{
    ff::point_int totalTiles = GetSizeInTiles();
    bool bShowScores = (!_host || _host->IsShowingScoreBar(this));
    bool bShowStatus = (!_host || _host->IsShowingStatusBar(this));
    std::shared_ptr<IPlayingMaze> pPlayMaze = _players[_player]->GetPlayingMaze();

    if (bShowScores)
    {
        std::string szScore0 = _players[0] ? FormatScoreAsString(_players[0]->GetScore()) : std::string();
        std::string szScore1 = _players[1] ? FormatScoreAsString(_players[1]->GetScore()) : std::string();
        std::string szHighScore = FormatScoreAsString(GetHighScore());
        ff::point_float tileSize = PixelsPerTileF();
        bool bNameVisible = _isGameOver || _switchPlayer || (_counter % 30) < 15;

        if ((bNameVisible || _player != 0) && _players[0])
        {
            _renderText->DrawText(draw, "1UP", ff::point_float(3 * tileSize.x, 0), 0, &s_colorText, nullptr, nullptr);
        }

        if ((bNameVisible || _player != 1) && _players[1])
        {
            _renderText->DrawText(draw, "2UP", ff::point_float(tileSize.x * (totalTiles.x - 6), 0), 0, &s_colorText, nullptr, nullptr);
        }

        _renderText->DrawText(draw, szScore0.c_str(), ff::point_float(0, tileSize.y), 0, &s_colorText, nullptr, nullptr);
        _renderText->DrawText(draw, szScore1.c_str(), ff::point_float(tileSize.x * (totalTiles.x - 8), tileSize.y), 0, &s_colorText, nullptr, nullptr);

        _renderText->DrawText(draw, "HIGH SCORE", ff::point_float(9 * tileSize.x, 0), 0, &s_colorText, nullptr, nullptr);
        _renderText->DrawText(draw, szHighScore.c_str(), ff::point_float(10 * tileSize.x, tileSize.y), 0, &s_colorText, nullptr, nullptr);

        if (_singleAdvance)
        {
            std::string szFrame = FormatScoreAsString(_counter);
            _renderText->DrawText(draw, szFrame.c_str(), ff::point_float(0, 2 * tileSize.y), 0, &s_colorPaused, nullptr, nullptr);

            if (pPlayMaze)
            {
                ff::point_int pix = pPlayMaze->GetPac()->GetPixel();

                if (pix.x >= 0 && pix.y >= 0)
                {
                    std::string szX = FormatScoreAsString((size_t)pix.x);
                    std::string szY = FormatScoreAsString((size_t)pix.y);

                    _renderText->DrawText(draw, szX.c_str(), ff::point_float(17 * tileSize.x, 2 * tileSize.y), 0, &s_colorPaused, nullptr, nullptr);
                    _renderText->DrawText(draw, szY.c_str(), ff::point_float(21 * tileSize.x, 2 * tileSize.y), 0, &s_colorPaused, nullptr, nullptr);
                }
            }
        }
    }

    size_t nLives = _players[_player]->GetLives();

    if (bShowStatus && pPlayMaze && nLives > 0)
    {
        size_t nRenderLives = (pPlayMaze->GetGameState() >= GS_READY)
            ? ff::constants::previous_unsigned<size_t>(nLives)
            : nLives;

        if (nRenderLives > 0)
        {
            nRenderLives = std::min<size_t>(5, nRenderLives);

            pPlayMaze->GetRenderMaze()->RenderFreeLives(
                draw,
                pPlayMaze.get(),
                nRenderLives,
                TileTopLeftToPixelF(ff::point_int(3, totalTiles.y - 1)));
        }
    }

    if (bShowStatus && pPlayMaze && _players[_player]->GetDisplayFruits().size())
    {
        pPlayMaze->GetRenderMaze()->RenderStatusFruits(
            draw,
            _players[_player]->GetDisplayFruits().data(),
            _players[_player]->GetDisplayFruits().size(),
            TileTopLeftToPixelF(ff::point_int(totalTiles.x - 3, totalTiles.y - 1)));
    }

    if (pPlayMaze)
    {
        if (bShowScores)
        {
            draw.world_matrix_stack().push();
            DirectX::XMFLOAT4X4 matrix;
            DirectX::XMStoreFloat4x4(&matrix, DirectX::XMMatrixTranslation(0, _nScoreTiles * PixelsPerTileF().y, 0));
            draw.world_matrix_stack().transform(matrix);
        }

        pPlayMaze->Render(draw);

        if (bShowScores)
        {
            draw.world_matrix_stack().pop();
        }
    }

    if ((_isGameOver || _gameOverCounter) && pPlayMaze)
    {
        ff::point_int doorTile = pPlayMaze->GetGhostStartTile();
        ff::point_float textPos;

        if (!_isGameOver)
        {
            textPos = TileTopLeftToPixelF(doorTile + ff::point_int(-4, 0 + (bShowScores ? _nScoreTiles : 0)));

            _renderText->DrawText(
                draw,
                !_player ? "PLAYER ONE" : "PLAYER TWO",
                textPos, 0,
                &s_colorGameOver,
                nullptr, nullptr);
        }

        textPos = TileTopLeftToPixelF(doorTile + ff::point_int(-4, 6 + (bShowScores ? _nScoreTiles : 0)));

        _renderText->DrawText(
            draw,
            "GAME OVER",
            textPos, 0,
            &s_colorGameOver,
            nullptr, nullptr);
    }

    if (pPlayMaze->GetGameState() <= GS_READY)
    {
        // Render intro text

        if (pPlayMaze->GetGameState() == GS_PLAYER_READY)
        {
            ff::point_int tile = pPlayMaze->GetGhostStartTile() + ff::point_int(-4, bShowScores ? _nScoreTiles : 0);

            _renderText->DrawText(
                draw,
                (_player == 1) ? "PLAYER TWO" : "PLAYER ONE",
                TileTopLeftToPixelF(tile), 0,
                &s_colorPlayerText,
                nullptr, nullptr);

            tile = pPlayMaze->GetGhostStartTile() + ff::point_int(-3, 6 + (bShowScores ? _nScoreTiles : 0));

            char szLevelText[12];
            _snprintf_s(szLevelText, _TRUNCATE,
                "LEVEL %02Iu",
                _players[_player]->GetLevel() + 1);

            _renderText->DrawText(
                draw,
                szLevelText,
                TileTopLeftToPixelF(tile), 0,
                &s_colorReadyText,
                nullptr, nullptr);
        }
        else if (pPlayMaze->GetGameState() == GS_READY)
        {
            ff::point_int tile = pPlayMaze->GetGhostStartTile() + ff::point_int(-2, 6 + (bShowScores ? _nScoreTiles : 0));

            _renderText->DrawText(
                draw,
                "READY!",
                TileTopLeftToPixelF(tile), 0,
                &s_colorReadyText,
                nullptr, nullptr);
        }
    }

    if (_paused)
    {
        if (!_singleAdvance)
        {
            // Fade the screen
            draw.draw_rectangle(ff::rect_float(ff::point_float(0, 0), TileTopLeftToPixelF(totalTiles)), s_pausedFade);
        }

        if (pPlayMaze)
        {
            ff::point_int doorTile = pPlayMaze->GetGhostStartTile();
            ff::point_float pausedPos = TileTopLeftToPixelF(doorTile + ff::point_int(-5, 4 + (bShowScores ? _nScoreTiles : 0)));
            const char* szPaused = "PAUSED";
            ff::point_float scale(2, 2);

            _renderText->DrawText(
                draw,
                szPaused,
                pausedPos + ff::point_float(-0.5f, -0.5f), 0,
                &s_colorBlack,
                nullptr, &scale);

            _renderText->DrawText(
                draw,
                szPaused,
                pausedPos + ff::point_float(0.5f, 0.5f), 0,
                &s_colorWhite,
                nullptr, &scale);

            _renderText->DrawText(
                draw,
                szPaused,
                pausedPos, 0,
                &s_colorPaused,
                nullptr, &scale);
        }
    }
}

std::shared_ptr<IMazes> PlayingGame::GetMazes()
{
    return _mazes;
}

ff::point_int PlayingGame::GetSizeInTiles() const
{
    ff::point_int size(16, 16);
    std::shared_ptr<IPlayingMaze> pPlayMaze = _players[_player]->GetPlayingMaze();

    if (pPlayMaze)
    {
        ff::point_int mazeSize = pPlayMaze->GetMaze()->GetSizeInTiles();

        int extraHeight = 0;

        if (!_host || _host->IsShowingScoreBar(const_cast<PlayingGame*>(this)))
        {
            extraHeight += _nScoreTiles;
        }

        if (!_host || _host->IsShowingStatusBar(const_cast<PlayingGame*>(this)))
        {
            extraHeight += _nStatusTiles;
        }

        size.x = std::max(size.x, mazeSize.x);
        size.y = std::max(size.y, mazeSize.y + extraHeight);
    }

    return size;
}

size_t PlayingGame::GetHighScore() const
{
    size_t nScore = 0;

    for (size_t i = 0; i < _countof(_players); i++)
    {
        if (_players[i])
        {
            const Stats& stats = Stats::Get(_mazes->GetID());

            nScore = std::max<size_t>(nScore, _players[i]->GetScore());
            nScore = std::max<size_t>(nScore, stats._highScores[0]._score);
        }
    }

    return nScore;
}

size_t PlayingGame::GetPlayers() const
{
    if (_players[1])
    {
        return 2;
    }

    if (_players[0])
    {
        return 1;
    }

    assert_ret_val(false, 0);
}

size_t PlayingGame::GetCurrentPlayer() const
{
    return _player;
}

std::shared_ptr<IPlayer> PlayingGame::GetPlayer(size_t nPlayer)
{
    return (nPlayer >= 0 && nPlayer < _countof(_players)) ? _players[nPlayer] : nullptr;
}

bool PlayingGame::IsGameOver() const
{
    return _isGameOver && _gameOverCounter > 120;
}

bool PlayingGame::IsPaused() const
{
    return _paused;
}

void PlayingGame::TogglePaused()
{
    _paused = !_paused;

    _singleAdvance = false;
}

void PlayingGame::PausedAdvance()
{
    if (ff::constants::debug_build && _paused)
    {
        _singleAdvance = true;

        InternalAdvance(true);
    }
}
