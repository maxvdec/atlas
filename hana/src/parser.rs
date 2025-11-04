use std::any::Any;
use std::fmt::Debug;

use crate::{
    error::throw_error,
    tokens::{Token, TokenType, Tokenizer},
};

pub trait Expression: Debug {
    fn as_any(&self) -> &dyn Any;
}

#[derive(Debug, Clone)]
pub struct Builtin {
    name: String,
    args: Vec<Token>,
}

impl Expression for Builtin {
    fn as_any(&self) -> &dyn Any {
        self
    }
}

#[derive(Debug, Clone)]
pub struct UseExpression {
    module: String,
}

impl Expression for UseExpression {
    fn as_any(&self) -> &dyn Any {
        self
    }
}

#[derive(Debug, Clone)]
pub struct TranslatableExpression {
    expressions: Vec<String>,
}

impl Expression for TranslatableExpression {
    fn as_any(&self) -> &dyn Any {
        self
    }
}

#[derive(Debug, Clone)]
pub struct TypedVariable {
    var_type: String,
    name: String,
}

#[derive(Debug, Clone)]
pub struct FunctionExpression {
    name: String,
    args: Vec<TypedVariable>,
    return_type: String,
    body: TranslatableExpression,
}

impl Expression for FunctionExpression {
    fn as_any(&self) -> &dyn Any {
        self
    }
}

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
                TokenType::Keyword if token.content == "func" => {
                    if !pending_translatable.is_empty() {
                        expresssions.push(Box::new(TranslatableExpression {
                            expressions: pending_translatable.clone(),
                        }));
                        pending_translatable.clear();
                    }

                    // Parse function name
                    let func_name_token = self.expect(
                        TokenType::Identifier,
                        "Expected function name after 'func' keyword.".to_string(),
                    );
                    let func_name = if let Some(t) = func_name_token {
                        t.content.clone()
                    } else {
                        continue;
                    };

                    // Parse arguments
                    self.expect(
                        TokenType::LeftParen,
                        "Expected '(' after function name.".to_string(),
                    );
                    let mut args: Vec<TypedVariable> = Vec::new();
                    while let Some(next_token) = self.advance() {
                        if next_token.typ == TokenType::RightParen {
                            break;
                        }
                        if next_token.typ == TokenType::Comma {
                            continue;
                        }
                        // Expect type
                        let var_type = next_token.content.clone();
                        // Expect name
                        let var_name_token = self.expect(
                            TokenType::Identifier,
                            "Expected variable name in function arguments.".to_string(),
                        );
                        let var_name = if let Some(t) = var_name_token {
                            t.content.clone()
                        } else {
                            continue;
                        };
                        args.push(TypedVariable {
                            var_type,
                            name: var_name,
                        });
                    }

                    // Expect return type
                    self.expect(
                        TokenType::Minus,
                        "Expected '->' after function arguments.".to_string(),
                    );
                    self.expect(
                        TokenType::GreaterThan,
                        "Expected '->' after function arguments.".to_string(),
                    );

                    let return_type = if let Some(t) = self.next() {
                        if t.typ == TokenType::Identifier {
                            let content = t.content.clone();
                            self.advance();
                            content
                        } else {
                            "void".to_string()
                        }
                    } else {
                        "void".to_string()
                    };

                    // Expect function body
                    self.expect(
                        TokenType::LeftBrace,
                        "Expected '{' to start function body.".to_string(),
                    );
                    let mut body_expressions: Vec<String> = Vec::new();
                    while let Some(next_token) = self.advance() {
                        if next_token.typ == TokenType::RightBrace {
                            break;
                        }
                        body_expressions.push(next_token.content.clone());
                    }

                    expresssions.push(Box::new(FunctionExpression {
                        name: func_name,
                        args,
                        return_type,
                        body: TranslatableExpression {
                            expressions: body_expressions,
                        },
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
