void emit(PlayingActor& actor) {
    auto p = actor.GetPixel(), d = actor.GetDir(), press = actor.GetPressDir();
    std::cout << p.x << ' ' << p.y << ' ' << d.x << ' ' << d.y << ' ' << press.x << ' ' << press.y;
}
int main() {
    PlayingMaze game;
    for (size_t i = 0; i < 4; ++i) game._ghosts[i]->SetBrains(IGhostBrains::Create(i));
    std::string command;
    while (std::cin >> command) {
        if (command == "M") {
            std::cin >> game._maze->width >> game._maze->height >> game._ghostStartPixel.x >> game._ghostStartPixel.y >> game._ghostHouseRect.top >> game._ghostHouseRect.bottom;
            const int count = game._maze->width * game._maze->height;
            game._maze->contents.resize(count); game._maze->zones.resize(count);
            for (int& value : game._maze->contents) std::cin >> value;
            for (int& value : game._maze->zones) std::cin >> value;
        } else if (command == "R") {
            std::cin >> oracleDraw;
        } else if (command == "P") {
            int x, y, dx, dy, px, py, turn;
            std::cin >> x >> y >> dx >> dy >> px >> py >> turn;
            auto& actor = *game._pac;
            actor.SetDir({dx,dy}); actor.SetPixel({x,y}); actor.SetPressDir({px,py}); actor.SetCanTurn(turn != 0);
            game.AdvancePac(actor);
            emit(actor); std::cout << ' ' << actor.CanTurn() << ' ' << actor.IsStuck() << '\n';
        } else if (command == "G") {
            int index, x, y, dx, dy, px, py, move, house, pacx, pacy, pacdx, pacdy, redx, redy, scatter, ms;
            std::cin >> index >> x >> y >> dx >> dy >> px >> py >> move >> house >> pacx >> pacy >> pacdx >> pacdy >> redx >> redy >> scatter >> ms;
            auto& actor = *game._ghosts[index];
            game._pac->SetPixel({pacx,pacy}); game._pac->SetDir({pacdx,pacdy});
            if (index != 0) game._ghosts[0]->SetPixel({redx,redy});
            actor.SetDir({dx,dy}); actor.SetPixel({x,y}); actor.SetPressDir({px,py}); actor.SetMoveState(static_cast<MoveState>(move)); actor.SetHouseState(static_cast<HouseState>(house));
            game._ghostScatterCountdown = scatter; game.character = static_cast<CharType>(ms);
            srand(0);
            game.AdvanceGhost(actor);
            emit(actor); std::cout << ' ' << actor.GetMoveState() << ' ' << actor.GetHouseState() << '\n';
        } else if (command == "F") {
            int x, y, dx, dy, exitx, exity, exiting;
            std::cin >> x >> y >> dx >> dy >> exitx >> exity >> exiting;
            game._fruit->Reset(); game._fruit->SetActive(true); game._fruit->SetPixel({x,y}); game._fruit->SetDir({dx,dy}); game._fruit->SetExitTile({exitx,exity}); game.exiting = exiting != 0;
            srand(0);
        } else if (command == "f") {
            game.AdvanceFruit(*game._fruit); emit(*game._fruit); std::cout << ' ' << game._fruit->IsActive() << '\n';
        } else { std::cerr << "Unknown command " << command; return 2; }
    }
}
