use crate::Commands;
use colored::Colorize;
use dialoguer::theme::ColorfulTheme;
use dialoguer::{Confirm, Input, Select};
use indicatif::{ProgressBar, ProgressStyle};
use reqwest::blocking::Client;
use serde::Deserialize;
use std::fs;
use std::io::Write;
use std::path::{Path, PathBuf};

#[derive(Deserialize)]
struct RemoteFile {
    name: String,
    path: String,
    #[serde(rename = "type")]
    file_type: String,
    download_url: Option<String>,
}

#[derive(Deserialize, Clone)]
struct ReleaseAsset {
    name: String,
    #[serde(rename = "browser_download_url")]
    download_url: String,
}

#[derive(Deserialize, Clone)]
struct Release {
    tag_name: String,
    assets: Vec<ReleaseAsset>,
}

#[derive(Clone)]
struct AtlasBinarySet {
    metal: Option<ReleaseAsset>,
    opengl: Option<ReleaseAsset>,
    vulkan: Option<ReleaseAsset>,
}

fn normalize_backend(input: &str) -> Option<String> {
    let normalized = input.trim().to_uppercase();
    match normalized.as_str() {
        "METAL" | "OPENGL" | "VULKAN" => Some(normalized),
        _ => None,
    }
}

fn normalize_platform(input: &str) -> Option<String> {
    let normalized = input.trim().to_lowercase();
    match normalized.as_str() {
        "macos" | "windows" | "linux" => Some(normalized),
        _ => None,
    }
}

fn fetch_folder(
    owner: &str,
    repo: &str,
    path: &str,
    outdir: &Path,
    branch: &str,
    exclude_subpaths: &[&str],
) -> Result<i32, Box<dyn std::error::Error>> {
    let url = format!("https://api.github.com/repos/{owner}/{repo}/contents/{path}?ref={branch}");
    let client = Client::new();
    let files: Vec<RemoteFile> = client
        .get(&url)
        .header("User-Agent", "atlas-cli")
        .send()?
        .json()?;

    fs::create_dir_all(outdir)?;

    let mut downloaded_files = 0;

    for file in files {
        if file.file_type == "file" {
            if let Some(download_url) = file.download_url {
                let response = client
                    .get(download_url)
                    .header("User-Agent", "atlas-cli")
                    .send()?;
                let mut dest = fs::File::create(outdir.join(&file.name))?;
                let bytes = response.bytes()?;
                dest.write_all(&bytes)?;
                downloaded_files += 1;
            }
        } else if file.file_type == "dir" {
            if exclude_subpaths
                .iter()
                .any(|&subpath| file.path.contains(subpath))
            {
                continue;
            }
            let subdir = outdir.join(&file.name);
            downloaded_files +=
                fetch_folder(owner, repo, &file.path, &subdir, branch, exclude_subpaths)?;
        }
    }

    Ok(downloaded_files)
}

fn fetch_releases(owner: &str, repo: &str) -> Result<Vec<Release>, Box<dyn std::error::Error>> {
    let url = format!("https://api.github.com/repos/{owner}/{repo}/releases");
    let client = Client::new();
    let releases: Vec<Release> = client
        .get(&url)
        .header("User-Agent", "atlas-cli")
        .send()?
        .json()?;
    Ok(releases)
}

fn pick_release(
    requested_version: &str,
    releases: &[Release],
) -> Result<Release, Box<dyn std::error::Error>> {
    if releases.is_empty() {
        return Err("No releases found".into());
    }

    if requested_version != "latest" {
        if let Some(found) = releases
            .iter()
            .find(|release| release.tag_name == requested_version)
        {
            return Ok(found.clone());
        }
        return Err(format!("Release '{requested_version}' not found").into());
    }

    let theme = ColorfulTheme::default();
    let mut sorted = releases.to_vec();
    sorted.sort_by(|a, b| b.tag_name.cmp(&a.tag_name));

    let mut items = Vec::with_capacity(sorted.len());
    for (idx, release) in sorted.iter().enumerate() {
        if idx == 0 {
            items.push(format!("{} (latest)", release.tag_name));
        } else {
            items.push(release.tag_name.clone());
        }
    }

    let selection = Select::with_theme(&theme)
        .with_prompt("Choose Atlas release")
        .items(&items)
        .default(0)
        .interact()?;

    Ok(sorted[selection].clone())
}

