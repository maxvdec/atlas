#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum TokenType {}

#[derive(Debug)]
pub struct Token {
    typ: TokenType,
    content: String,
}

pub struct Tokenizer {
    source: String,
    current: usize,
}

impl Tokenizer {
    pub fn new(source: String) -> Self {
        Self { source, current: 0 }
    }

    pub fn tokenize(&mut self) -> Vec<Token> {
        let tokens = Vec::new();
        return tokens;
    }
}
