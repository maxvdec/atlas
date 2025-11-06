use std::collections::{HashMap, HashSet};

use crate::{
    backend::{Backend, Stage},
    error::{Error, throw_error},
    parser::{Builtin, FunctionExpression, TranslatableExpression, TypedVariable, UseExpression},
};

#[derive(Clone)]
struct StructField {
    typ: String,
    name: String,
    array_suffix: Option<String>,
}

#[derive(Clone)]
struct StructInfo {
    fields: Vec<StructField>,
    alignment: Option<String>,
}

#[derive(Clone)]
struct UniformAnnotation {
    set: Option<u32>,
    binding: Option<u32>,
}

#[derive(Clone, Copy)]
enum StageIO {
    In,
    Out,
}

#[derive(Clone)]
struct StageAnnotation {
    stage: Stage,
    io: StageIO,
}

#[derive(Clone)]
struct BufferAnnotation {
    _set: Option<u32>,
    binding: Option<u32>,
}

#[derive(Clone)]
struct OutputAnnotation {
    location: u32,
}

#[derive(Clone)]
struct OpenGLTransformAnnotation {
    block_name: Option<String>,
    max_elements: usize,
}

enum Annotation {
    Uniform(UniformAnnotation),
    OpenGLName(String),
    Stage(StageAnnotation),
    PushConstant,
    Align(String),
    Buffer(BufferAnnotation),
    Output(OutputAnnotation),
    OpenGLTransform(OpenGLTransformAnnotation),
    HanaVersion,
}

struct BodyContext {
    input_param: Option<String>,
    output_var: Option<String>,
}

pub struct OpenGLBackend {
    include_paths: Vec<String>,
    pending_annotations: Vec<Annotation>,
    version_directive: Option<String>,
    structs: HashMap<String, StructInfo>,
    global_struct_decls: Vec<String>,
    global_decls: Vec<String>,
    uniform_decls: Vec<String>,
    uniform_decl_cache: HashSet<String>,
    vertex_input_struct: Option<String>,
    vertex_input_decls: Vec<String>,
    vertex_input_map: HashMap<String, String>,
    vertex_output_struct: Option<String>,
    vertex_output_decls: Vec<String>,
    vertex_output_map: HashMap<String, String>,
    fragment_input_struct: Option<String>,
    fragment_input_decls: Vec<String>,
    fragment_input_map: HashMap<String, String>,
    fragment_output_decls: Vec<String>,
    helper_functions: Vec<String>,
    stage_functions: HashMap<Stage, FunctionExpression>,
    current_function_stage: Stage,
}

impl OpenGLBackend {
    pub fn new() -> Self {
        Self {
            include_paths: Vec::new(),
            pending_annotations: Vec::new(),
            version_directive: None,
            structs: HashMap::new(),
            global_struct_decls: Vec::new(),
            global_decls: Vec::new(),
            uniform_decls: Vec::new(),
            uniform_decl_cache: HashSet::new(),
            vertex_input_struct: None,
            vertex_input_decls: Vec::new(),
            vertex_input_map: HashMap::new(),
            vertex_output_struct: None,
            vertex_output_decls: Vec::new(),
            vertex_output_map: HashMap::new(),
            fragment_input_struct: None,
            fragment_input_decls: Vec::new(),
            fragment_input_map: HashMap::new(),
            fragment_output_decls: Vec::new(),
            helper_functions: Vec::new(),
            stage_functions: HashMap::new(),
            current_function_stage: Stage::All,
        }
    }

    fn write_indent(buf: &mut String, indent: usize) {
        for _ in 0..indent {
            buf.push_str("    ");
        }
    }

    fn take_uniform_annotation(&mut self) -> Option<UniformAnnotation> {
        if let Some(index) = self
            .pending_annotations
            .iter()
            .position(|ann| matches!(ann, Annotation::Uniform(_)))
        {
            if let Annotation::Uniform(info) = self.pending_annotations.remove(index) {
                return Some(info);
            }
        }
        None
    }

    fn take_opengl_name_annotation(&mut self) -> Option<String> {
        if let Some(index) = self
            .pending_annotations
            .iter()
            .position(|ann| matches!(ann, Annotation::OpenGLName(_)))
        {
            if let Annotation::OpenGLName(name) = self.pending_annotations.remove(index) {
                return Some(name);
            }
        }
        None
    }

    fn take_stage_annotations(&mut self) -> Vec<StageAnnotation> {
        let mut stages = Vec::new();
        let mut i = 0;
        while i < self.pending_annotations.len() {
            if let Annotation::Stage(stage) = &self.pending_annotations[i] {
                stages.push(stage.clone());
                self.pending_annotations.remove(i);
            } else {
                i += 1;
            }
        }
        stages
    }

    fn take_align_annotation(&mut self) -> Option<String> {
        if let Some(index) = self
            .pending_annotations
            .iter()
            .position(|ann| matches!(ann, Annotation::Align(_)))
        {
            if let Annotation::Align(value) = self.pending_annotations.remove(index) {
                return Some(value);
            }
        }
        None
    }