fn find_macos_assets(release: &Release) -> Result<AtlasBinarySet, Box<dyn std::error::Error>> {
    let mut metal: Option<ReleaseAsset> = None;
    let mut opengl: Option<ReleaseAsset> = None;
    let mut vulkan: Option<ReleaseAsset> = None;

    for asset in &release.assets {
        if asset.name == "macOS-atlas-metal.a" {
            metal = Some(asset.clone());
        } else if asset.name == "macOS-atlas-opengl.a" {
            opengl = Some(asset.clone());
        } else if asset.name == "macOS-atlas-vulkan.a" {
            vulkan = Some(asset.clone());
        }
    }

    if metal.is_none() && opengl.is_none() && vulkan.is_none() {
        return Err(format!(
            "Release '{}' does not contain any macOS backend archive or that release is too old. Please choose a different release or specify a newer version.",
            release.tag_name
        )
        .into());
    }

    Ok(AtlasBinarySet {
        metal,
        opengl,
        vulkan,
    })
}

fn available_backends(assets: &AtlasBinarySet) -> Vec<String> {
    let mut values = Vec::new();
    if assets.metal.is_some() {
        values.push(String::from("METAL"));
    }
    if assets.opengl.is_some() {
        values.push(String::from("OPENGL"));
    }
    if assets.vulkan.is_some() {
        values.push(String::from("VULKAN"));
    }
    values
}

fn download_asset(asset: &ReleaseAsset, out_path: &Path) -> Result<(), Box<dyn std::error::Error>> {
    let client = Client::new();
    let response = client
        .get(&asset.download_url)
        .header("User-Agent", "atlas-cli")
        .send()?;
    let mut dest = fs::File::create(out_path)?;
    let bytes = response.bytes()?;
    dest.write_all(&bytes)?;
    Ok(())
}

const TEMPLATE_MAIN: &str = r#"#include <SDL3/SDL_main.h>
#include <atlas/window.h>
#include <atlas/scene.h>
#include <atlas/object.h>

class MainScene : public Scene {
public:
    CoreObject cube;
    Camera camera;

    void initialize(Window& window) override {
        camera = Camera();
        window.setCamera(&camera);

        cube = createBox({1.0, 1.0, 1.0});
        window.addObject(&cube);

        this->setAmbientIntensity(0.2f);
    }
};

int main() {
    Window window({"Atlas App", 800, 600, false});
    MainScene scene;
    window.setScene(&scene);
    window.run();
    return 0;
}
"#;

