use crate::{backend::Backend, error::throw_error};

pub struct OpenGLBackend {
    include_paths: Vec<String>,
}

impl Backend for OpenGLBackend {
    fn run_translatable(&mut self, expressions: &crate::parser::TranslatableExpression) -> String {
        let mut output = String::new();
        for expr in &expressions.expressions {
            output.push_str(&format!("{} ", expr));
            if expr.ends_with(";") {
                output.push_str("\n");
            }
        }
        output
    }

    fn run_builtin(&mut self, builtin: &crate::parser::Builtin) -> (String, crate::backend::Stage) {
        // Change stage based on builtin type
        match builtin.name.as_str() {
            "vertex" => {
                return (String::new(), crate::backend::Stage::Vertex);
            }
            "fragment" => {
                return (String::new(), crate::backend::Stage::Fragment);
            }
            "tessellation" => {
                if builtin.args.len() >= 1 {
                    let shader_type = &builtin.args[0];
                    if shader_type.content == "control" {
                        return (String::new(), crate::backend::Stage::TessellationControl);
                    } else if shader_type.content == "evaluation" {
                        return (String::new(), crate::backend::Stage::TessellationEvaluation);
                    } else {
                        throw_error(crate::error::Error::InternalError(
                            "Tessellation builtin requires a shader type argument (control/evaluation)"
                                .to_string(),
                        ));
                        std::process::exit(1);
                    }
                } else {
                    throw_error(crate::error::Error::InternalError(
                        "Tessellation builtin requires a shader type argument (control/evaluation)"
                            .to_string(),
                    ));
                    std::process::exit(1);
                }
            }
            "geometry" => {
                return (String::new(), crate::backend::Stage::Geometry);
            }
            "compute" => {
                return (String::new(), crate::backend::Stage::Compute);
            }
            "mesh" => {
                return (String::new(), crate::backend::Stage::Mesh);
            }
            "task" => {
                return (String::new(), crate::backend::Stage::Task);
            }
            "raytracing" => {
                if !self.include_paths.contains(&"hana::raytracing".to_string()) {
                    throw_error(crate::error::Error::InternalError(
                        "Raytracing extension not included.".to_string(),
                    ));
                    std::process::exit(1);
                } else {
                    let shader_type = &builtin.args[0];
                    match shader_type.content.as_str() {
                        "generation" => {
                            return (String::new(), crate::backend::Stage::RaytracingGeneration);
                        }
                        "closest" => {
                            return (String::new(), crate::backend::Stage::RaytracingClosest);
                        }
                        "any" => {
                            return (String::new(), crate::backend::Stage::RaytracingAny);
                        }
                        "miss" => {
                            return (String::new(), crate::backend::Stage::RaytracingMiss);
                        }
                        "intersection" => {
                            return (String::new(), crate::backend::Stage::RaytracingIntersection);
                        }
                        "callable" => {
                            return (String::new(), crate::backend::Stage::RaytracingCallable);
                        }
                        _ => {
                            throw_error(crate::error::Error::InternalError(
                                "Raytracing builtin requires a valid shader type argument (generation/closest/any/miss/intersection/callable)"
                                    .to_string(),
                            ));
                            std::process::exit(1);
                        }
                    }
                }
            }
            _ => {}
        }

        // Stage arguments
        match builtin.name.as_str() {
            "stage" => {}
        }
    }

    fn run_function(&mut self, func: &crate::parser::FunctionExpression) -> String {}

    fn run_use(&mut self, use_expr: &crate::parser::UseExpression) -> String {}
}
