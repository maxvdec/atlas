use crate::parser::{
    Builtin, Expression, FunctionExpression, Parser, TranslatableExpression, UseExpression,
};

pub trait Backend {
    fn run_translatable(&self, expressions: &TranslatableExpression) -> String;
    fn run_builtin(&self, builtin: &Builtin) -> String;
    fn run_use(&self, use_expr: &UseExpression) -> String;
    fn run_function(&self, func: &FunctionExpression) -> String;
}

pub fn compile(expressions: &[Box<dyn Expression>], backend: &dyn Backend) -> String {
    let mut output = String::new();

    for expr in expressions {
        if let Some(translatable) = expr.as_any().downcast_ref::<TranslatableExpression>() {
            output.push_str(&backend.run_translatable(translatable));
        } else if let Some(builtin) = expr.as_any().downcast_ref::<Builtin>() {
            output.push_str(&backend.run_builtin(builtin));
        } else if let Some(use_expr) = expr.as_any().downcast_ref::<UseExpression>() {
            output.push_str(&backend.run_use(use_expr));
        } else if let Some(func) = expr.as_any().downcast_ref::<FunctionExpression>() {
            output.push_str(&backend.run_function(func));
        }
    }

    output
}
