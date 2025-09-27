use colored::Colorize;
use std::{fs, path::Path, process::Command};

use crate::{Commands, Config};

const INFO_PLIST: &str = r#"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" 
 "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>((APPNAME))</string>

    <key>CFBundleDisplayName</key>
    <string>((APPNAME))</string>

    <key>CFBundleIdentifier</key>
    <string>com.((IDENTIFIER)).((APPNAMELC))</string>

    <key>CFBundleVersion</key>
    <string>((VERSION))</string>

    <key>CFBundleExecutable</key>
    <string>((EXECUTABLE))</string>

    <key>CFBundlePackageType</key>
    <string>APPL</string>

    <key>CFBundleIconFile</key>
    <string>AppIcon.icns</string>

    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
</dict>
</plist>

"#;

fn run_cmake(build_dir: &Path, release: bool) {
    let mut configure_cmd = Command::new("cmake");
    configure_cmd
        .current_dir(build_dir)
        .arg("..")
        .arg(if release {
            "-DCMAKE_BUILD_TYPE=Release"
        } else {
            "-DCMAKE_BUILD_TYPE=Debug"
        })
        .arg("-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=bin");

    let output = configure_cmd
        .output()
        .expect("Failed to execute cmake command");
    if !output.status.success() {
        eprintln!("{}", "CMake configuration failed!".italic().red());
        eprintln!("{}", String::from_utf8_lossy(&output.stderr));
        return;
    }
    println!("{}", "CMake configuration successful!".italic().green());

    let mut binding = Command::new("cmake");
    let build_cmd = binding
        .current_dir(build_dir)
        .arg("--build")
        .arg(".")
        .arg("--config")
        .arg(if release { "Release" } else { "Debug" });

    let output = build_cmd.output().expect("Failed to build project");
    if output.status.success() {
        println!("{}", "CMake build successful!".italic().green());
    } else {
        eprintln!("{}", "CMake build failed!".italic().red());
        eprintln!("{}", String::from_utf8_lossy(&output.stderr));
    }
}

fn clean() {
    if Path::new("build").exists() {
        std::fs::remove_dir_all("build").expect("Failed to remove build directory");
    }
    if Path::new("app").exists() {
        std::fs::remove_dir_all("app").expect("Failed to remove app directory");
    }
}

pub fn pack(cmd: Commands) {
    clean();
    println!(
        "{}",
        "Packing project for target machine.".italic().yellow()
    );

    let renderer: &str = "OpenGL";
    println!(
        "{} {}",
        "Using renderer:".italic().cyan(),
        renderer.bold().green()
    );

    let release = if let Commands::Pack { release } = cmd {
        release != 0
    } else {
        false
    };

    let build_dir = Path::new("build");
    if !build_dir.exists() {
        std::fs::create_dir(build_dir).expect("Failed to create build directory");
    }
    run_cmake(build_dir, release);
    println!("{}", "CMake build successful!".italic().green());

    let current_machine = if cfg!(target_os = "windows") {
        "windows"
    } else if cfg!(target_os = "macos") {
        "macos"
    } else if cfg!(target_os = "linux") {
        "linux"
    } else {
        "unknown"
    };

    let config_str = std::fs::read_to_string("atlas.toml").expect("Failed to read atlas.toml");
    let config: Config = toml::from_str(&config_str).expect("Failed to parse atlas.toml");
    if config.pack.supported_platforms != "all"
        && !config
            .pack
            .supported_platforms
            .split(',')
            .any(|platform| platform.trim() == current_machine)
    {
        eprintln!(
            "{}",
            format!("Current platform '{current_machine}' is not supported for packing.")
                .italic()
                .red()
        );
    }

    let built_files = std::fs::read_dir("build/bin").expect("Failed to read build directory");
    let mut executable_path: std::path::PathBuf = std::path::PathBuf::new();
    for path in built_files {
        let path = path.expect("Failed to read file in build directory").path();
        if path.is_file() {
            executable_path = path;
            println!(
                "{} {}",
                "Executable found at:".italic().cyan(),
                executable_path.display().to_string().bold().green()
            );
            break;
        } else {
            continue;
        }
    }
    if executable_path == std::path::PathBuf::new() {
        eprintln!(
            "{}",
            "No executable found in build directory.".italic().red()
        );
        return;
    }

    if current_machine == "macos" {
        let current_path = std::env::current_dir().unwrap();
        std::fs::create_dir(format!("{}/app", current_path.display())).unwrap();
        std::fs::create_dir_all(format!(
            "{}/app/{}.app/Contents/MacOS",
            current_path.display(),
            config
                .project
                .app_name
                .as_ref()
                .unwrap_or(&config.project.name)
        ))
        .unwrap();
        std::fs::create_dir_all(format!(
            "{}/app/{}.app/Contents/Resources",
            current_path.display(),
            config
                .project
                .app_name
                .as_ref()
                .unwrap_or(&config.project.name)
        ))
        .unwrap();
        if fs::metadata("assets/".to_string() + config.pack.icon.as_str()).is_ok() {
            let logo_path = "assets/".to_string() + config.pack.icon.as_str();
            std::fs::copy(
                logo_path,
                format!(
                    "{}/app/{}.app/Contents/Resources/AppIcon.icns",
                    current_path.display(),
                    config
                        .project
                        .app_name
                        .as_ref()
                        .unwrap_or(&config.project.name)
                ),
            )
            .unwrap();
        }
        std::fs::copy(
            executable_path,
            format!(
                "{}/app/{}.app/Contents/MacOS/{}",
                current_path.display(),
                config
                    .project
                    .app_name
                    .as_ref()
                    .unwrap_or(&config.project.name),
                config
                    .project
                    .app_name
                    .as_ref()
                    .unwrap_or(&config.project.name),
            ),
        )
        .unwrap();

        let plist = INFO_PLIST
            .replace(
                "((APPNAME))",
                config
                    .project
                    .app_name
                    .as_ref()
                    .unwrap_or(&config.project.name),
            )
            .replace(
                "((APPNAMELC))",
                &config
                    .project
                    .app_name
                    .as_ref()
                    .unwrap_or(&config.project.name)
                    .to_lowercase()
                    .replace(' ', "_"),
            )
            .replace(
                "((IDENTIFIER))",
                config
                    .pack
                    .identifier
                    .as_ref()
                    .unwrap_or(&String::from("example")),
            )
            .replace(
                "((VERSION))",
                config.pack.version.as_ref().unwrap_or(&String::from("1.0")),
            )
            .replace(
                "((EXECUTABLE))",
                config
                    .project
                    .app_name
                    .as_ref()
                    .unwrap_or(&config.project.name),
            );
        std::fs::write(
            format!(
                "{}/app/{}.app/Contents/Info.plist",
                current_path.display(),
                config
                    .project
                    .app_name
                    .as_ref()
                    .unwrap_or(&config.project.name)
            ),
            plist,
        )
        .unwrap();
    }

    println!("{}", "Packing completed successfully.".italic().green());
}
