use crate::Commands;
use colored::Colorize;
use dialoguer::theme::ColorfulTheme;
use dialoguer::{Confirm, Input, Select};
use indicatif::{ProgressBar, ProgressStyle};
use reqwest::blocking::Client;
use std::fs;
use std::io::{Cursor, Write};
use std::path::{Path, PathBuf};

#[derive(Clone)]
struct ReleaseAsset {
    name: String,
    download_url: String,
}

#[derive(Clone)]
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

fn github_client() -> Result<Client, Box<dyn std::error::Error>> {
    Ok(Client::builder().user_agent("atlas-cli").build()?)
}

fn fetch_latest_release_tag(
    owner: &str,
    repo: &str,
    client: &Client,
) -> Result<String, Box<dyn std::error::Error>> {
    let url = format!("https://github.com/{owner}/{repo}/releases/latest");
    let response = client.get(&url).send()?.error_for_status()?;
    let release_url = response.url().path().trim_end_matches('/');

    release_url
        .rsplit('/')
        .next()
        .filter(|segment| !segment.is_empty() && *segment != "latest")
        .map(str::to_string)
        .ok_or_else(|| format!("Failed to resolve latest release from {release_url}").into())
}

fn release_asset_url(owner: &str, repo: &str, tag: &str, asset_name: &str) -> String {
    format!("https://github.com/{owner}/{repo}/releases/download/{tag}/{asset_name}")
}

fn release_asset_exists(
    client: &Client,
    owner: &str,
    repo: &str,
    tag: &str,
    asset_name: &str,
) -> bool {
    client
        .head(release_asset_url(owner, repo, tag, asset_name))
        .send()
        .map(|response| response.status().is_success())
        .unwrap_or(false)
}

fn resolve_release(
    owner: &str,
    repo: &str,
    requested_version: &str,
) -> Result<Release, Box<dyn std::error::Error>> {
    let client = github_client()?;
    let tag_name = if requested_version == "latest" {
        fetch_latest_release_tag(owner, repo, &client)?
    } else {
        requested_version.to_string()
    };

    let mut assets = Vec::new();
    for asset_name in [
        "macOS-atlas-metal.a",
        "macOS-atlas-opengl.a",
        "macOS-atlas-vulkan.a",
    ] {
        if release_asset_exists(&client, owner, repo, &tag_name, asset_name) {
            assets.push(ReleaseAsset {
                name: asset_name.to_string(),
                download_url: release_asset_url(owner, repo, &tag_name, asset_name),
            });
        }
    }

    Ok(Release { tag_name, assets })
}

fn download_repository_archive(
    owner: &str,
    repo: &str,
    git_ref: &str,
    prefer_tag: bool,
) -> Result<Vec<u8>, Box<dyn std::error::Error>> {
    let client = github_client()?;
    let ref_kinds = if prefer_tag {
        ["tags", "heads"]
    } else {
        ["heads", "tags"]
    };
    let mut last_error: Option<Box<dyn std::error::Error>> = None;

    for ref_kind in ref_kinds {
        let url =
            format!("https://github.com/{owner}/{repo}/archive/refs/{ref_kind}/{git_ref}.zip");
        match client.get(&url).send() {
            Ok(response) => match response.error_for_status() {
                Ok(ok_response) => return Ok(ok_response.bytes()?.to_vec()),
                Err(e) => last_error = Some(Box::new(e)),
            },
            Err(e) => last_error = Some(Box::new(e)),
        }
    }

    Err(last_error
        .unwrap_or_else(|| format!("Failed to download source archive for ref '{git_ref}'").into()))
}