    fn take_buffer_annotation(&mut self) -> Option<BufferAnnotation> {
        if let Some(index) = self
            .pending_annotations
            .iter()
            .position(|ann| matches!(ann, Annotation::Buffer(_)))
        {
            if let Annotation::Buffer(info) = self.pending_annotations.remove(index) {
                return Some(info);
            }
        }
        None
    }

    fn take_output_annotation(&mut self) -> Option<OutputAnnotation> {
        if let Some(index) = self
            .pending_annotations
            .iter()
            .position(|ann| matches!(ann, Annotation::Output(_)))
        {
            if let Annotation::Output(info) = self.pending_annotations.remove(index) {
                return Some(info);
            }
        }
        None
    }

    fn take_transform_annotation(&mut self) -> Option<OpenGLTransformAnnotation> {
        if let Some(index) = self
            .pending_annotations
            .iter()
            .position(|ann| matches!(ann, Annotation::OpenGLTransform(_)))
        {
            if let Annotation::OpenGLTransform(info) = self.pending_annotations.remove(index) {
                return Some(info);
            }
        }
        None
    }

    fn take_push_annotation(&mut self) -> bool {
        if let Some(index) = self
            .pending_annotations
            .iter()
            .position(|ann| matches!(ann, Annotation::PushConstant))
        {
            self.pending_annotations.remove(index);
            return true;
        }
        false
    }

    fn take_hana_version_annotation(&mut self) -> bool {
        if let Some(index) = self
            .pending_annotations
            .iter()
            .position(|ann| matches!(ann, Annotation::HanaVersion))
        {
            self.pending_annotations.remove(index);
            return true;
        }
        false
    }

    fn map_type(&self, hana_type: &str) -> String {
        match hana_type {
            "Texture" | "Texture2D" => "sampler2D".to_string(),
            "TextureCube" => "samplerCube".to_string(),
            "Texture3D" => "sampler3D".to_string(),
            "Color" => "vec4".to_string(),
            other => other.to_string(),
        }
    }

    fn is_scalar_type(typ: &str) -> bool {
        matches!(typ, "int" | "uint" | "float" | "double" | "bool")
    }

    fn push_uniform_decl(&mut self, declaration: String) {
        if self.uniform_decl_cache.insert(declaration.clone()) {
            self.uniform_decls.push(declaration);
        }
    }

    fn format_struct_definition(name: &str, info: &StructInfo) -> String {
        let mut out = String::new();
        out.push_str(&format!("struct {} {{\n", name));
        for field in &info.fields {
            let mut line = String::new();
            line.push_str("    ");
            line.push_str(&field.typ);
            line.push(' ');
            line.push_str(&field.name);
            if let Some(suffix) = &field.array_suffix {
                line.push_str(suffix);
            }
            line.push_str(";\n");
            out.push_str(&line);
        }
        out.push_str("};");
        out
    }

    fn register_stage_struct(&mut self, name: &str, info: &StructInfo, stages: &[StageAnnotation]) {
        for stage_ann in stages {
            match (stage_ann.stage, &stage_ann.io) {
                (Stage::Vertex, StageIO::In) => {
                    if self.vertex_input_struct.is_some() {
                        continue;
                    }
                    self.vertex_input_struct = Some(name.to_string());
                    for (index, field) in info.fields.iter().enumerate() {
                        let glsl_type = self.map_type(&field.typ);
                        let var_name = format!("a_{}", field.name);
                        let decl = format!(
                            "layout(location = {}) in {} {};",
                            index, glsl_type, var_name
                        );
                        self.vertex_input_decls.push(decl);
                        self.vertex_input_map
                            .insert(field.name.clone(), var_name.clone());
                    }
                }
                (Stage::Vertex, StageIO::Out) => {
                    if self.vertex_output_struct.is_some() {
                        continue;
                    }
                    self.vertex_output_struct = Some(name.to_string());
                    for (index, field) in info.fields.iter().enumerate() {
                        let glsl_type = self.map_type(&field.typ);
                        let var_name = format!("v_{}", field.name);
                        let decl = format!(
                            "layout(location = {}) out {} {};",
                            index, glsl_type, var_name
                        );
                        self.vertex_output_decls.push(decl);
                        self.vertex_output_map
                            .insert(field.name.clone(), var_name.clone());
                        self.fragment_input_map
                            .insert(field.name.clone(), var_name.clone());
                    }
                }
                (Stage::Fragment, StageIO::In) => {
                    if self.fragment_input_struct.is_some() {
                        continue;
                    }
                    self.fragment_input_struct = Some(name.to_string());
                    for (index, field) in info.fields.iter().enumerate() {
                        let glsl_type = self.map_type(&field.typ);
                        let var_name = format!("v_{}", field.name);
                        let decl = format!(
                            "layout(location = {}) in {} {};",
                            index, glsl_type, var_name
                        );
                        self.fragment_input_decls.push(decl);
                        self.fragment_input_map
                            .insert(field.name.clone(), var_name.clone());
                    }
                }
                _ => {}
            }
        }
    }