const TEMPLATE_CMAKE: &str = r###"cmake_minimum_required(VERSION 3.18)
project(##PROJECTNAME## LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(ATLAS_BACKEND "##BACKEND##" CACHE STRING "Atlas backend")
set_property(CACHE ATLAS_BACKEND PROPERTY STRINGS ##BACKEND_OPTIONS##)
string(TOUPPER "${ATLAS_BACKEND}" ATLAS_BACKEND)

if(ATLAS_BACKEND STREQUAL "METAL")
    set(ATLAS_BINARY "${CMAKE_SOURCE_DIR}/lib/macOS-atlas-metal.a")
elseif(ATLAS_BACKEND STREQUAL "OPENGL")
    set(ATLAS_BINARY "${CMAKE_SOURCE_DIR}/lib/macOS-atlas-opengl.a")
elseif(ATLAS_BACKEND STREQUAL "VULKAN")
    set(ATLAS_BINARY "${CMAKE_SOURCE_DIR}/lib/macOS-atlas-vulkan.a")
else()
    message(FATAL_ERROR "Unsupported ATLAS_BACKEND='${ATLAS_BACKEND}'")
endif()

if(NOT EXISTS "${ATLAS_BINARY}")
    message(FATAL_ERROR "Missing ${ATLAS_BINARY}")
endif()

find_package(SDL3 REQUIRED CONFIG)

file(GLOB_RECURSE SOURCE_FILES ##PROJECTNAME##/*.cpp)
add_executable(##PROJECTNAMELC## ${SOURCE_FILES})

target_include_directories(##PROJECTNAMELC## PRIVATE include lib/include)

add_library(atlasbundle STATIC IMPORTED GLOBAL)
set_target_properties(atlasbundle PROPERTIES IMPORTED_LOCATION "${ATLAS_BINARY}")

target_link_libraries(##PROJECTNAMELC## PRIVATE atlasbundle SDL3::SDL3)

if(APPLE)
    if(ATLAS_BACKEND STREQUAL "METAL")
        target_link_libraries(##PROJECTNAMELC## PRIVATE
            "-framework Metal"
            "-framework MetalKit"
            "-framework MetalFX"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework QuartzCore")
    elseif(ATLAS_BACKEND STREQUAL "OPENGL")
        target_link_libraries(##PROJECTNAMELC## PRIVATE
            "-framework OpenGL"
            "-framework Cocoa"
            "-framework IOKit")
    elseif(ATLAS_BACKEND STREQUAL "VULKAN")
        target_link_libraries(##PROJECTNAMELC## PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework QuartzCore")
    endif()
endif()
"###;

const TEMPLATE_CONFIG: &str = r#"[project]
name = "((PROJECTNAME))"
app_name = "((PROJECTNAME))"
backend = "((BACKEND))"
platform = "((PLATFORM))"
atlas_version = "((ATLAS_VERSION))"

[pack]
icon = "none"
supported_platforms = "all"
"#;

fn create_or_open_branch(repo_path: &Path) {
    let git_dir = repo_path.join(".git");
    if !git_dir.exists() {
        return;
    }

    let theme = ColorfulTheme::default();
    let should_branch = Confirm::with_theme(&theme)
        .with_prompt("Create or switch to a new git branch?")
        .default(false)
        .interact()
        .unwrap_or(false);

    if !should_branch {
        return;
    }

    let branch_name: String = Input::with_theme(&theme)
        .with_prompt("Branch name")
        .with_initial_text("feature/atlas-setup")
        .interact_text()
        .unwrap_or_else(|_| String::from("feature/atlas-setup"));

    let _ = std::process::Command::new("git")
        .current_dir(repo_path)
        .arg("checkout")
        .arg("-b")
        .arg(&branch_name)
        .status();
}

pub fn create(cmd: Commands) {
    if let Commands::Create {
        name,
        path,
        version,
        branch,
        backend,
        platform,
    } = cmd
    {
        let releases = match fetch_releases("maxvdec", "atlas") {
            Ok(r) => r,
            Err(e) => {
                eprintln!("{} {e}", "Failed to fetch releases:".red().bold());
                return;
            }
        };

        let release = match pick_release(&version, &releases) {
            Ok(r) => r,
            Err(e) => {
                eprintln!("{} {e}", "Failed to choose release:".red().bold());
                return;
            }
        };

        let assets = match find_macos_assets(&release) {
            Ok(a) => a,
            Err(e) => {
                eprintln!("{} {e}", "Invalid release assets:".red().bold());
                return;
            }
        };

        let theme = ColorfulTheme::default();
        let selected_platform = match platform.and_then(|p| normalize_platform(&p)) {
            Some(p) => p,
            None => {
                let options = ["macOS", "Windows", "Linux"];
                let idx = Select::with_theme(&theme)
                    .with_prompt("Target platform")
                    .items(&options)
                    .default(0)
                    .interact()
                    .unwrap_or(0);
                options[idx].to_lowercase()
            }
        };

        let available = available_backends(&assets);
        let selected_backend = match backend.and_then(|b| normalize_backend(&b)) {
            Some(b) => {
                if !available.iter().any(|candidate| candidate == &b) {
                    eprintln!(
                        "{} {}",
                        "Requested backend is not available in selected release:"
                            .red()
                            .bold(),
                        b
                    );
                    eprintln!(
                        "{} {}",
                        "Available backends:".yellow(),
                        available.join(", ")
                    );
                    return;
                }
                b
            }
            None => {
                if available.len() == 1 {
                    available[0].clone()
                } else {
                    let default_backend = if selected_platform == "macos" {
                        String::from("METAL")
                    } else {
                        String::from("VULKAN")
                    };
                    let default_idx = available
                        .iter()
                        .position(|b| b == &default_backend)
                        .unwrap_or(0);
                    let idx = Select::with_theme(&theme)
                        .with_prompt("Rendering backend")
                        .items(&available)
                        .default(default_idx)
                        .interact()
                        .unwrap_or(default_idx);
                    available[idx].clone()
                }
            }
        };

        let project_path = path.unwrap_or_else(|| name.clone());
        if Path::new(&project_path).exists() {
            eprintln!(
                "{} {}",
                "Target path already exists:".red().bold(),
                project_path
            );
            return;
        }

        if fs::create_dir_all(&project_path).is_err() {
            eprintln!("{}", "Could not create project directory".red().bold());
            return;
        }

        let project_root = PathBuf::from(&project_path);
        if fs::create_dir(project_root.join(&name)).is_err() {
            eprintln!("{}", "Could not create source directory".red().bold());
            return;
        }
        if fs::create_dir(project_root.join("assets")).is_err() {
            eprintln!("{}", "Could not create assets directory".red().bold());
            return;
        }
        if fs::create_dir_all(project_root.join("lib/include")).is_err() {
            eprintln!("{}", "Could not create include directory".red().bold());
            return;
        }
        if fs::create_dir(project_root.join("include")).is_err() {
            eprintln!(
                "{}",
                "Could not create local include directory".red().bold()
            );
            return;
        }

        let main_path = project_root.join(&name).join("main.cpp");
        if fs::File::create(&main_path)
            .and_then(|mut f| f.write_all(TEMPLATE_MAIN.as_bytes()))
            .is_err()
        {
            eprintln!("{}", "Could not write main.cpp".red().bold());
            return;
        }

        let spinner = ProgressBar::new_spinner();
        if let Ok(style) = ProgressStyle::with_template("{spinner} {msg}") {
            spinner.set_style(style);
        }
        spinner.enable_steady_tick(std::time::Duration::from_millis(80));

        let pull_branch = branch.unwrap_or_else(|| String::from("stable"));
        spinner.set_message("Downloading Atlas headers");
        let outdir = project_root.join("lib/include");
        if fetch_folder("maxvdec", "atlas", "include", &outdir, &pull_branch, &[]).is_err() {
            spinner.finish_and_clear();
            eprintln!("{}", "Failed to download include files".red().bold());
            return;
        }
        if fetch_folder(
            "maxvdec",
            "atlas",
            "extern",
            &outdir,
            &pull_branch,
            &["freetype"],
        )
        .is_err()
        {
            spinner.finish_and_clear();
            eprintln!("{}", "Failed to download extern headers".red().bold());
            return;
        }

        spinner.set_message("Downloading macOS backend binaries");
        let lib_dir = project_root.join("lib");
        if let Some(asset) = &assets.metal {
            if download_asset(asset, &lib_dir.join("macOS-atlas-metal.a")).is_err() {
                spinner.finish_and_clear();
                eprintln!(
                    "{}",
                    "Failed to download Metal backend library".red().bold()
                );
                return;
            }
        }
        if let Some(asset) = &assets.opengl {
            if download_asset(asset, &lib_dir.join("macOS-atlas-opengl.a")).is_err() {
                spinner.finish_and_clear();
                eprintln!(
                    "{}",
                    "Failed to download OpenGL backend library".red().bold()
                );
                return;
            }
        }
        if let Some(asset) = &assets.vulkan {
            if download_asset(asset, &lib_dir.join("macOS-atlas-vulkan.a")).is_err() {
                spinner.finish_and_clear();
                eprintln!(
                    "{}",
                    "Failed to download Vulkan backend library".red().bold()
                );
                return;
            }
        }
        spinner.finish_and_clear();

        let backend_options = available.join(" ");

        let cmake_content = TEMPLATE_CMAKE
            .replace("##PROJECTNAME##", &name)
            .replace("##PROJECTNAMELC##", &name.to_lowercase())
            .replace("##BACKEND##", &selected_backend)
            .replace("##BACKEND_OPTIONS##", &backend_options);
        if fs::File::create(project_root.join("CMakeLists.txt"))
            .and_then(|mut f| f.write_all(cmake_content.as_bytes()))
            .is_err()
        {
            eprintln!("{}", "Could not write CMakeLists.txt".red().bold());
            return;
        }

        let atlas_toml = TEMPLATE_CONFIG
            .replace("((PROJECTNAME))", &name)
            .replace("((BACKEND))", &selected_backend)
            .replace("((PLATFORM))", &selected_platform)
            .replace("((ATLAS_VERSION))", &release.tag_name);

        if fs::File::create(project_root.join("atlas.toml"))
            .and_then(|mut f| f.write_all(atlas_toml.as_bytes()))
            .is_err()
        {
            eprintln!("{}", "Could not write atlas.toml".red().bold());
            return;
        }

        create_or_open_branch(&project_root);

        println!(
            "{} {}",
            "Created Atlas project:".green().bold(),
            name.bold().green()
        );
        println!("{} {}", "Release:".cyan(), release.tag_name.bold());
        println!("{} {}", "Platform:".cyan(), selected_platform.bold());
        println!("{} {}", "Backend:".cyan(), selected_backend.bold());
        println!(
            "{}",
            "Next: atlas build, atlas run, atlas pack, atlas clangd".yellow()
        );
    }
}
