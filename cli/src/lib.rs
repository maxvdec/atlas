pub mod create;
pub mod pack;
use clap::Subcommand;

#[derive(Subcommand)]
pub enum Commands {
    Create {
        name: String,
        path: Option<String>,
        #[arg(long, short)]
        branch: Option<String>,
        #[arg(default_value_t = String::from("latest"))]
        version: String,
        #[arg(long)]
        backend: Option<String>,
        #[arg(long)]
        platform: Option<String>,
    },
    Build {
        #[arg(default_value_t = 0, long, short)]
        release: u8,
        #[arg(long)]
        backend: Option<String>,
    },
    Pack {
        #[arg(default_value_t = 0, long, short)]
        release: u8,
        #[arg(long)]
        backend: Option<String>,
    },
    Run {
        #[arg(default_value_t = 0, long, short)]
        release: u8,
        #[arg(long)]
        backend: Option<String>,
    },
    Clangd {
        #[arg(long)]
        backend: Option<String>,
    },
}

#[derive(serde::Deserialize)]
pub struct ProjectConfig {
    pub name: String,
    pub app_name: Option<String>,
    pub backend: Option<String>,
    pub platform: Option<String>,
    pub atlas_version: Option<String>,
}

#[derive(serde::Deserialize)]
pub struct PackConfig {
    pub icon: String,
    pub supported_platforms: String,
    pub version: Option<String>,
    pub identifier: Option<String>,
}

#[derive(serde::Deserialize)]
pub struct Config {
    pub project: ProjectConfig,
    pub pack: PackConfig,
}
