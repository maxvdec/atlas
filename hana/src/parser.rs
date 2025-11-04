use std::fmt::Debug;

use crate::{
    error::throw_error,
    tokens::{Token, TokenType, Tokenizer},
};

pub trait Expression: Debug {}

#[derive(Debug)]
pub struct Builtin {
    name: String,
    args: Vec<Token>,
}

impl Expression for Builtin {}

#[derive(Debug)]
pub struct UseExpression {
    module: String,
}

impl Expression for UseExpression {}

#[derive(Debug)]
pub struct TranslatableExpression {
    expressions: Vec<String>,
}

impl Expression for TranslatableExpression {}

#[derive(Debug)]
pub struct TypedVariable {
    var_type: String,
    name: String,
}

#[derive(Debug)]
pub struct FunctionExpression {
    name: String,
    args: Vec<TypedVariable>,
    return_type: String,
    body: TranslatableExpression,
}

impl Expression for FunctionExpression {}

pub struct Parser {
    tokens: Vec<Token>,
    current: usize,
    source: String,
}

impl Parser {
    pub fn new(source: String) -> Self {
        let mut tokenizer = Tokenizer::new(source.clone());
        Parser {
            tokens: tokenizer.tokenize(),
            current: 0,
            source: source,
        }
    }

    pub fn advance(&mut self) -> Option<&Token> {
        if self.current < self.tokens.len() {
            let token = &self.tokens[self.current];
            self.current += 1;
            Some(token)
        } else {
            None
        }
    }

    pub fn next(&mut self) -> Option<&Token> {
        if self.current < self.tokens.len() {
            Some(&self.tokens[self.current])
        } else {
            None
        }
    }

    pub fn expect(&mut self, expected: TokenType, message: String) -> Option<&Token> {
        let source = self.source.clone();
        if let Some(token) = self.advance() {
            if token.typ == expected {
                Some(token)
            } else {
                throw_error(crate::error::Error::ParseError(
                    token.clone(),
                    source,
                    message,
                ));
                None
            }
        } else {
            None
        }
    }

    pub fn parse(&mut self) -> Vec<Box<dyn Expression>> {
        let mut expresssions: Vec<Box<dyn Expression>> = Vec::new();
        let mut pending_translatable: Vec<String> = Vec::new();

        while let Some(token) = self.advance() {
            match token.typ {
                TokenType::Keyword if token.content == "use" => {
                    if !pending_translatable.is_empty() {
                        expresssions.push(Box::new(TranslatableExpression {
                            expressions: pending_translatable.clone(),
                        }));
                        pending_translatable.clear();
                    }

                    let mut module_name = String::new();
                    while let Some(next_token) = self.advance() {
                        if next_token.typ == TokenType::Semicolon {
                            break;
                        }
                        module_name.push_str(&next_token.content);
                    }
                    expresssions.push(Box::new(UseExpression {
                        module: module_name,
                    }));
                }
                TokenType::Builtin => {
                    if !pending_translatable.is_empty() {
                        expresssions.push(Box::new(TranslatableExpression {
                            expressions: pending_translatable.clone(),
                        }));
                        pending_translatable.clear();
                    }

                    let builtin_name = token.content.clone();
                    let mut args: Vec<Token> = Vec::new();
                    let has_paren = if let Some(next_token) = self.next() {
                        next_token.typ == TokenType::LeftParen
                    } else {
                        false
                    };

                    if has_paren {
                        self.advance();
                        while let Some(next_token) = self.advance() {
                            if next_token.typ == TokenType::Semicolon {
                                break;
                            }
                            if next_token.typ == TokenType::Comma {
                                continue;
                            }
                            if next_token.typ == TokenType::RightParen {
                                break;
                            }
                            args.push(next_token.clone());
                        }
                    }

                    expresssions.push(Box::new(Builtin {
                        name: builtin_name,
                        args,
                    }));
                }

                _ => {
                    pending_translatable.push(token.content.clone());
                }
            }
        }

        if !pending_translatable.is_empty() {
            expresssions.push(Box::new(TranslatableExpression {
                expressions: pending_translatable,
            }));
        }

        expresssions
    }
}
