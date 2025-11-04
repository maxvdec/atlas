use std::process::exit;

use colored::*;

use crate::tokens::Token;

pub enum Error {
    InternalError(String),
    TokenizationError(usize, String, String),
    ParseError(Token, String, String),
}

pub fn throw_error(error: Error) {
    match error {
        Error::InternalError(message) => {
            println!(
                "{}",
                "The Hana compiler ran into an unexpected iternal error:"
                    .bold()
                    .red()
            );
            println!("{}", message.red());
            println!(
                "If this issue persists, please file a bug report at https://github.com/maxvdec/atlas/issues/new"
            );
        }
        Error::TokenizationError(pos, source, message) => {
            println!("{}", message.red().bold());
            let byte_of_pos = source
                .char_indices()
                .nth(pos)
                .map(|(b, _)| b)
                .unwrap_or(source.len());
            let line_start_byte = source[..byte_of_pos]
                .rfind('\n')
                .map(|i| i + 1)
                .unwrap_or(0);
            let line_end_byte = source[byte_of_pos..]
                .find('\n')
                .map(|i| byte_of_pos + i)
                .unwrap_or(source.len());

            let rel_pos =
                source[..byte_of_pos].chars().count() - source[..line_start_byte].chars().count();

            let source = &source[line_start_byte..line_end_byte];
            let pos = rel_pos;

            println!("│ {}", source);
            let mut prefix_width = 0usize;
            for (i, c) in source.chars().enumerate() {
                if i >= pos {
                    break;
                }
                prefix_width += if c == '\t' { 4 } else { 1 };
            }
            let underline = format!("{}^", " ".repeat(prefix_width));
            println!("│ {}", underline.red().bold());

            println!(
                "╰─{}",
                "Check that the character is valid and try again".yellow()
            );
            exit(1);
        }
        Error::ParseError(token, source, message) => {
            println!("{}", message.red().bold());
            let byte_of_pos = token.start;
            let line_start_byte = source[..byte_of_pos]
                .rfind('\n')
                .map(|i| i + 1)
                .unwrap_or(0);
            let line_end_byte = source[byte_of_pos..]
                .find('\n')
                .map(|i| byte_of_pos + i)
                .unwrap_or(source.len());

            let rel_pos =
                source[..byte_of_pos].chars().count() - source[..line_start_byte].chars().count();

            let source = &source[line_start_byte..line_end_byte];
            let pos = rel_pos;

            println!("│ {}", source);
            let mut prefix_width = 0usize;
            for (i, c) in source.chars().enumerate() {
                if i >= pos {
                    break;
                }
                prefix_width += if c == '\t' { 4 } else { 1 };
            }
            let underline = format!("{}^", " ".repeat(prefix_width));
            println!("│ {}", underline.red().bold());

            println!(
                "╰─{}",
                "Check that the character is valid and try again".yellow()
            );
            exit(1);
        }
    }
}