fn extract_archive_subpaths(
    archive_bytes: &[u8],
    outdir: &Path,
    mappings: &[(&str, &[&str])],
) -> Result<usize, Box<dyn std::error::Error>> {
    fs::create_dir_all(outdir)?;

    let cursor = Cursor::new(archive_bytes);
    let mut archive = zip::ZipArchive::new(cursor)?;
    let mut extracted_files = 0;

    for idx in 0..archive.len() {
        let mut entry = archive.by_index(idx)?;
        if !entry.is_file() {
            continue;
        }

        let Some(path) = entry.enclosed_name().map(|path| path.to_path_buf()) else {
            continue;
        };

        let relative: PathBuf = path.iter().skip(1).collect();
        if relative.as_os_str().is_empty() {
            continue;
        }

        for (prefix, excluded_prefixes) in mappings {
            let Ok(stripped) = relative.strip_prefix(prefix) else {
                continue;
            };

            if excluded_prefixes
                .iter()
                .any(|excluded| stripped.starts_with(excluded))
            {
                break;
            }

            let destination = outdir.join(stripped);
            if let Some(parent) = destination.parent() {
                fs::create_dir_all(parent)?;
            }

            let mut dest = fs::File::create(destination)?;
            std::io::copy(&mut entry, &mut dest)?;
            extracted_files += 1;
            break;
        }
    }

    Ok(extracted_files)
}

