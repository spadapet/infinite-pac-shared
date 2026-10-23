# Infinite Pac Shared

Source code for *Infinite Pac Shared*, a desktop port of [Infinite Pac in the Microsoft Store](https://www.microsoft.com/store/apps/9nblggh4vbpc), with native Windows and web versions.

[Watch the game](https://www.youtube.com/watch?v=LZImeRnXGvA).

## Visual Studio

Open `maze.slnx` in **Visual Studio 2026 or later**.

- **Native Windows:** Install **Desktop development with C++** and a Windows SDK. Set **maze** as the startup project, select an **x64** configuration, build, and press **F5**. The solution includes the required `ff_game_library` projects.
- **Web:** Install Visual Studio's JavaScript/TypeScript project support and **Node.js 22.12+** (24 LTS recommended, on PATH). Restart Visual Studio after installing Node.js. Set **maze.web** as the startup project, keep **Build** and **Deploy** checked in Configuration Manager, and press **F5**. Dependencies restore on the first build; the app opens at http://127.0.0.1:5173. If the browser reports `ERR_CONNECTION_REFUSED`, check **Deploy** and retry.

## Web terminal commands

```powershell
Set-Location maze.web
npm ci
npm run dev
```

- **Production build:** `npm run build` produces `maze.web\dist`; deploy its entire contents to a static web host.
- **Preview:** `npm run preview` serves the production build at http://127.0.0.1:4173. Use a web server, not `file:` URLs.
- **Tests:** `npm test`; after building, `npm run test:browser` runs Playwright tests using installed Microsoft Edge.

## Copyright

Copyright 2026 Peter Spada