    fn parse_struct(&mut self, tokens: &[String]) -> usize {
        if tokens.len() < 3 || tokens[0] != "struct" {
            return 0;
        }
        let name = tokens[1].clone();
        let mut fields = Vec::new();
        let mut index = 2;
        while index < tokens.len() && tokens[index] != "{" {
            index += 1;
        }
        if index >= tokens.len() {
            return tokens.len();
        }
        index += 1; // skip '{'
        while index < tokens.len() {
            if tokens[index] == "}" {
                index += 1;
                break;
            }
            if index + 1 >= tokens.len() {
                break;
            }
            let field_type = tokens[index].clone();
            index += 1;
            let field_name = tokens[index].clone();
            index += 1;
            let mut suffix_parts: Vec<String> = Vec::new();
            while index < tokens.len() && tokens[index] != ";" {
                if tokens[index] == "}" {
                    break;
                }
                suffix_parts.push(tokens[index].clone());
                index += 1;
            }
            if index < tokens.len() && tokens[index] == ";" {
                index += 1;
            }
            let array_suffix = if suffix_parts.is_empty() {
                None
            } else {
                Some(suffix_parts.join(""))
            };
            fields.push(StructField {
                typ: field_type,
                name: field_name,
                array_suffix,
            });
        }

        let alignment = self.take_align_annotation();
        let stages = self.take_stage_annotations();
        let info = StructInfo { fields, alignment };
        self.register_stage_struct(&name, &info, &stages);
        self.structs.insert(name.clone(), info.clone());
        self.global_struct_decls
            .push(Self::format_struct_definition(&name, &info));
        index
    }

    fn parse_global_declaration(&mut self, tokens: &[String]) -> usize {
        let Some(semicolon) = tokens.iter().position(|t| t == ";") else {
            return 0;
        };

        let mut idx = 0;
        let mut is_const = false;
        if tokens[idx] == "const" {
            is_const = true;
            idx += 1;
        }
        if idx >= semicolon {
            return semicolon + 1;
        }
        let type_token = tokens[idx].clone();
        idx += 1;
        if idx >= semicolon {
            return semicolon + 1;
        }
        let mut name_token = tokens[idx].clone();
        idx += 1;
        while idx < semicolon {
            name_token.push_str(&tokens[idx]);
            idx += 1;
        }

        let mut glsl_name = name_token.clone();
        if let Some(opengl_name) = self.take_opengl_name_annotation() {
            glsl_name = opengl_name;
        }

        if let Some(output) = self.take_output_annotation() {
            let glsl_type = self.map_type(&type_token);
            let decl = format!(
                "layout(location = {}) out {} {};",
                output.location, glsl_type, glsl_name
            );
            self.fragment_output_decls.push(decl);
            return semicolon + 1;
        }

        let transform = self.take_transform_annotation();
        if let Some(buffer) = self.take_buffer_annotation() {
            self.build_uniform_block(&type_token, &glsl_name, buffer, transform);
            return semicolon + 1;
        }

        let has_push = self.take_push_annotation();
        if let Some(_uniform) = self.take_uniform_annotation().or_else(|| {
            if has_push {
                Some(UniformAnnotation {
                    set: None,
                    binding: None,
                })
            } else {
                None
            }
        }) {
            let glsl_type = self.map_type(&type_token);
            let decl = format!("uniform {} {};", glsl_type, glsl_name);
            self.push_uniform_decl(decl);
            return semicolon + 1;
        }

        let mut line = String::new();
        for (i, token) in tokens.iter().enumerate().take(semicolon + 1) {
            if i > 0 {
                if token == ";" {
                    line.push_str(token);
                    break;
                }
                if !token.starts_with("(")
                    && !token.starts_with(")")
                    && !token.starts_with(",")
                    && !token.starts_with("[")
                    && !token.starts_with("]")
                {
                    line.push(' ');
                }
            }
            line.push_str(token);
        }
        if is_const {
            self.global_decls.push(line);
        } else {
            self.global_decls.push(line);
        }

        semicolon + 1
    }