fn download_headers(
    owner: &str,
    repo: &str,
    git_ref: &str,
    outdir: &Path,
    prefer_tag: bool,
) -> Result<usize, Box<dyn std::error::Error>> {
    let archive = download_repository_archive(owner, repo, git_ref, prefer_tag)?;
    extract_archive_subpaths(
        &archive,
        outdir,
        &[("include", &[]), ("extern", &["freetype"])],
    )
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
    let client = github_client()?;
    let response = client.get(&asset.download_url).send()?.error_for_status()?;
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

include(FetchContent)
cmake_policy(SET CMP0135 NEW)
set(JOLT_VERSION "5.5.0")

FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG        v${JOLT_VERSION}
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  Build
)
set(JPH_DEBUG_RENDERER  ON  CACHE BOOL "" FORCE)
set(JPH_PROFILE_ENABLED ON  CACHE BOOL "" FORCE)
set(JPH_OBJECT_STREAM   ON  CACHE BOOL "" FORCE)
set(JPH_ENABLE_IPO      OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(JoltPhysics)
if(TARGET Jolt)
    set_target_properties(Jolt PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
endif()
message(STATUS "Downloaded Jolt Physics, version ${JOLT_VERSION}")

find_package(SDL3 REQUIRED CONFIG)

if(APPLE)
    set(OPENAL_LIB /opt/homebrew/opt/openal-soft/lib/libopenal.dylib)
    set(OPENAL_INCLUDE /opt/homebrew/opt/openal-soft/include)
else()
    find_package(OpenAL REQUIRED)
    set(OPENAL_LIB ${OPENAL_LIBRARY})
    set(OPENAL_INCLUDE ${OPENAL_INCLUDE_DIR})
endif()

file(GLOB_RECURSE SOURCE_FILES ##PROJECTNAME##/*.cpp)
add_executable(##PROJECTNAMELC## ${SOURCE_FILES})

if(ATLAS_BACKEND STREQUAL "METAL")
    target_compile_definitions(##PROJECTNAMELC## PRIVATE METAL GLM_FORCE_DEPTH_ZERO_TO_ONE)
elseif(ATLAS_BACKEND STREQUAL "OPENGL")
    target_compile_definitions(##PROJECTNAMELC## PRIVATE OPENGL)
elseif(ATLAS_BACKEND STREQUAL "VULKAN")
    target_compile_definitions(##PROJECTNAMELC## PRIVATE VULKAN GLM_FORCE_DEPTH_ZERO_TO_ONE)
endif()

target_include_directories(##PROJECTNAMELC## PRIVATE
    include
    lib/include
    ${joltphysics_SOURCE_DIR}
    ${OPENAL_INCLUDE}
)

add_library(atlasbundle STATIC IMPORTED GLOBAL)
set_target_properties(atlasbundle PROPERTIES IMPORTED_LOCATION "${ATLAS_BINARY}")

target_link_libraries(##PROJECTNAMELC## PRIVATE atlasbundle SDL3::SDL3 ${OPENAL_LIB})

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
        let release = match resolve_release("maxvdec", "atlas", &version) {
            Ok(r) => r,
            Err(e) => {
                eprintln!("{} {e}", "Failed to resolve release:".red().bold());
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

        if let Err(e) = fs::create_dir_all(&project_path) {
            eprintln!("{} {e}", "Could not create project directory:".red().bold());
            return;
        }

        let project_root = PathBuf::from(&project_path);
        if let Err(e) = fs::create_dir(project_root.join(&name)) {
            eprintln!("{} {e}", "Could not create source directory:".red().bold());
            return;
        }
        if let Err(e) = fs::create_dir(project_root.join("assets")) {
            eprintln!("{} {e}", "Could not create assets directory:".red().bold());
            return;
        }
        if let Err(e) = fs::create_dir_all(project_root.join("lib/include")) {
            eprintln!("{} {e}", "Could not create include directory:".red().bold());
            return;
        }
        if let Err(e) = fs::create_dir(project_root.join("include")) {
            eprintln!(
                "{} {e}",
                "Could not create local include directory:".red().bold()
            );
            return;
        }

        let main_path = project_root.join(&name).join("main.cpp");
        if let Err(e) =
            fs::File::create(&main_path).and_then(|mut f| f.write_all(TEMPLATE_MAIN.as_bytes()))
        {
            eprintln!("{} {e}", "Could not write main.cpp:".red().bold());
            return;
        }

        let spinner = ProgressBar::new_spinner();
        if let Ok(style) = ProgressStyle::with_template("{spinner} {msg}") {
            spinner.set_style(style);
        }
        spinner.enable_steady_tick(std::time::Duration::from_millis(80));

        let prefer_tag = branch.is_none();
        let pull_branch = branch.unwrap_or_else(|| release.tag_name.clone());
        spinner.set_message("Downloading Atlas headers");
        let outdir = project_root.join("lib/include");
        if let Err(e) = download_headers("maxvdec", "atlas", &pull_branch, &outdir, prefer_tag) {
            spinner.finish_and_clear();
            eprintln!("{} {e}", "Failed to download include files:".red().bold());
            return;
        }

        spinner.set_message("Downloading macOS backend binaries");
        let lib_dir = project_root.join("lib");
        if let Some(asset) = &assets.metal {
            if let Err(e) = download_asset(asset, &lib_dir.join("macOS-atlas-metal.a")) {
                spinner.finish_and_clear();
                eprintln!(
                    "{} {e}",
                    "Failed to download Metal backend library:".red().bold()
                );
                return;
            }
        }
        if let Some(asset) = &assets.opengl {
            if let Err(e) = download_asset(asset, &lib_dir.join("macOS-atlas-opengl.a")) {
                spinner.finish_and_clear();
                eprintln!(
                    "{} {e}",
                    "Failed to download OpenGL backend library:".red().bold()
                );
                return;
            }
        }
        if let Some(asset) = &assets.vulkan {
            if let Err(e) = download_asset(asset, &lib_dir.join("macOS-atlas-vulkan.a")) {
                spinner.finish_and_clear();
                eprintln!(
                    "{} {e}",
                    "Failed to download Vulkan backend library:".red().bold()
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
        if let Err(e) = fs::File::create(project_root.join("CMakeLists.txt"))
            .and_then(|mut f| f.write_all(cmake_content.as_bytes()))
        {
            eprintln!("{} {e}", "Could not write CMakeLists.txt:".red().bold());
            return;
        }

        let atlas_toml = TEMPLATE_CONFIG
            .replace("((PROJECTNAME))", &name)
            .replace("((BACKEND))", &selected_backend)
            .replace("((PLATFORM))", &selected_platform)
            .replace("((ATLAS_VERSION))", &release.tag_name);

        if let Err(e) = fs::File::create(project_root.join("atlas.toml"))
            .and_then(|mut f| f.write_all(atlas_toml.as_bytes()))
        {
            eprintln!("{} {e}", "Could not write atlas.toml:".red().bold());
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
