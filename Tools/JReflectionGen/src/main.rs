//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

use std::{
    env,
    fs,
    path::{Path, PathBuf},
};

#[derive(Debug)]
struct Property {
    name: String,
    meta: String, // raw text inside JPROPERTY(...)
}

#[derive(Debug)]
struct TypeDecl {
    name: String,
    base: String,
    header_path: String, // as given (for includes)
    properties: Vec<Property>,
}

fn main() {
    // Usage:
    // jreflectgen --out <gen_dir> --headers <h1>;<h2>;<h3>
    // Example headers list is semicolon-separated so CMake can pass it easily.
    let mut out_dir: Option<PathBuf> = None;
    let mut headers: Vec<String> = vec![];

    let args: Vec<String> = env::args().collect();
    let mut i = 1;
    while i < args.len() {
        match args[i].as_str() {
            "--out" => {
                i += 1;
                out_dir = Some(PathBuf::from(&args[i]));
            }
            "--headers" => {
                i += 1;
                headers = args[i]
                    .split(';')
                    .filter(|s| !s.trim().is_empty())
                    .map(|s| s.to_string())
                    .collect();
            }
            _ => {}
        }
        i += 1;
    }

    let out_dir = out_dir.expect("Missing --out <dir>");
    if headers.is_empty() {
        eprintln!("No headers passed. Use --headers h1;h2;h3");
        std::process::exit(1);
    }

    fs::create_dir_all(&out_dir).expect("Failed to create output dir");

    let mut gen_cpp_files: Vec<PathBuf> = vec![];

    for h in headers {
        let src = fs::read_to_string(&h).unwrap_or_else(|_| panic!("Failed to read header: {}", h));
        let Some(t) = parse_header_for_type(&src, &h) else {
            // Not all headers are reflected; skip quietly.
            continue;
        };

        let rel = make_rel_gen_paths(&h);
        let gen_h = out_dir.join(rel.gen_h);
        let gen_cpp = out_dir.join(rel.gen_cpp);

        fs::create_dir_all(gen_h.parent().unwrap()).ok();
        fs::create_dir_all(gen_cpp.parent().unwrap()).ok();

        emit_generated_h(&gen_h, &t).expect("emit .generated.h failed");
        emit_generated_cpp(&gen_cpp, &t).expect("emit .gen.cpp failed");

        gen_cpp_files.push(gen_cpp);
    }

    // Optional: write a manifest for CMake (list of generated .cpp files)
    let manifest_path = out_dir.join("reflection_manifest.txt");
    let mut manifest = String::new();
    for p in &gen_cpp_files {
        manifest.push_str(&p.to_string_lossy());
        manifest.push('\n');
    }
    fs::write(manifest_path, manifest).expect("Failed to write manifest");
}

struct RelPaths {
    gen_h: PathBuf,
    gen_cpp: PathBuf,
}

// Maps "Source/Scene/JActor.h" -> "Source/Scene/JActor.refl.generated.h" etc
fn make_rel_gen_paths(header: &str) -> RelPaths {
    let p = Path::new(header);
    let parent = p.parent().unwrap_or_else(|| Path::new(""));
    let stem = p.file_stem().unwrap().to_string_lossy();

    let gen_h_name = format!("{}.refl.generated.h", stem);
    let gen_cpp_name = format!("{}.refl.gen.cpp", stem);

    RelPaths {
        gen_h: parent.join(gen_h_name),
        gen_cpp: parent.join(gen_cpp_name),
    }
}
fn parse_header_for_type(src: &str, header_path: &str) -> Option<TypeDecl> {
    let lines: Vec<&str> = src.lines().collect();

    // 1) Find JCLASS()
    let class_idx = lines
        .iter()
        .position(|l| l.contains("JCLASS"))?;

    // 2) Find the class declaration after it (robust: allow macros, multi-line)
    let mut decl_accum = String::new();
    let mut found_any = false;

    for j in class_idx..lines.len() {
        let l = lines[j].trim();
        if l.is_empty() {
            continue;
        }

        // If we see another marker before we found a class decl, stop.
        if j != class_idx && l.contains("JCLASS") && !found_any {
            break;
        }

        // Accumulate a small window of lines so we can handle:
        // JCLASS()
        // API_EXPORT
        // class JActor : public Base
        //
        // or:
        // API_EXPORT class JActor : public Base
        if !decl_accum.is_empty() {
            decl_accum.push(' ');
        }
        decl_accum.push_str(l);

        // Start searching only once we see "class "
        if decl_accum.contains("class ") {
            found_any = true;
        }

        // If we have a class decl and it looks complete enough, try parse it.
        // "complete enough" here = contains ':'
        if found_any && decl_accum.contains(':') {
            if let Some((name, base)) = parse_class_decl_anywhere(&decl_accum) {
                // 3) Collect properties
                let mut props: Vec<Property> = vec![];
                let mut pending_meta: Option<String> = None;

                for l2 in &lines {
                    let t = l2.trim();

                    if let Some(meta) = parse_jproperty(t) {
                        pending_meta = Some(meta);
                        continue;
                    }

                    if let Some(meta) = pending_meta.take() {
                        if let Some(field_name) = parse_field_name(t) {
                            props.push(Property { name: field_name, meta });
                        }
                    }
                }

                return Some(TypeDecl {
                    name,
                    base,
                    header_path: header_path.replace('\\', "/"),
                    properties: props,
                });
            }
        }

        // Safety: don't accumulate forever; a class decl shouldn't exceed a few lines.
        if decl_accum.len() > 1024 {
            break;
        }
    }

    None
}

