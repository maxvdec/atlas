//
//  plugin.swift
//  Atlas
//
//  Created by Max Van den Eynde on 7/7/25.
//

import PackagePlugin

@main
struct GenerateShadersPlugin: BuildToolPlugin {
    func createBuildCommands(context: PluginContext, target: Target) throws -> [Command] {
        let metalDirectory = context.package.directoryURL.appending(path: "Shaders")

        let scriptPath = context.package.directoryURL.appending(path: "generate_shaders.sh")

        let outputSwiftFile = context.pluginWorkDirectoryURL.appending(path: "GeneratedShaders.swift")

        return [
            .prebuildCommand(
                displayName: "Generate Swift shaders from Metal files",
                executable: scriptPath,
                arguments: [metalDirectory.absoluteString, outputSwiftFile.absoluteString],
                environment: [:],
                outputFilesDirectory: context.pluginWorkDirectoryURL
            )
        ]
    }
}
