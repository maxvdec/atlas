pub mod create;
pub mod lsp;
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
    },
    Pack {
        #[arg(default_value_t = 0, long, short)]
        release: u8,
    },
    Run {
        #[arg(default_value_t = 0, long, short)]
        release: u8,
    },
    LSP {},
}

#[derive(serde::Deserialize)]
struct ProjectConfig {
    name: String,
    app_name: Option<String>,
}

#[derive(serde::Deserialize)]
struct PackConfig {
    icon: String,
    supported_platforms: String,
    version: Option<String>,
    identifier: Option<String>,
}

#[derive(serde::Deserialize)]
struct Config {
    project: ProjectConfig,
    pack: PackConfig,
}
