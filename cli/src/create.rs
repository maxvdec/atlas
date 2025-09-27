use crate::Commands;
use colored::Colorize;
use reqwest::blocking::Client;
use serde::Deserialize;
use std::{fs, io::Write, path::Path};

#[derive(Deserialize)]
struct RemoteFile {
    name: String,
    path: String,
    #[serde(rename = "type")]
    file_type: String,
    download_url: Option<String>,
}

#[derive(Deserialize)]
struct ReleaseAsset {
    name: String,
    #[serde(rename = "browser_download_url")]
    download_url: String,
}

#[derive(Deserialize)]
struct Release {
    tag_name: String,
    assets: Vec<ReleaseAsset>,
}

fn fetch_folder(
    owner: &str,
    repo: &str,
    path: &str,
    outdir: &Path,
) -> Result<i32, Box<dyn std::error::Error>> {
    let url = format!("https://api.github.com/repos/{owner}/{repo}/contents/{path}?ref=stable");
    let client = Client::new();
    let files: Vec<RemoteFile> = client
        .get(&url)
        .header("User-Agent", "rust-client")
        .send()?
        .json()?;

    fs::create_dir_all(outdir)?;

    let mut downloaded_files = 0;

    for file in files {
        if file.file_type == "file" {
            if let Some(download_url) = file.download_url {
                let response = client
                    .get(download_url)
                    .header("User-Agent", "rust-client")
                    .send()?;
                let mut dest = fs::File::create(outdir.join(&file.name))?;
                let bytes = response.bytes()?;
                dest.write_all(&bytes)?;
                println!(
                    "{}",
                    format!("{}{}", "Downloaded file: ", file.name)
                        .italic()
                        .yellow()
                );
                downloaded_files += 1;
            }
        } else if file.file_type == "dir" {
            let subdir = outdir.join(&file.name);
            downloaded_files += fetch_folder(owner, repo, &file.path, &subdir)?;
        }
    }

    Ok(downloaded_files)
}

fn fetch_releases(owner: &str, repo: &str) -> Result<Vec<Release>, Box<dyn std::error::Error>> {
    let url = format!("https://api.github.com/repos/{owner}/{repo}/releases");
    let client = Client::new();
    let releases: Vec<Release> = client
        .get(&url)
        .header("User-Agent", "rust-client")
        .send()?
        .json()?;
    Ok(releases)
}

fn fetch_latest_release(owner: &str, repo: &str) -> Result<Release, Box<dyn std::error::Error>> {
    let releases = fetch_releases(owner, repo)?;
    releases
        .into_iter()
        .max_by(|a, b| a.tag_name.cmp(&b.tag_name))
        .ok_or_else(|| "No releases found".into())
}

fn fetch_all_assets(
    owner: &str,
    repo: &str,
    outdir: &Path,
) -> Result<(), Box<dyn std::error::Error>> {
    let release = fetch_latest_release(owner, repo)?;
    let client = Client::new();

    fs::create_dir_all(outdir)?;

    for asset in release.assets.iter() {
        let response = client
            .get(&asset.download_url)
            .header("User-Agent", "rust-client")
            .send()?;
        let mut dest = fs::File::create(outdir.join(&asset.name))?;
        let bytes = response.bytes()?;
        dest.write_all(&bytes)?;
        println!(
            "{}",
            format!("{}{}", "Downloaded asset: ", asset.name)
                .italic()
                .yellow()
        );
    }
    clear_previous_lines(release.assets.len() as usize);
    Ok(())
}

const TEMPLATE_MAIN: &str = r#"
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

        this->ambientLight.intensity = 0.5;
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

const TEMPLATE_CMAKE: &str = r#"
cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(##PROJECTNAME##)

# Find packages

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

file(GLOB_RECURSE SOURCE_FILES ##PROJECTNAME##/*.cpp)

add_executable(##PROJECTNAMELC## ${SOURCE_FILES})
target_include_directories(##PROJECTNAMELC## PRIVATE include lib/include)

link_directories(${CMAKE_SOURCE_DIR}/lib)

add_library(atlas STATIC IMPORTED)
set_target_properties(atlas PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/libatlas.a)

add_library(bezel STATIC IMPORTED)
set_target_properties(bezel PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/lib/libbezel.a)

target_link_libraries(##PROJECTNAMELC## PRIVATE atlas bezel OpenGL::GL glfw glm::glm)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

"#;

const TEMPLATE_CONFIG: &str = r#"
[project]
name = "((PROJECTNAME))"
app_name = "((PROJECTNAME))"

[pack]
icon = "none"
supported_platforms = "all"
"#;

fn clear_previous_lines(n: usize) {
    for _ in 0..n {
        print!("\x1B[1A");
        print!("\x1B[2K");
    }
    std::io::stdout().flush().unwrap();
}

pub fn create(cmd: Commands) {
    if let Commands::Create {
        name,
        path,
        version,
    } = cmd
    {
        if version != "latest" {
            eprintln!("Only 'latest' version is supported at the moment.");
            return;
        }
        let project_path = path.unwrap_or_else(|| name.clone());
        std::fs::create_dir_all(project_path.clone()).unwrap();

        // Create the main directory
        std::fs::create_dir(project_path.clone() + "/" + &name).unwrap();
        match std::fs::File::create(project_path.clone() + "/" + &name + "/main.cpp")
            .unwrap()
            .write_all(TEMPLATE_MAIN.as_bytes())
        {
            Ok(_) => println!("{}", "Created main.cpp".italic().cyan()),
            Err(e) => eprintln!("Error creating main.cpp: {e}"),
        }

        // Create the assets directory
        std::fs::create_dir(project_path.clone() + "/assets").unwrap();

        // Create the include directory
        std::fs::create_dir_all(project_path.clone() + "/lib/include").unwrap();
        std::fs::create_dir(project_path.clone() + "/include").unwrap();

        // Obtain the include files from the GitHub repository
        let outdir = Path::new(&project_path).join("lib/include");
        match fetch_folder("maxvdec", "atlas", "include", &outdir) {
            Ok(downloaded_files) => {
                clear_previous_lines(downloaded_files as usize);
                println!("{}", "Successfully fetched include files.".italic().cyan())
            }
            Err(e) => eprintln!("Error fetching include files: {e}"),
        }

        match fetch_folder("maxvdec", "atlas", "extern", &outdir) {
            Ok(downloaded_files) => {
                clear_previous_lines(downloaded_files as usize);
                println!(
                    "{}",
                    "Successfully fetched extern include files.".italic().cyan()
                )
            }
            Err(e) => eprintln!("Error fetching extern files: {e}"),
        }

        fetch_all_assets(
            "maxvdec",
            "atlas",
            Path::new(&project_path).join("lib").as_path(),
        )
        .unwrap();
        println!("{}", "Successfully fetched library files.".italic().cyan());

        let cmake_content = TEMPLATE_CMAKE
            .replace("##PROJECTNAME##", &name)
            .replace("##PROJECTNAMELC##", &name.to_lowercase());
        std::fs::File::create(project_path.clone() + "/CMakeLists.txt")
            .unwrap()
            .write_all(cmake_content.as_bytes())
            .unwrap();
        println!("{}", "Created CMakeLists.txt".italic().cyan());
        println!(
            "{}",
            format!("Project '{name}' created successfully!")
                .bold()
                .green()
        );
        std::fs::File::create(project_path.clone() + "/atlas.toml")
            .unwrap()
            .write_all(TEMPLATE_CONFIG.replace("((PROJECTNAME))", &name).as_bytes())
            .unwrap();
    }
}
