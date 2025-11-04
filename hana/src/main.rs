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
    Tokenize {
        input: String,
    },
    Parse {
        input: String,
    },
    Compile {
        input: String,
        #[arg(long = "for")]
        api: String,
    },
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
        Commands::Parse { input } => {
            let file_exists = std::path::Path::new(input).exists();
            if !file_exists {
                eprintln!("Error: File '{}' does not exist.", input);
                std::process::exit(1);
            }
            let source = std::fs::read_to_string(input).unwrap();
            let mut parser = hana::parser::Parser::new(source.clone());
            println!("{:#?}", parser.parse());
        }
        Commands::Compile { input, api } => {
            let file_exists = std::path::Path::new(input).exists();
            if !file_exists {
                eprintln!("Error: File '{}' does not exist.", input);
                std::process::exit(1);
            }
            let source = std::fs::read_to_string(input).unwrap();
            let mut parser = hana::parser::Parser::new(source.clone());
            let expressions = parser.parse();

            let backend: Box<dyn hana::backend::Backend> = match api.as_str() {
                "opengl" => Box::new(hana::opengl::OpenGLBackend {}),
                _ => {
                    eprintln!(
                        "Error: Unsupported API '{}'. Supported APIs are 'opengl' and 'vulkan'.",
                        api
                    );
                    std::process::exit(1);
                }
            };

            let compiled_code = hana::backend::compile(&expressions, backend.as_ref());
            println!("{:#?}", compiled_code);
        }
    }
}