    fn build_uniform_block(
        &mut self,
        struct_type: &str,
        instance_name: &str,
        buffer: BufferAnnotation,
        transform: Option<OpenGLTransformAnnotation>,
    ) {
        let Some(struct_info) = self.structs.get(struct_type) else {
            throw_error(Error::InternalError(format!(
                "Struct '{}' not found for buffer declaration.",
                struct_type
            )));
            return;
        };

        let mut layout_items = Vec::new();
        if let Some(alignment) = &struct_info.alignment {
            layout_items.push(alignment.clone());
        }
        if let Some(binding) = buffer.binding {
            layout_items.push(format!("binding = {}", binding));
        }
        let layout_prefix = if layout_items.is_empty() {
            String::new()
        } else {
            format!("layout({}) ", layout_items.join(", "))
        };

        let block_name = transform
            .as_ref()
            .and_then(|t| t.block_name.clone())
            .unwrap_or_else(|| format!("{}_Block", instance_name));
        let max_elements = transform.as_ref().map(|t| t.max_elements);

        let mut block = String::new();
        block.push_str(&format!("{}uniform {} {{\n", layout_prefix, block_name));
        for field in &struct_info.fields {
            let glsl_type = self.map_type(&field.typ);
            let mut field_line = String::new();
            field_line.push_str("    ");
            if let Some(max) = max_elements {
                if !Self::is_scalar_type(&field.typ) {
                    field_line.push_str(&format!("{} {}[{}];", glsl_type, field.name, max));
                    field_line.push('\n');
                    block.push_str(&field_line);
                    continue;
                }
            }
            field_line.push_str(&glsl_type);
            field_line.push(' ');
            field_line.push_str(&field.name);
            if let Some(suffix) = &field.array_suffix {
                field_line.push_str(suffix);
            }
            field_line.push_str(";\n");
            block.push_str(&field_line);
        }
        block.push_str(&format!("}} {};", instance_name));
        self.push_uniform_decl(block);
    }

    fn resolve_version(&self, tokens: &[String]) -> String {
        if tokens.is_empty() {
            return "#version 410 core".to_string();
        }
        match tokens[0].as_str() {
            "latest" => "#version 410 core".to_string(),
            other => format!("#version {} core", other),
        }
    }

    fn find_param(args: &[TypedVariable], target_type: &Option<String>) -> Option<String> {
        target_type.as_ref().and_then(|t| {
            args.iter()
                .find(|arg| arg.var_type == *t)
                .map(|arg| arg.name.clone())
        })
    }

    fn detect_struct_variable(tokens: &[String], struct_name: &Option<String>) -> Option<String> {
        let Some(name) = struct_name else {
            return None;
        };
        let mut index = 0;
        while index + 2 < tokens.len() {
            if tokens[index] == *name {
                let candidate = tokens[index + 1].clone();
                let terminator = tokens[index + 2].clone();
                if terminator == ";" || terminator == "=" {
                    return Some(candidate);
                }
            }
            index += 1;
        }
        None
    }

    fn map_field_access(
        &self,
        stage: Stage,
        ctx: &BodyContext,
        base: &str,
        field: &str,
    ) -> Option<String> {
        match stage {
            Stage::Vertex => {
                if ctx
                    .input_param
                    .as_ref()
                    .map(|param| param == base)
                    .unwrap_or(false)
                {
                    return self.vertex_input_map.get(field).cloned();
                }
                if ctx
                    .output_var
                    .as_ref()
                    .map(|param| param == base)
                    .unwrap_or(false)
                {
                    return self.vertex_output_map.get(field).cloned();
                }
            }
            Stage::Fragment => {
                if ctx
                    .input_param
                    .as_ref()
                    .map(|param| param == base)
                    .unwrap_or(false)
                {
                    return self.fragment_input_map.get(field).cloned();
                }
            }
            _ => {}
        }
        None
    }

