import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..', '..');
const read = name => readFileSync(path.join(root, 'maze', 'core', name), 'utf8');
function functionBody(source, signature) {
    const start = source.indexOf(signature);
    if (start < 0) throw new Error(`Missing native function ${signature}`);
    const brace = source.indexOf('{', start);
    let depth = 1, end = brace + 1;
    while (depth && end < source.length) { if (source[end] === '{') depth++; else if (source[end] === '}') depth--; end++; }
    if (depth) throw new Error(`Unbalanced native function ${signature}`);
    return source.slice(start, end);
}
const header = readFileSync(path.join(here, 'movement-oracle.h'), 'utf8');
const actorHeader = read('Actors.h').split('class PointActor')[0].replace('#pragma once', '');
const actors = read('Actors.cpp').split('PointActor::PointActor')[0].replace(/^#include.*$/gm, '');
const helpers = read('Helpers.cpp').replace(/^#include.*$/gm, '');
const brains = read('GhostBrains.cpp').replace(/^#include.*$/gm, '');
const playing = read('PlayingMaze.cpp');
const methods = [
    'void PlayingMaze::AdvancePac(', 'void PlayingMaze::AdvanceGhost(', 'void PlayingMaze::AdvanceFruit(',
    'ff::point_int PlayingMaze::GhostDecidePress(GhostActor&', 'ff::point_int PlayingMaze::GhostDecidePress(IGhostBrains*',
    'bool PlayingMaze::CheckTunnel(', 'bool PlayingMaze::IsWall(', 'bool PlayingMaze::HitWall(',
    'void PlayingMaze::FlipGhosts(', 'void PlayingMaze::ScareGhosts(', 'bool PlayingMaze::ReleaseGhost(',
    'size_t PlayingMaze::CountGhostsInHouse(', 'void PlayingMaze::UpdateGhostMode(', 'void PlayingMaze::UpdateGhostHouse(', 'void PlayingMaze::UpdateActorSpeeds(',
];
const fruitBrains = playing.slice(playing.indexOf('class CFruitBrains'), playing.indexOf('bool PlayingMaze::IsWall'));
const difficulty = read('Difficulty.cpp');
const difficultyMethods = difficulty.slice(difficulty.indexOf('size_t Difficulty::GetPacSpeed'), difficulty.indexOf('void Difficulty::GetGhostModeFrames'));
const split = header.indexOf('class Maze');
const driver = readFileSync(path.join(here, 'movement-oracle-driver.cpp'), 'utf8');
let output = [header.slice(0, split), '#define rand() 0\ndouble oracleDraw = 0;\nsize_t oracleChoice(size_t count) { return static_cast<size_t>(oracleDraw * count); }', helpers, actorHeader, header.slice(split), actors, brains,
    ...methods.map(signature => functionBody(playing, signature)), fruitBrains, difficultyMethods, driver].join('\n\n');
for (const count of ['4', 'nTiles', 'tiles.size()']) output = output.replaceAll(`rand() % ${count}`, `oracleChoice(${count})`);
writeFileSync(path.join(here, 'movement-oracle.generated.cpp'), output);
console.log(`Generated native movement oracle directly from ${methods.length} PlayingMaze methods, Actors, Helpers, GhostBrains and fruit brains.`);
