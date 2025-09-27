use atlas_cli::*;
use clap::Parser;

#[derive(Parser)]
#[command(name = "atlas")]
pub struct Cli {
    #[command(subcommand)]
    command: Commands,
}
fn main() {
    let cli = Cli::parse();

    match &cli.command {
        Commands::Create { .. } => {
            create::create(cli.command);
        }
        Commands::Pack { .. } => {
            pack::pack(cli.command);
        }
    }
}