    fn map_stage_variable(stage: Stage, symbol: &str) -> Option<&'static str> {
        match stage {
            Stage::Vertex => match symbol {
                "@position" => Some("gl_Position"),
                "@pointSize" => Some("gl_PointSize"),
                "@instanceId" => Some("gl_InstanceID"),
                "@vertexId" => Some("gl_VertexID"),
                "@drawId" => Some("gl_DrawID"),
                _ => None,
            },
            Stage::Fragment => match symbol {
                "@fragCoordinates" => Some("gl_FragCoord"),
                "@frontFacing" => Some("gl_FrontFacing"),
                "@pointCoordinates" => Some("gl_PointCoord"),
                "@sampleId" => Some("gl_SampleID"),
                "@samplePosition" => Some("gl_SamplePosition"),
                "@sampleMask" => Some("gl_SampleMask"),
                "@sampleMaskIn" => Some("gl_SampleMaskIn"),
                "@fragDepth" => Some("gl_FragDepth"),
                "@primitiveId" => Some("gl_PrimitiveID"),
                _ => None,
            },
            Stage::Compute => match symbol {
                "@localInvocationId" => Some("gl_LocalInvocationID"),
                "@globalInvocationId" => Some("gl_GlobalInvocationID"),
                "@workgroupId" => Some("gl_WorkGroupID"),
                "@numWorkgroups" => Some("gl_NumWorkGroups"),
                "@localInvocationIndex" => Some("gl_LocalInvocationIndex"),
                _ => None,
            },
            Stage::TessellationControl => match symbol {
                "@invocationId" => Some("gl_InvocationID"),
                "@in" => Some("gl_in"),
                "@out" => Some("gl_out"),
                "@tessLevelOuter" => Some("gl_TessLevelOuter"),
                "@tessLevelInner" => Some("gl_TessLevelInner"),
                "@primitiveId" => Some("gl_PrimitiveID"),
                _ => None,
            },
            Stage::TessellationEvaluation => match symbol {
                "@tessCoord" => Some("gl_TessCoord"),
                "@in" => Some("gl_in"),
                "@primitiveId" => Some("gl_PrimitiveID"),
                "@position" => Some("gl_Position"),
                _ => None,
            },
            Stage::Geometry => match symbol {
                "@in" => Some("gl_in"),
                "@emitVertex" => Some("EmitVertex"),
                "@endPrimitive" => Some("EndPrimitive"),
                "@primitiveIdIn" => Some("gl_PrimitiveIDIn"),
                "@primitiveId" => Some("gl_PrimitiveID"),
                "@layer" => Some("gl_Layer"),
                "@viewportIndex" => Some("gl_ViewportIndex"),
                _ => None,
            },
            Stage::Mesh => match symbol {
                "@meshVertices" => Some("gl_MeshVerticesNV"),
                "@meshPrimitives" => Some("gl_MeshPrimitivesNV"),
                "@taskCount" => Some("gl_TaskCountNV"),
                "@workgroupId" => Some("gl_WorkGroupID"),
                "@localInvocationId" => Some("gl_LocalInvocationID"),
                _ => None,
            },
            Stage::Task => match symbol {
                "@taskCount" => Some("gl_TaskCountNV"),
                "@workgroupId" => Some("gl_WorkGroupID"),
                "@localInvocationId" => Some("gl_LocalInvocationID"),
                _ => None,
            },
            Stage::RaytracingGeneration
            | Stage::RaytracingClosest
            | Stage::RaytracingAny
            | Stage::RaytracingMiss
            | Stage::RaytracingIntersection
            | Stage::RaytracingCallable => match symbol {
                "@rayOrigin" => Some("gl_WorldRayOriginNV"),
                "@rayDirection" => Some("gl_WorldRayDirectionNV"),
                "@hitT" => Some("gl_HitTNV"),
                "@launchId" => Some("gl_LaunchIDNV"),
                "@launchSize" => Some("gl_LaunchSizeNV"),
                "@primitiveId" => Some("gl_PrimitiveID"),
                "@instanceId" => Some("gl_InstanceID"),
                "@geometryId" => Some("gl_GeometryIndexEXT"),
                "@hitKind" => Some("gl_HitKindNV"),
                "@missIndex" => Some("gl_MissIndexNV"),
                _ => None,
            },
            _ => None,
        }
    }

    fn join_tokens(tokens: &[String]) -> String {
        let mut out = String::new();
        for token in tokens {
            match token.as_str() {
                "," | "." | "(" | ")" | "[" | "]" => {
                    out.push_str(token);
                }
                _ => {
                    if !out.is_empty() && !out.ends_with([' ', '\n', '(', '[', ',']) {
                        out.push(' ');
                    }
                    out.push_str(token);
                }
            }
        }
        out
    }

    fn try_transform_optional_expression(
        tokens: &[String],
        start: usize,
    ) -> Option<(String, usize)> {
        if tokens.get(start + 1).map(|t| t.as_str()) != Some(".") {
            return None;
        }
        let field_token = tokens.get(start + 2)?;
        if tokens.get(start + 3).map(|t| t.as_str()) != Some("[") {
            return None;
        }
        let mut index_end = start + 4;
        let mut index_tokens = Vec::new();
        while index_end < tokens.len() {
            if tokens[index_end] == "]" {
                break;
            }
            index_tokens.push(tokens[index_end].clone());
            index_end += 1;
        }
        if index_end >= tokens.len() || tokens[index_end] != "]" {
            return None;
        }
        let or_index = index_end + 1;
        if tokens.get(or_index).map(|t| t.as_str()) != Some("or") {
            return None;
        }
        let mut fallback_tokens = Vec::new();
        let mut cursor = or_index + 1;
        let mut paren_balance = 0i32;
        while cursor < tokens.len() {
            let token = &tokens[cursor];
            if token == ";" && paren_balance == 0 {
                break;
            }
            if token == "," && paren_balance == 0 {
                break;
            }
            if token == "(" {
                paren_balance += 1;
            } else if token == ")" {
                if paren_balance == 0 {
                    break;
                }
                paren_balance -= 1;
            }
            fallback_tokens.push(token.clone());
            cursor += 1;
        }

        if fallback_tokens.is_empty() {
            return None;
        }

        let index_expr = Self::join_tokens(&index_tokens);
        let fallback_expr = Self::join_tokens(&fallback_tokens);
        let value_expr = format!("{}.{}[{}]", tokens[start], field_token, index_expr);
        let guard = format!("{} < {}.lightCount", index_expr, tokens[start]);
        let wrapped = format!("({} ? {} : {})", guard, value_expr, fallback_expr);
        Some((wrapped, cursor))
    }

    fn format_function_args(&self, args: &[TypedVariable]) -> String {
        args.iter()
            .map(|arg| format!("{} {}", self.map_type(&arg.var_type), arg.name))
            .collect::<Vec<_>>()
            .join(", ")
    }

    fn format_function_body(&self, tokens: &[String], stage: Stage, ctx: &BodyContext) -> String {
        let mut out = String::new();
        let mut i = 0;
        let mut indent = 1usize;
        let mut line_start = true;

        while i < tokens.len() {
            if let Some((transformed, next_index)) =
                Self::try_transform_optional_expression(tokens, i)
            {
                if line_start {
                    Self::write_indent(&mut out, indent);
                    line_start = false;
                } else if !out.ends_with([' ', '\n', '(']) {
                    out.push(' ');
                }
                out.push_str(&transformed);
                i = next_index;
                continue;
            }

            if let Some(mapped) = Self::map_stage_variable(stage, tokens[i].as_str()) {
                if line_start {
                    Self::write_indent(&mut out, indent);
                    line_start = false;
                } else if !out.ends_with([' ', '\n', '(']) {
                    out.push(' ');
                }
                out.push_str(mapped);
                i += 1;
                continue;
            }

            if i + 3 < tokens.len()
                && tokens[i + 1] == "."
                && tokens[i + 2] == "sample"
                && tokens[i + 3] == "("
            {
                if line_start {
                    Self::write_indent(&mut out, indent);
                    line_start = false;
                } else if !out.ends_with([' ', '\n', '(']) {
                    out.push(' ');
                }
                out.push_str(&format!("texture({}, ", tokens[i]));
                i += 4;
                continue;
            }

            if i + 2 < tokens.len() && tokens[i + 1] == "." {
                if let Some(mapped) = self.map_field_access(stage, ctx, &tokens[i], &tokens[i + 2])
                {
                    if line_start {
                        Self::write_indent(&mut out, indent);
                        line_start = false;
                    } else if !out.ends_with([' ', '\n', '(']) {
                        out.push(' ');
                    }
                    out.push_str(&mapped);
                    i += 3;
                    continue;
                }
            }

            let token = tokens[i].as_str();
            match token {
                "{" => {
                    if line_start {
                        Self::write_indent(&mut out, indent);
                    }
                    out.push_str("{\n");
                    indent += 1;
                    line_start = true;
                    i += 1;
                }
                "}" => {
                    if indent > 0 {
                        indent -= 1;
                    }
                    if !line_start {
                        out.push('\n');
                    }
                    Self::write_indent(&mut out, indent);
                    out.push_str("}\n");
                    line_start = true;
                    i += 1;
                }
                ";" => {
                    out.push_str(";\n");
                    line_start = true;
                    i += 1;
                }
                "," => {
                    out.push_str(", ");
                    line_start = false;
                    i += 1;
                }
                "(" => {
                    out.push('(');
                    line_start = false;
                    i += 1;
                }
                ")" => {
                    out.push(')');
                    line_start = false;
                    i += 1;
                }
                "[" => {
                    out.push('[');
                    line_start = false;
                    i += 1;
                }
                "]" => {
                    out.push(']');
                    line_start = false;
                    i += 1;
                }
                "=" | "+" | "-" | "*" | "/" | "%" | "==" | "!=" | "<" | ">" | "<=" | ">=" => {
                    if line_start {
                        Self::write_indent(&mut out, indent);
                        line_start = false;
                    } else if !out.ends_with(' ') {
                        out.push(' ');
                    }
                    out.push_str(token);
                    out.push(' ');
                    i += 1;
                }
                _ => {
                    if line_start {
                        Self::write_indent(&mut out, indent);
                        line_start = false;
                    } else if !out.ends_with([' ', '\n', '(', '[']) {
                        out.push(' ');
                    }
                    out.push_str(token);
                    i += 1;
                }
            }
        }

        if !out.ends_with('\n') {
            out.push('\n');
        }

        out
    }

    fn build_stage_shader(&self, stage: Stage, func: &FunctionExpression) -> String {
        let mut sections: Vec<String> = Vec::new();
        sections.push(
            self.version_directive
                .clone()
                .unwrap_or_else(|| "#version 410 core".to_string()),
        );

        if !self.global_struct_decls.is_empty() {
            sections.push(self.global_struct_decls.join("\n"));
        }

        if !self.uniform_decls.is_empty() {
            sections.push(self.uniform_decls.join("\n"));
        }

        match stage {
            Stage::Vertex => {
                if !self.vertex_input_decls.is_empty() {
                    sections.push(self.vertex_input_decls.join("\n"));
                }
                if !self.vertex_output_decls.is_empty() {
                    sections.push(self.vertex_output_decls.join("\n"));
                }
            }
            Stage::Fragment => {
                if !self.fragment_input_decls.is_empty() {
                    sections.push(self.fragment_input_decls.join("\n"));
                }
                if !self.fragment_output_decls.is_empty() {
                    sections.push(self.fragment_output_decls.join("\n"));
                }
            }
            _ => {}
        }

        if !self.global_decls.is_empty() {
            sections.push(self.global_decls.join("\n"));
        }

        if !self.helper_functions.is_empty() {
            sections.push(self.helper_functions.join("\n"));
        }

        let input_param = match stage {
            Stage::Vertex => Self::find_param(&func.args, &self.vertex_input_struct),
            Stage::Fragment => Self::find_param(&func.args, &self.fragment_input_struct),
            _ => None,
        };
        let output_var = match stage {
            Stage::Vertex => {
                Self::detect_struct_variable(&func.body.expressions, &self.vertex_output_struct)
            }
            _ => None,
        };

        let ctx = BodyContext {
            input_param,
            output_var,
        };

        let body = self.format_function_body(&func.body.expressions, stage, &ctx);

        let mut source = sections.join("\n\n");
        if !source.is_empty() {
            source.push_str("\n\n");
        }
        source.push_str("void main() {\n");
        source.push_str(&body);
        source.push_str("}\n");
        source
    }

    fn build_helper_function(&self, func: &FunctionExpression) -> String {
        let mut source = String::new();
        let signature = format!(
            "{} {}({})",
            self.map_type(&func.return_type),
            func.name,
            self.format_function_args(&func.args)
        );
        source.push_str(&signature);
        source.push_str(" {\n");
        let ctx = BodyContext {
            input_param: None,
            output_var: None,
        };
        let body = self.format_function_body(&func.body.expressions, Stage::All, &ctx);
        source.push_str(&body);
        source.push_str("}\n");
        source
    }

    fn parse_uniform_like_builtin(args: &[crate::tokens::Token]) -> UniformAnnotation {
        let mut set = None;
        let mut binding = None;
        let mut index = 0;
        while index < args.len() {
            match args[index].content.as_str() {
                "set" => {
                    if index + 2 < args.len() {
                        set = args[index + 2].content.parse::<u32>().ok();
                        index += 3;
                        continue;
                    }
                }
                "binding" => {
                    if index + 2 < args.len() {
                        binding = args[index + 2].content.parse::<u32>().ok();
                        index += 3;
                        continue;
                    }
                }
                value => {
                    if set.is_none() {
                        set = value.parse::<u32>().ok();
                    } else if binding.is_none() {
                        binding = value.parse::<u32>().ok();
                    }
                }
            }
            index += 1;
        }

        UniformAnnotation { set, binding }
    }

    fn parse_buffer_builtin(args: &[crate::tokens::Token]) -> BufferAnnotation {
        let uniform = Self::parse_uniform_like_builtin(args);
        BufferAnnotation {
            _set: uniform.set,
            binding: uniform.binding,
        }
    }

    fn parse_output_builtin(args: &[crate::tokens::Token]) -> OutputAnnotation {
        let location = args
            .first()
            .and_then(|token| token.content.parse::<u32>().ok())
            .unwrap_or(0);
        OutputAnnotation { location }
    }

    fn parse_transform_builtin(args: &[crate::tokens::Token]) -> OpenGLTransformAnnotation {
        let block_name = args
            .iter()
            .find(|token| token.content.chars().all(|c| c.is_alphabetic() || c == '_'))
            .map(|token| token.content.clone());
        let max_elements = args
            .iter()
            .find_map(|token| token.content.parse::<usize>().ok())
            .unwrap_or(1);
        OpenGLTransformAnnotation {
            block_name,
            max_elements,
        }
    }

    fn parse_stage_builtin(args: &[crate::tokens::Token]) -> Option<StageAnnotation> {
        if args.len() < 2 {
            return None;
        }
        let stage = match args[0].content.as_str() {
            "vertex" => Stage::Vertex,
            "fragment" => Stage::Fragment,
            "compute" => Stage::Compute,
            "geometry" => Stage::Geometry,
            "mesh" => Stage::Mesh,
            "task" => Stage::Task,
            "tessellation" => Stage::TessellationEvaluation,
            "raytracing" => Stage::RaytracingAny,
            _ => Stage::All,
        };
        let io = match args[1].content.as_str() {
            "in" => StageIO::In,
            "out" => StageIO::Out,
            _ => StageIO::In,
        };
        Some(StageAnnotation { stage, io })
    }
}

