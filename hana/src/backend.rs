use crate::parser::{
    Builtin, Expression, FunctionExpression, TranslatableExpression, UseExpression,
};

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Stage {
    Vertex,
    Fragment,
    TessellationControl,
    TessellationEvaluation,
    Geometry,
    Compute,
    Mesh,
    Task,
    RaytracingGeneration,
    RaytracingClosest,
    RaytracingAny,
    RaytracingMiss,
    RaytracingIntersection,
    RaytracingCallable,
    Same,
    All,
}

pub trait Backend {
    fn run_translatable(&mut self, expressions: &TranslatableExpression) -> String;
    fn run_builtin(&mut self, builtin: &Builtin) -> (String, Stage);
    fn run_use(&mut self, use_expr: &UseExpression) -> String;
    fn run_function(&mut self, func: &FunctionExpression) -> String;
    fn finalize(&mut self, _outputs: &mut Vec<(String, Stage)>) {}
}

pub fn compile(
    expressions: &[Box<dyn Expression>],
    backend: &mut dyn Backend,
) -> Vec<(String, Stage)> {
    let mut output = Vec::new();
    output.push(("".to_string(), Stage::Vertex));
    output.push(("".to_string(), Stage::Fragment));
    output.push(("".to_string(), Stage::Compute));
    output.push(("".to_string(), Stage::Mesh));
    output.push(("".to_string(), Stage::Task));
    output.push(("".to_string(), Stage::TessellationControl));
    output.push(("".to_string(), Stage::TessellationEvaluation));
    output.push(("".to_string(), Stage::Geometry));
    output.push(("".to_string(), Stage::RaytracingGeneration));
    output.push(("".to_string(), Stage::RaytracingClosest));
    output.push(("".to_string(), Stage::RaytracingAny));
    output.push(("".to_string(), Stage::RaytracingMiss));
    output.push(("".to_string(), Stage::RaytracingIntersection));
    output.push(("".to_string(), Stage::RaytracingCallable));

    let mut current_stage: Stage = Stage::All;
    let mut result_string: String = String::new();

    for expr in expressions {
        if let Some(translatable) = expr.as_any().downcast_ref::<TranslatableExpression>() {
            result_string = backend.run_translatable(translatable);
        } else if let Some(builtin) = expr.as_any().downcast_ref::<Builtin>() {
            let (result, stage) = backend.run_builtin(builtin);
            if stage != Stage::Same {
                current_stage = stage;
            }
            result_string = result;
        } else if let Some(use_expr) = expr.as_any().downcast_ref::<UseExpression>() {
            result_string = backend.run_use(use_expr);
        } else if let Some(func) = expr.as_any().downcast_ref::<FunctionExpression>() {
            result_string = backend.run_function(func);
        }

        match current_stage {
            Stage::All => {
                for i in 0..output.len() {
                    output[i].0.push_str(&result_string);
                }
            }
            _ => {
                for (content, stage) in output.iter_mut() {
                    if stage == &current_stage {
                        content.push_str(&result_string);
                    }
                }
            }
        }
    }

    backend.finalize(&mut output);

    output
}
