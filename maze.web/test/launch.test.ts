import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

test('Visual Studio builds incrementally and deploys Vite in every solution configuration', async () => {
    const [solution, project] = await Promise.all([
        readFile(new URL('../../maze.slnx', import.meta.url), 'utf8'),
        readFile(new URL('../maze.web.esproj', import.meta.url), 'utf8'),
    ]);
    const webProject = solution.match(/<Project Path="maze\.web\/maze\.web\.esproj">([\s\S]*?)<\/Project>/)?.[1];
    assert.ok(webProject, 'The solution must include the web project');
    assert.match(webProject, /<Build\s*\/>/, 'Build must remain enabled');
    assert.match(webProject, /<Deploy\s*\/>/, 'Deploy must remain enabled so F5 starts Vite before Edge');
    assert.match(webProject, /<BuildType Solution="Profile\|\*" Project="Release"\s*\/>/);
    assert.deepEqual([...solution.matchAll(/<BuildType Name="([^"]+)"/g)].map(match => match[1]), ['Debug', 'Profile', 'Release']);
    assert.match(project, /<StartupCommand>npm run dev<\/StartupCommand>/);
    assert.match(project, /<BuildCommand>npm run build<\/BuildCommand>/);
    assert.match(project, /<ShouldRunBuildScript>false<\/ShouldRunBuildScript>/);
    const target = project.match(/<Target Name="BuildWebAssets"[\s\S]*?<\/Target>/)?.[0];
    assert.ok(target, 'The SDK build must be replaced with an incremental production target');
    assert.match(target, /BeforeTargets="CoreCompile"/);
    assert.match(target, /Inputs="@\(WebBuildInput\);/);
    assert.match(target, /Outputs="\$\(IntermediateOutputPath\)web-build\.stamp"/);
    assert.match(project, /<WebBuildMissingOutput Include="@\(WebBuildExpectedOutput\)" Condition="!Exists\('%\(Identity\)'\)"/);
    assert.match(target, /<Exec Command="\$\(BuildCommand\)"/);
    assert.match(project, /WriteOnlyWhenDifferent="true"/);
});