impl Default for OpenGLBackend {
    fn default() -> Self {
        Self::new()
    }
}

impl Backend for OpenGLBackend {
    fn run_translatable(&mut self, expressions: &TranslatableExpression) -> String {
        if self.take_hana_version_annotation() {
            self.version_directive = Some(self.resolve_version(&expressions.expressions));
            return String::new();
        }

        let tokens = &expressions.expressions;
        let mut index = 0;
        while index < tokens.len() {
            match tokens[index].as_str() {
                "struct" => {
                    let consumed = self.parse_struct(&tokens[index..]);
                    if consumed == 0 {
                        break;
                    }
                    index += consumed;
                }
                ";" | "}" => {
                    index += 1;
                }
                _ => {
                    let consumed = self.parse_global_declaration(&tokens[index..]);
                    if consumed == 0 {
                        break;
                    }
                    index += consumed;
                }
            }
        }

        String::new()
    }

    fn run_builtin(&mut self, builtin: &Builtin) -> (String, Stage) {
        match builtin.name.as_str() {
            "hana" => {
                self.pending_annotations.push(Annotation::HanaVersion);
            }
            "uniform" => {
                let info = Self::parse_uniform_like_builtin(&builtin.args);
                self.pending_annotations.push(Annotation::Uniform(info));
            }
            "opengl" => {
                if let Some(token) = builtin.args.first() {
                    self.pending_annotations
                        .push(Annotation::OpenGLName(token.content.clone()));
                }
            }
            "stage" => {
                if let Some(stage) = Self::parse_stage_builtin(&builtin.args) {
                    self.pending_annotations.push(Annotation::Stage(stage));
                }
            }
            "push" => {
                self.pending_annotations.push(Annotation::PushConstant);
            }
            "align" => {
                if let Some(token) = builtin.args.first() {
                    self.pending_annotations
                        .push(Annotation::Align(token.content.clone()));
                }
            }
            "buffer" => {
                let info = Self::parse_buffer_builtin(&builtin.args);
                self.pending_annotations.push(Annotation::Buffer(info));
            }
            "openglTransformToUniform" => {
                let info = Self::parse_transform_builtin(&builtin.args);
                self.pending_annotations
                    .push(Annotation::OpenGLTransform(info));
            }
            "output" => {
                let info = Self::parse_output_builtin(&builtin.args);
                self.pending_annotations.push(Annotation::Output(info));
            }
            "vertex" => {
                self.current_function_stage = Stage::Vertex;
                return (String::new(), Stage::Vertex);
            }
            "fragment" => {
                self.current_function_stage = Stage::Fragment;
                return (String::new(), Stage::Fragment);
            }
            "tessellation" => {
                if let Some(arg) = builtin.args.first() {
                    match arg.content.as_str() {
                        "control" => {
                            self.current_function_stage = Stage::TessellationControl;
                            return (String::new(), Stage::TessellationControl);
                        }
                        "evaluation" => {
                            self.current_function_stage = Stage::TessellationEvaluation;
                            return (String::new(), Stage::TessellationEvaluation);
                        }
                        _ => {
                            throw_error(Error::InternalError(
								"Tessellation builtin requires a shader type argument (control/evaluation)"
									.to_string(),
							));
                            std::process::exit(1);
                        }
                    }
                }
            }
            "geometry" => {
                self.current_function_stage = Stage::Geometry;
                return (String::new(), Stage::Geometry);
            }
            "compute" => {
                self.current_function_stage = Stage::Compute;
                return (String::new(), Stage::Compute);
            }
            "mesh" => {
                self.current_function_stage = Stage::Mesh;
                return (String::new(), Stage::Mesh);
            }
            "task" => {
                self.current_function_stage = Stage::Task;
                return (String::new(), Stage::Task);
            }
            "raytracing" => {
                if !self.include_paths.contains(&"hana::raytracing".to_string()) {
                    throw_error(Error::InternalError(
                        "Raytracing extension not included.".to_string(),
                    ));
                    std::process::exit(1);
                }
                if let Some(shader_type) = builtin.args.first() {
                    let stage = match shader_type.content.as_str() {
                        "generation" => Stage::RaytracingGeneration,
                        "closest" => Stage::RaytracingClosest,
                        "any" => Stage::RaytracingAny,
                        "miss" => Stage::RaytracingMiss,
                        "intersection" => Stage::RaytracingIntersection,
                        "callable" => Stage::RaytracingCallable,
                        _ => {
                            throw_error(Error::InternalError(
								"Raytracing builtin requires a valid shader type argument (generation/closest/any/miss/intersection/callable)"
									.to_string(),
							));
                            std::process::exit(1);
                        }
                    };
                    self.current_function_stage = stage;
                    return (String::new(), stage);
                }
            }
            _ => {}
        }

        (String::new(), Stage::Same)
    }

    fn run_function(&mut self, func: &FunctionExpression) -> String {
        match self.current_function_stage {
            Stage::All | Stage::Same => {
                let helper = self.build_helper_function(func);
                self.helper_functions.push(helper);
                String::new()
            }
            stage => {
                self.stage_functions.insert(stage, func.clone());
                self.current_function_stage = Stage::All;
                String::new()
            }
        }
    }

    fn run_use(&mut self, use_expr: &UseExpression) -> String {
        if !self.include_paths.contains(&use_expr.module) {
            self.include_paths.push(use_expr.module.clone());
        }
        String::new()
    }

    fn finalize(&mut self, outputs: &mut Vec<(String, Stage)>) {
        let mut stage_sources: HashMap<Stage, String> = HashMap::new();
        for (stage, func) in &self.stage_functions {
            let source = self.build_stage_shader(*stage, func);
            stage_sources.insert(*stage, source);
        }

        for (content, stage) in outputs.iter_mut() {
            if let Some(source) = stage_sources.get(stage) {
                *content = source.clone();
            } else {
                content.clear();
            }
        }
    }
}
