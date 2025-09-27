use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(name = "atlas")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    Create {
        name: String,
        path: Option<String>,
        #[arg(default_value_t = String::from("latest"))]
        version: String,
    },
    Pack {},
}

fn main() {
    let cli = Cli::parse();
}