// Like parse_class_decl, but it can find "class " even if it's not at the start
fn parse_class_decl_anywhere(s: &str) -> Option<(String, String)> {
    // Find "class " anywhere
    let idx = s.find("class ")?;
    let after_class = &s[(idx + "class ".len())..];

    // Expect "Name : public Base"
    let parts: Vec<&str> = after_class.split(':').collect();
    if parts.len() < 2 {
        return None;
    }

    let name = parts[0].trim().split_whitespace().next()?.to_string();

    let right = parts[1];
    let base = right
        .split_whitespace()
        .skip_while(|x| *x != "public")
        .skip(1)
        .next()?
        .trim()
        .trim_end_matches(',')
        .trim_end_matches('{')
        .to_string();

    Some((name, base))
}

fn parse_class_decl(line: &str) -> Option<(String, String)> {
    // "class JActor : public JCoreObject"
    let s = line.trim().trim_end_matches('{').trim();
    let after_class = s.strip_prefix("class ")?;
    let parts: Vec<&str> = after_class.split(':').collect();
    if parts.len() < 2 {
        return None;
    }
    let name = parts[0].trim().split_whitespace().next()?.to_string();
    let right = parts[1];
    // Expect "... public Base"
    let base = right
        .split_whitespace()
        .skip_while(|x| *x != "public")
        .skip(1)
        .next()?
        .trim()
        .trim_end_matches(',')
        .to_string();

    Some((name, base))
}

fn parse_jproperty(line: &str) -> Option<String> {
    // "JPROPERTY(...)" capture inside parentheses
    let start = line.find("JPROPERTY")?;
    let after = &line[start..];
    let open = after.find('(')?;
    let close = after.rfind(')')?;
    if close <= open {
        return None;
    }
    Some(after[(open + 1)..close].trim().to_string())
}

fn parse_field_name(line: &str) -> Option<String> {
    // super simple: must end with ';', ignore functions
    let t = line.trim();
    if !t.ends_with(';') {
        return None;
    }
    if t.contains('(') {
        return None; // function decl
    }
    // Grab last "word" before ';' (handles pointers/references)
    let no_semi = t.trim_end_matches(';').trim();
    let tokens: Vec<&str> = no_semi.split_whitespace().collect();
    let last = tokens.last()?.trim();
    // handle "m_Name" or "*m_Name"
    let field = last.trim_start_matches('*').trim_start_matches('&');
    // ignore bitfields like "x : 1" (MVP)
    if field == ":" {
        return None;
    }
    Some(field.to_string())
}

fn emit_generated_h(path: &Path, t: &TypeDecl) -> std::io::Result<()> {
    let mut out = String::new();
    out.push_str("#pragma once\n");
    out.push_str("// generated - do not edit\n\n");
    out.push_str(&format!("struct {}_Reflection\n", t.name));
    out.push_str("{\n");
    out.push_str("    static void Register();\n");
    out.push_str("};\n");
    fs::write(path, out)
}

fn emit_generated_cpp(path: &Path, t: &TypeDecl) -> std::io::Result<()> {
    let mut out = String::new();
    out.push_str("// generated - do not edit\n\n");
    out.push_str(&format!("#include \"{}\"\n", t.header_path));
    out.push_str("#include \"Core/Reflection/RETypeRegistry.h\"\n");
    out.push_str("#include \"Core/Reflection/JReflectionMetaData.h\"\n\n");

    out.push_str(&format!(
        "static struct _AutoReg_{} {{ _AutoReg_{}() {{ {}_Reflection::Register(); }} }} _AutoReg_{}_Instance;\n\n",
        t.name, t.name, t.name, t.name
    ));

    out.push_str(&format!("void {}_Reflection::Register()\n", t.name));
    out.push_str("{\n");
    out.push_str(&format!(
        "    RETypeRegistry::BeginType(\"{}\", typeid({}), typeid({}));\n",
        t.name, t.name, t.base
    ));
    out.push_str(&format!("    RETypeRegistry::SetDefaultFactory<{}>();\n", t.name));
    out.push('\n');

    for p in &t.properties {
        let meta = if p.meta.trim().is_empty() {
            "J_META_NONE()".to_string()
        } else {
            format!("J_META_PACK({})", p.meta.trim())
        };
        out.push_str(&format!(
            "    RETypeRegistry::AddProperty<{}>(\"{}\", &{}::{}, {});\n",
            t.name, p.name, t.name, p.name, meta
        ));
    }

    out.push_str("}\n");
    fs::write(path, out)
}