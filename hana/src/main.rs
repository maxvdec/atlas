use clap::{Parser, Subcommand};

#[derive(Parser)]
#[command(name = "hana")]
#[command(about = "A fast and reliable shading language for the future")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    Tokenize { input: String },
}

fn main() {
    let cli = Cli::parse();

    match &cli.command {
        Commands::Tokenize { input } => {
            let file_exists = std::path::Path::new(input).exists();
            if !file_exists {
                eprintln!("Error: File '{}' does not exist.", input);
                std::process::exit(1);
            }
            let source = std::fs::read_to_string(input).unwrap();
            let mut tokenizer = hana::tokens::Tokenizer::new(source.clone());
            println!("{:?}", tokenizer.tokenize());
        }
    }
}
