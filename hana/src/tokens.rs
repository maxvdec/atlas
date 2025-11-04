use crate::error::{Error, throw_error};

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum TokenType {
    // Define token types
    Number,
    Identifier,
    Keyword,
    Builtin,
    String,
    // Operators
    Plus,
    Minus,
    Asterisk,
    Slash,
    // Delimiters
    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Comma,
    Semicolon,
    Colon,
    Dot,
    Bang,
    LeftBracket,
    RightBracket,
    Equal,
    LessThan,
    GreaterThan,
    And,
    Or,
    // End of file
    EOF,
}

fn is_keyword(s: &str) -> bool {
    matches!(
        s,
        "if" | "else" | "while" | "for" | "return" | "func" | "const" | "struct" | "use"
    )
}

#[derive(Debug, Clone)]
pub struct Token {
    pub typ: TokenType,
    pub content: String,
    pub start: usize,
    pub end: usize,
}

pub struct Tokenizer {
    source: String,
    current: usize,
}

impl Tokenizer {
    pub fn new(source: String) -> Self {
        Self { source, current: 0 }
    }

    pub fn advance(&mut self) -> Option<char> {
        if self.current < self.source.len() {
            let ch = self.source.as_bytes()[self.current] as char;
            self.current += 1;
            Some(ch)
        } else {
            None
        }
    }

    pub fn advance_until(&mut self, target: char) -> String {
        let mut result = String::new();
        while let Some(ch) = self.advance() {
            if ch == target {
                break;
            }
            result.push(ch);
        }
        result
    }

    pub fn tokenize(&mut self) -> Vec<Token> {
        let mut tokens = Vec::new();
        while self.current < self.source.len() {
            let char = self.advance();
            if char.is_none() {
                break;
            }
            let ch = char.unwrap();
            if ch.is_whitespace() {
                continue;
            }

            if ch == '/' {
                if let Some(next_ch) = self.source.chars().nth(self.current) {
                    if next_ch == '/' {
                        while let Some(c) = self.advance() {
                            if c == '\n' {
                                break;
                            }
                        }
                        continue;
                    } else if next_ch == '*' {
                        self.advance();
                        while let Some(c) = self.advance() {
                            if c == '*' {
                                if let Some(nc) = self.source.chars().nth(self.current) {
                                    if nc == '/' {
                                        self.advance();
                                        break;
                                    }
                                }
                            }
                        }
                        continue;
                    }
                }
            }

            match ch {
                '+' => {
                    tokens.push(Token {
                        typ: TokenType::Plus,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '-' => {
                    tokens.push(Token {
                        typ: TokenType::Minus,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '*' => {
                    tokens.push(Token {
                        typ: TokenType::Asterisk,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '/' => {
                    tokens.push(Token {
                        typ: TokenType::Slash,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '(' => {
                    tokens.push(Token {
                        typ: TokenType::LeftParen,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                ')' => {
                    tokens.push(Token {
                        typ: TokenType::RightParen,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '{' => {
                    tokens.push(Token {
                        typ: TokenType::LeftBrace,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '}' => {
                    tokens.push(Token {
                        typ: TokenType::RightBrace,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                ',' => {
                    tokens.push(Token {
                        typ: TokenType::Comma,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                ';' => {
                    tokens.push(Token {
                        typ: TokenType::Semicolon,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '"' => {
                    let string_content = self.advance_until('"');
                    let start = self.current - string_content.len() - 1;
                    tokens.push(Token {
                        typ: TokenType::String,
                        content: string_content,
                        start,
                        end: self.current - 1,
                    });
                    continue;
                }
                '<' => {
                    tokens.push(Token {
                        typ: TokenType::LessThan,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '>' => {
                    tokens.push(Token {
                        typ: TokenType::GreaterThan,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '=' => {
                    tokens.push(Token {
                        typ: TokenType::Equal,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '!' => {
                    tokens.push(Token {
                        typ: TokenType::Bang,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '&' => {
                    if let Some(next_ch) = self.source.chars().nth(self.current) {
                        if next_ch == '&' {
                            self.advance();
                            tokens.push(Token {
                                typ: TokenType::And,
                                content: "&&".to_string(),
                                start: self.current - 2,
                                end: self.current - 1,
                            });
                            continue;
                        } else {
                            throw_error(Error::TokenizationError(
                                self.current - 1,
                                self.source.clone(),
                                "Unexpected character '&'".to_string(),
                            ));
                        }
                    }
                }
                '|' => {
                    if let Some(next_ch) = self.source.chars().nth(self.current) {
                        if next_ch == '|' {
                            self.advance();
                            tokens.push(Token {
                                typ: TokenType::Or,
                                content: "||".to_string(),
                                start: self.current - 2,
                                end: self.current - 1,
                            });
                            continue;
                        } else {
                            throw_error(Error::TokenizationError(
                                self.current - 1,
                                self.source.clone(),
                                "Unexpected character '|'".to_string(),
                            ));
                        }
                    }
                }
                ':' => {
                    tokens.push(Token {
                        typ: TokenType::Colon,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '.' => {
                    tokens.push(Token {
                        typ: TokenType::Dot,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                '[' => {
                    tokens.push(Token {
                        typ: TokenType::LeftBracket,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                ']' => {
                    tokens.push(Token {
                        typ: TokenType::RightBracket,
                        content: ch.to_string(),
                        start: self.current - 1,
                        end: self.current - 1,
                    });
                    continue;
                }
                _ => {}
            }

            if ch.is_numeric() {
                let mut number_content = ch.to_string();
                while let Some(next_ch) = self.advance() {
                    if next_ch.is_numeric() || next_ch == '.' {
                        number_content.push(next_ch);
                    } else {
                        self.current -= 1;
                        break;
                    }
                }
                tokens.push(Token {
                    typ: TokenType::Number,
                    content: number_content.clone(),
                    start: self.current - number_content.len(),
                    end: self.current - 1,
                });
                continue;
            }

            if ch.is_alphanumeric() || ch == '_' {
                let mut identifier_content = ch.to_string();
                while let Some(next_ch) = self.advance() {
                    if next_ch.is_alphanumeric() || next_ch == '_' {
                        identifier_content.push(next_ch);
                    } else {
                        self.current -= 1;
                        break;
                    }
                }
                let token_type = if is_keyword(&identifier_content) {
                    TokenType::Keyword
                } else {
                    TokenType::Identifier
                };
                tokens.push(Token {
                    typ: token_type,
                    content: identifier_content.clone(),
                    start: self.current - identifier_content.len(),
                    end: self.current - 1,
                });
                continue;
            }

            if ch == '@' {
                let mut builtin_content = String::new();
                while let Some(next_ch) = self.advance() {
                    if next_ch.is_alphanumeric() || next_ch == '_' {
                        builtin_content.push(next_ch);
                    } else {
                        self.current -= 1;
                        break;
                    }
                }
                tokens.push(Token {
                    typ: TokenType::Builtin,
                    content: builtin_content.clone(),
                    start: self.current - builtin_content.len() - 1,
                    end: self.current - 1,
                });
                continue;
            }

            throw_error(Error::TokenizationError(
                self.current - 1,
                self.source.clone(),
                "Unexpected character".to_string(),
            ));
        }
        return tokens;
    }
}
