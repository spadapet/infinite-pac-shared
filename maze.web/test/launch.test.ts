import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

test('Visual Studio builds and deploys the web startup project in every solution configuration', async () => {
    const [solution, project] = await Promise.all([
        readFile(new URL('../../maze.sln', import.meta.url), 'utf8'),
        readFile(new URL('../maze.web.esproj', import.meta.url), 'utf8'),
    ]);
    const projectGuid = project.match(/<ProjectGuid>(\{[A-F\d-]+\})<\/ProjectGuid>/i)?.[1];
    assert.ok(projectGuid, 'The web project must declare its solution GUID');
    assert.ok(solution.includes(`"maze.web", "maze.web\\maze.web.esproj", "${projectGuid}"`));
    assert.match(project, /<StartupCommand>npm run dev<\/StartupCommand>/);

    const configurationSection = solution.match(/GlobalSection\(SolutionConfigurationPlatforms\)[\s\S]*?EndGlobalSection/)?.[0];
    assert.ok(configurationSection, 'The solution must declare configurations');
    const configurations = [...configurationSection.matchAll(/^\s*([^=|\r\n]+\|[^=\r\n]+?)\s*=\s*[^\r\n]+/gm)].map(match => match[1].trim());
    assert.ok(configurations.length > 0);
    const lines = solution.split(/\r?\n/).map(line => line.trim());
    for (const configuration of configurations) {
        const activePrefix = `${projectGuid}.${configuration}.ActiveCfg = `;
        const active = lines.find(line => line.startsWith(activePrefix))?.slice(activePrefix.length);
        assert.ok(active, `The web project must map ${configuration}`);
        assert.ok(lines.includes(`${projectGuid}.${configuration}.Build.0 = ${active}`), `Build must be enabled for ${configuration}`);
        assert.ok(lines.includes(`${projectGuid}.${configuration}.Deploy.0 = ${active}`), `Deploy must be enabled for ${configuration} so F5 starts Vite before Edge`);
    }
});
