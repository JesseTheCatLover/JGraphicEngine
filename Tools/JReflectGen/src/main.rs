//  Copyright 2025-2026 JesseTheCatLover.
//
// JReflectGen MVP Token Parser (macro-anchored; NOT a full C++ parser)
//
// Goals:
//  - Fast, no clang/libtooling.
//  - Robust to "real header mess": API macros, attributes, access labels, spacing.
//  - Support JCLASS/JSTRUCT/JENUM discovery.
//  - Support JPROPERTY + JFUNCTION scanning inside class/struct bodies.
//  - Support enum values scanning inside enum bodies.
//  - Token-based (much more reliable than line heuristics).
//
// MVP limitations (by design):
//  - Not a full C++ parser (templates/decltype/complex declarators are "raw strings").
//  - One reflected type per header is recommended, but we can detect multiple.
//  - JPROPERTY expects a single declarator ending with ';'.
//  - JFUNCTION supports typical member functions; overloads are allowed but signature is stored raw.
//  - Nested reflected types not supported yet.

use std::path::{Path, PathBuf};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum TypeKind {
    Class,
    Struct,
    Enum,
}

#[derive(Debug, Clone)]
struct Attr {
    name: String,
    args_raw: String, // raw token text inside (...) if present
}

#[derive(Debug, Clone)]
struct TypeRef {
    raw: String, // raw type tokens joined
}

#[derive(Debug, Clone)]
struct PropertyInfo {
    name: String,
    ty: TypeRef,
    attrs: Vec<Attr>,
    access: Option<String>,
}

#[derive(Debug, Clone)]
struct FunctionInfo {
    name: String,
    return_ty: Option<TypeRef>, // None for ctor/dtor
    params_raw: String,         // "(...)" raw
    tail_raw: String,           // "const noexcept override" etc. raw
    is_static: bool,
    is_virtual: bool,
    attrs: Vec<Attr>,
    access: Option<String>,
}

#[derive(Debug, Clone)]
struct EnumValueInfo {
    name: String,
    value_expr: Option<String>,
}

#[derive(Debug, Clone)]
struct EnumInfo {
    name: String,
    underlying_raw: Option<String>,
    is_scoped: bool,
    attrs: Vec<Attr>,
    values: Vec<EnumValueInfo>,
}

#[derive(Debug, Clone)]
struct ClassInfo {
    kind: TypeKind, // Class or Struct
    name: String,
    bases_raw: Option<String>, // raw after ':' until '{'
    attrs: Vec<Attr>,
    properties: Vec<PropertyInfo>,
    functions: Vec<FunctionInfo>,
}

#[derive(Debug, Clone)]
enum ReflectedDecl {
    Class(ClassInfo),
    Enum(EnumInfo),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum TokKind {
    Ident,
    Number,
    StringLit,
    Symbol, // single-char symbol or multi like "::", "->", "==", etc.
}

#[derive(Debug, Clone)]
struct Token {
    kind: TokKind,
    text: String,
    line: usize,
    col: usize,
}

#[derive(Debug)]
struct Diag {
    msg: String,
    line: usize,
    col: usize,
}

fn is_ident_start(c: char) -> bool {
    c == '_' || c.is_ascii_alphabetic()
}
fn is_ident_continue(c: char) -> bool {
    c == '_' || c.is_ascii_alphanumeric()
}

fn tokenize(src: &str) -> Vec<Token> {
    let mut out = Vec::new();

    let mut it = src.chars().peekable();
    let mut line: usize = 1;
    let mut col: usize = 1;

    while let Some(c) = it.next() {
        // track position at start of token
        let start_line = line;
        let start_col = col;

        // newline handling
        if c == '\n' {
            line += 1;
            col = 1;
            continue;
        }

        // whitespace
        if c.is_whitespace() {
            col += 1;
            continue;
        }

        // line comment //
        if c == '/' && it.peek() == Some(&'/') {
            // consume second '/'
            it.next();
            col += 2;
            while let Some(cc) = it.next() {
                if cc == '\n' {
                    line += 1;
                    col = 1;
                    break;
                }
                col += 1;
            }
            continue;
        }

        // block comment /* ... */
        if c == '/' && it.peek() == Some(&'*') {
            it.next();
            col += 2;
            while let Some(cc) = it.next() {
                if cc == '\n' {
                    line += 1;
                    col = 1;
                    continue;
                }
                col += 1;
                if cc == '*' && it.peek() == Some(&'/') {
                    it.next();
                    col += 1;
                    break;
                }
            }
            continue;
        }

        // preprocessor: if '#' at beginning of line (col==1) treat as skip-to-eol
        if c == '#' && start_col == 1 {
            col += 1;
            while let Some(cc) = it.next() {
                if cc == '\n' {
                    line += 1;
                    col = 1;
                    break;
                }
                col += 1;
            }
            continue;
        }

        // string literal "..."
        if c == '"' {
            let mut s = String::new();
            s.push(c);
            col += 1;

            while let Some(cc) = it.next() {
                s.push(cc);
                if cc == '\n' {
                    line += 1;
                    col = 1;
                } else {
                    col += 1;
                }

                if cc == '\\' {
                    // escape next char if any
                    if let Some(ee) = it.next() {
                        s.push(ee);
                        if ee == '\n' {
                            line += 1;
                            col = 1;
                        } else {
                            col += 1;
                        }
                    }
                    continue;
                }

                if cc == '"' {
                    break;
                }
            }

            out.push(Token {
                kind: TokKind::StringLit,
                text: s,
                line: start_line,
                col: start_col,
            });
            continue;
        }

        // identifier
        if is_ident_start(c) {
            let mut s = String::new();
            s.push(c);
            col += 1;

            while let Some(&cc) = it.peek() {
                if is_ident_continue(cc) {
                    it.next();
                    s.push(cc);
                    col += 1;
                } else {
                    break;
                }
            }

            out.push(Token {
                kind: TokKind::Ident,
                text: s,
                line: start_line,
                col: start_col,
            });
            continue;
        }

        // number (very basic)
        if c.is_ascii_digit() {
            let mut s = String::new();
            s.push(c);
            col += 1;

            while let Some(&cc) = it.peek() {
                if cc.is_ascii_alphanumeric() || cc == '.' || cc == '_' {
                    it.next();
                    s.push(cc);
                    col += 1;
                } else {
                    break;
                }
            }

            out.push(Token {
                kind: TokKind::Number,
                text: s,
                line: start_line,
                col: start_col,
            });
            continue;
        }

        // symbols (try multi-char combos)
        let mut sym = String::new();
        sym.push(c);

        let peek2 = it.peek().copied();
        if let Some(n) = peek2 {
            let two = format!("{}{}", c, n);
            let is_two = matches!(
                two.as_str(),
                "::" | "->" | "==" | "!=" | "<=" | ">=" | "&&" | "||" | "++" | "--" | "+=" | "-=" | "*=" | "/="
                    | "&=" | "|=" | "^=" | "<<" | ">>" | "<<=" | ">>=" | "..." | "##"
            );
            if is_two {
                it.next();
                sym.push(n);
                col += 2;
                out.push(Token {
                    kind: TokKind::Symbol,
                    text: sym,
                    line: start_line,
                    col: start_col,
                });
                continue;
            }
        }

        col += 1;
        out.push(Token {
            kind: TokKind::Symbol,
            text: sym,
            line: start_line,
            col: start_col,
        });
    }

    out
}

fn tok_text(tokens: &[Token]) -> String {
    // join with single spaces, but try to avoid ugly spacing around :: and punctuation
    let mut out = String::new();
    let mut prev: Option<&str> = None;

    for t in tokens {
        let s = t.text.as_str();
        let no_space_before = matches!(s, ")" | "]" | "}" | "," | ";" | ":" | "::" | ">" | ">>" | "..." );
        let no_space_after_prev = match prev {
            None => true,
            Some(p) => matches!(p, "(" | "[" | "{" | "::" | "<" | "<<" ),
        };

        if !out.is_empty() && !no_space_before && !no_space_after_prev {
            out.push(' ');
        }
        out.push_str(s);
        prev = Some(s);
    }
    out
}

fn is_kw(tok: &Token, kw: &str) -> bool {
    tok.kind == TokKind::Ident && tok.text == kw
}

fn parse_paren_group(tokens: &[Token], i: &mut usize) -> Option<Vec<Token>> {
    if *i >= tokens.len() || tokens[*i].text != "(" {
        return None;
    }
    let start = *i;
    *i += 1;
    let mut depth = 1;

    while *i < tokens.len() {
        let t = &tokens[*i];
        if t.text == "(" {
            depth += 1;
        } else if t.text == ")" {
            depth -= 1;
            if depth == 0 {
                // return inside tokens only
                let inside = tokens[(start + 1)..*i].to_vec();
                *i += 1;
                return Some(inside);
            }
        }
        *i += 1;
    }
    None
}

fn parse_brace_group(tokens: &[Token], i: &mut usize) -> Option<Vec<Token>> {
    if *i >= tokens.len() || tokens[*i].text != "{" {
        return None;
    }
    let start = *i;
    *i += 1;
    let mut depth = 1;

    while *i < tokens.len() {
        let t = &tokens[*i];
        if t.text == "{" {
            depth += 1;
        } else if t.text == "}" {
            depth -= 1;
            if depth == 0 {
                let inside = tokens[(start + 1)..*i].to_vec();
                *i += 1;
                return Some(inside);
            }
        }
        *i += 1;
    }
    None
}

fn parse_marker_attrs(tokens: &[Token], i: &mut usize, marker_name: &str) -> Option<Vec<Attr>> {
    if *i >= tokens.len() || !is_kw(&tokens[*i], marker_name) {
        return None;
    }
    *i += 1;

    // optional "( ... )"
    let args = if *i < tokens.len() && tokens[*i].text == "(" {
        let inside = parse_paren_group(tokens, i).unwrap_or_default();
        let raw = tok_text(&inside);
        vec![Attr {
            name: marker_name.to_string(),
            args_raw: raw,
        }]
    } else {
        vec![Attr {
            name: marker_name.to_string(),
            args_raw: String::new(),
        }]
    };

    Some(args)
}

fn skip_until(tokens: &[Token], i: &mut usize, stop: &[&str]) {
    while *i < tokens.len() {
        let s = tokens[*i].text.as_str();
        if stop.iter().any(|x| *x == s) {
            break;
        }
        *i += 1;
    }
}

fn parse_enum_decl(tokens: &[Token], i: &mut usize, attrs: Vec<Attr>) -> Result<EnumInfo, Diag> {
    // expecting: enum [class|struct] Name [ : underlying ] { ... }
    if *i >= tokens.len() || !is_kw(&tokens[*i], "enum") {
        let t = tokens.get(*i).cloned();
        return Err(Diag {
            msg: "Expected 'enum' after JENUM".to_string(),
            line: t.as_ref().map(|x| x.line).unwrap_or(0),
            col: t.as_ref().map(|x| x.col).unwrap_or(0),
        });
    }
    *i += 1;

    let mut is_scoped = false;
    if *i < tokens.len() && (is_kw(&tokens[*i], "class") || is_kw(&tokens[*i], "struct")) {
        is_scoped = true;
        *i += 1;
    }

    let name_tok = tokens.get(*i).cloned().ok_or(Diag {
        msg: "Expected enum name".to_string(),
        line: 0,
        col: 0,
    })?;
    if name_tok.kind != TokKind::Ident {
        return Err(Diag {
            msg: "Expected enum name identifier".to_string(),
            line: name_tok.line,
            col: name_tok.col,
        });
    }
    let name = name_tok.text.clone();
    *i += 1;

    // optional ": underlying"
    let mut underlying_raw: Option<String> = None;
    if *i < tokens.len() && tokens[*i].text == ":" {
        *i += 1;
        let start = *i;
        skip_until(tokens, i, &["{"]);
        let under = tok_text(&tokens[start..*i]);
        underlying_raw = if under.trim().is_empty() { None } else { Some(under) };
    }

    // body
    if *i >= tokens.len() || tokens[*i].text != "{" {
        let t = tokens.get(*i).cloned();
        return Err(Diag {
            msg: "Expected '{' to start enum body".to_string(),
            line: t.as_ref().map(|x| x.line).unwrap_or(name_tok.line),
            col: t.as_ref().map(|x| x.col).unwrap_or(name_tok.col),
        });
    }

    let body = parse_brace_group(tokens, i).ok_or(Diag {
        msg: "Unterminated enum body".to_string(),
        line: name_tok.line,
        col: name_tok.col,
    })?;

    let values = parse_enum_values(&body);

    Ok(EnumInfo {
        name,
        underlying_raw,
        is_scoped,
        attrs,
        values,
    })
}

fn parse_enum_values(body: &[Token]) -> Vec<EnumValueInfo> {
    // very token-simple:
    // Enumerator := Ident [ '=' <expr tokens> ] (',' | end)
    let mut out = Vec::new();
    let mut i = 0;

    while i < body.len() {
        // skip stray commas
        while i < body.len() && body[i].text == "," {
            i += 1;
        }
        if i >= body.len() {
            break;
        }

        if body[i].kind != TokKind::Ident {
            // not an enumerator start; skip token
            i += 1;
            continue;
        }

        let name = body[i].text.clone();
        i += 1;

        let mut value_expr: Option<String> = None;
        if i < body.len() && body[i].text == "=" {
            i += 1;
            let start = i;
            while i < body.len() && body[i].text != "," {
                // stop at end-of-enum-body handled by loop
                i += 1;
            }
            let expr = tok_text(&body[start..i]).trim().to_string();
            if !expr.is_empty() {
                value_expr = Some(expr);
            }
        } else {
            // consume tokens until ',' if weird stuff occurs (defensive)
            while i < body.len() && body[i].text != "," {
                // if we hit another ident directly, likely next enumerator missing comma; break
                if body[i].kind == TokKind::Ident {
                    break;
                }
                i += 1;
            }
        }

        out.push(EnumValueInfo { name, value_expr });
    }

    out
}

fn parse_class_or_struct_decl(
    tokens: &[Token],
    i: &mut usize,
    kind: TypeKind,
    attrs: Vec<Attr>,
) -> Result<ClassInfo, Diag> {
    // expecting: class|struct Name [ : ... ] { ... }
    let kw = match kind {
        TypeKind::Class => "class",
        TypeKind::Struct => "struct",
        _ => unreachable!(),
    };

    // Find the actual 'class'/'struct' keyword (tolerate export macros in between)
    while *i < tokens.len() && !is_kw(&tokens[*i], kw) {
        // stop if we hit '{' unexpectedly
        if tokens[*i].text == "{" {
            break;
        }
        *i += 1;
    }

    if *i >= tokens.len() || !is_kw(&tokens[*i], kw) {
        let t = tokens.get(*i).cloned();
        return Err(Diag {
            msg: format!("Expected '{}' after marker", kw),
            line: t.as_ref().map(|x| x.line).unwrap_or(0),
            col: t.as_ref().map(|x| x.col).unwrap_or(0),
        });
    }
    *i += 1;

    // Name
    let name_tok = tokens.get(*i).cloned().ok_or(Diag {
        msg: "Expected type name".to_string(),
        line: 0,
        col: 0,
    })?;
    if name_tok.kind != TokKind::Ident {
        return Err(Diag {
            msg: "Expected type name identifier".to_string(),
            line: name_tok.line,
            col: name_tok.col,
        });
    }
    let name = name_tok.text.clone();
    *i += 1;

    // optional inheritance: ':' ... until '{'
    let mut bases_raw: Option<String> = None;
    if *i < tokens.len() && tokens[*i].text == ":" {
        *i += 1;
        let start = *i;
        skip_until(tokens, i, &["{"]);
        let raw = tok_text(&tokens[start..*i]).trim().to_string();
        if !raw.is_empty() {
            bases_raw = Some(raw);
        }
    }

    // body
    while *i < tokens.len() && tokens[*i].text != "{" {
        *i += 1;
    }
    if *i >= tokens.len() || tokens[*i].text != "{" {
        return Err(Diag {
            msg: "Expected '{' to start class/struct body".to_string(),
            line: name_tok.line,
            col: name_tok.col,
        });
    }

    let body = parse_brace_group(tokens, i).ok_or(Diag {
        msg: "Unterminated class/struct body".to_string(),
        line: name_tok.line,
        col: name_tok.col,
    })?;

    let (properties, functions) = parse_members(&name, &body);

    Ok(ClassInfo {
        kind,
        name,
        bases_raw,
        attrs,
        properties,
        functions,
    })
}

fn parse_members(class_name: &str, body: &[Token]) -> (Vec<PropertyInfo>, Vec<FunctionInfo>) {
    let mut props = Vec::new();
    let mut funcs = Vec::new();

    let mut i = 0;
    let mut current_access: Option<String> = None;

    while i < body.len() {
        // access labels: public: private: protected:
        if body[i].kind == TokKind::Ident
            && (body[i].text == "public" || body[i].text == "private" || body[i].text == "protected")
        {
            if i + 1 < body.len() && body[i + 1].text == ":" {
                current_access = Some(body[i].text.clone());
                i += 2;
                continue;
            }
        }

        // JPROPERTY
        if let Some(attrs) = parse_marker_attrs(body, &mut i, "JPROPERTY") {
            if let Some((prop, new_i)) = parse_property_after_marker(body, i, attrs, current_access.clone())
            {
                props.push(prop);
                i = new_i;
                continue;
            }
            // if parse failed, keep scanning to avoid infinite loops
            continue;
        }

        // JFUNCTION
        if let Some(attrs) = parse_marker_attrs(body, &mut i, "JFUNCTION") {
            if let Some((fun, new_i)) =
                parse_function_after_marker(body, i, class_name, attrs, current_access.clone())
            {
                funcs.push(fun);
                i = new_i;
                continue;
            }
            continue;
        }

        i += 1;
    }

    (props, funcs)
}

fn parse_property_after_marker(
    tokens: &[Token],
    mut i: usize,
    attrs: Vec<Attr>,
    access: Option<String>,
) -> Option<(PropertyInfo, usize)> {
    // Skip attributes like [[nodiscard]] (tokenized as symbols/idents), and 'static_assert' etc.
    // We look for a declaration that ends with ';' and DOES NOT contain '(' at top-level.
    // We also stop if we hit another marker before finding a decl.
    let mut decl = Vec::<Token>::new();
    let mut paren_depth = 0usize;

    while i < tokens.len() {
        // stop if another marker appears
        if tokens[i].kind == TokKind::Ident
            && (tokens[i].text == "JPROPERTY"
            || tokens[i].text == "JFUNCTION"
            || tokens[i].text == "JCLASS"
            || tokens[i].text == "JSTRUCT"
            || tokens[i].text == "JENUM")
        {
            return None;
        }

        let t = &tokens[i];

        if t.text == "(" {
            paren_depth += 1;
        } else if t.text == ")" && paren_depth > 0 {
            paren_depth -= 1;
        }

        decl.push(t.clone());

        if t.text == ";" && paren_depth == 0 {
            break;
        }
        i += 1;
    }

    if decl.is_empty() || decl.last()?.text != ";" {
        return None;
    }

    // Reject if contains '(' at top-level (likely function)
    // (Constructor init / attributes could have parens; but MVP rule: properties shouldn't.)
    if decl.iter().any(|t| t.text == "(") {
        return None;
    }

    // Remove trailing ';'
    decl.pop();

    // If there is '=', drop initializer tokens from '=' onward
    let mut end = decl.len();
    for (idx, t) in decl.iter().enumerate() {
        if t.text == "=" {
            end = idx;
            break;
        }
    }
    let decl = decl[..end].to_vec();

    // Name heuristic: last identifier token is the field name
    let mut name_idx: Option<usize> = None;
    for (idx, t) in decl.iter().enumerate() {
        if t.kind == TokKind::Ident {
            name_idx = Some(idx);
        }
    }
    let name_idx = name_idx?;
    let name = decl[name_idx].text.clone();

    // Type is everything before name
    let ty_tokens = decl[..name_idx].to_vec();
    let ty_raw = tok_text(&ty_tokens).trim().to_string();

    if ty_raw.is_empty() {
        return None;
    }

    Some((
        PropertyInfo {
            name,
            ty: TypeRef { raw: ty_raw },
            attrs,
            access,
        },
        i + 1,
    ))
}

fn parse_function_after_marker(
    tokens: &[Token],
    mut i: usize,
    class_name: &str,
    attrs: Vec<Attr>,
    access: Option<String>,
) -> Option<(FunctionInfo, usize)> {
    // Parse "return name (params) tail ;" or with inline body "{...}"
    // We store mostly raw; we identify:
    //  - name: token before '('
    //  - return type: tokens before name (unless ctor/dtor)
    //  - params_raw: raw "(...)"
    //  - tail_raw: tokens after ')' until ';' or '{'
    //  - flags: static/virtual

    // Gather tokens until we hit '(' that belongs to function
    // Stop if another marker appears before we see '('
    let mut head = Vec::<Token>::new();
    while i < tokens.len() {
        if tokens[i].kind == TokKind::Ident
            && (tokens[i].text == "JPROPERTY"
            || tokens[i].text == "JFUNCTION"
            || tokens[i].text == "JCLASS"
            || tokens[i].text == "JSTRUCT"
            || tokens[i].text == "JENUM")
        {
            return None;
        }

        if tokens[i].text == "(" {
            break;
        }
        // stop on ';' (not a function)
        if tokens[i].text == ";" {
            return None;
        }

        head.push(tokens[i].clone());
        i += 1;
    }

    if i >= tokens.len() || tokens[i].text != "(" {
        return None;
    }

    // name is last identifier in head
    let mut name_idx: Option<usize> = None;
    for (idx, t) in head.iter().enumerate() {
        if t.kind == TokKind::Ident {
            name_idx = Some(idx);
        }
    }
    let name_idx = name_idx?;
    let name = head[name_idx].text.clone();

    // params group
    let params_inside = parse_paren_group(tokens, &mut i)?;
    let params_raw = format!("({})", tok_text(&params_inside));

    // tail tokens until ';' or '{'
    let mut tail = Vec::<Token>::new();
    while i < tokens.len() {
        if tokens[i].text == ";" || tokens[i].text == "{" {
            break;
        }
        tail.push(tokens[i].clone());
        i += 1;
    }
    let tail_raw = tok_text(&tail).trim().to_string();

    // determine static/virtual from head tokens
    let is_static = head.iter().any(|t| t.kind == TokKind::Ident && t.text == "static");
    let is_virtual = head.iter().any(|t| t.kind == TokKind::Ident && t.text == "virtual");

    // return type tokens = head[..name_idx], minus 'static'/'virtual'/friend/inline/constexpr etc
    // MVP: just drop a few known specifiers
    let mut ret_tokens: Vec<Token> = head[..name_idx].to_vec();
    ret_tokens.retain(|t| {
        !(t.kind == TokKind::Ident
            && matches!(
                t.text.as_str(),
                "static" | "virtual" | "inline" | "constexpr" | "friend" | "explicit" | "FORCEINLINE"
            ))
    });
    let ret_raw = tok_text(&ret_tokens).trim().to_string();

    // ctor/dtor detection: name == class_name or ~class_name
    let is_ctor = name == class_name;
    let is_dtor = name == format!("~{}", class_name);
    let return_ty = if is_ctor || is_dtor || ret_raw.is_empty() {
        None
    } else {
        Some(TypeRef { raw: ret_raw })
    };

    // If inline body: skip brace group
    if i < tokens.len() && tokens[i].text == "{" {
        let _ = parse_brace_group(tokens, &mut i);
    } else if i < tokens.len() && tokens[i].text == ";" {
        i += 1;
    }

    Some((
        FunctionInfo {
            name,
            return_ty,
            params_raw,
            tail_raw,
            is_static,
            is_virtual,
            attrs,
            access,
        },
        i,
    ))
}

fn parse_reflected_first(tokens: &[Token]) -> Result<Option<ReflectedDecl>, Diag> {
    // Find first JCLASS/JSTRUCT/JENUM marker and parse its following declaration.
    let mut i = 0;
    while i < tokens.len() {
        if is_kw(&tokens[i], "JCLASS") {
            let attrs = parse_marker_attrs(tokens, &mut i, "JCLASS").unwrap_or_default();
            // after marker: allow other macros/idents until we see "class"
            let ci = parse_class_or_struct_decl(tokens, &mut i, TypeKind::Class, attrs)?;
            return Ok(Some(ReflectedDecl::Class(ci)));
        }
        if is_kw(&tokens[i], "JSTRUCT") {
            let attrs = parse_marker_attrs(tokens, &mut i, "JSTRUCT").unwrap_or_default();
            let ci = parse_class_or_struct_decl(tokens, &mut i, TypeKind::Struct, attrs)?;
            return Ok(Some(ReflectedDecl::Class(ci)));
        }
        if is_kw(&tokens[i], "JENUM") {
            let attrs = parse_marker_attrs(tokens, &mut i, "JENUM").unwrap_or_default();
            // tolerate export macros until "enum"
            while i < tokens.len() && !is_kw(&tokens[i], "enum") {
                i += 1;
            }
            let ei = parse_enum_decl(tokens, &mut i, attrs)?;
            return Ok(Some(ReflectedDecl::Enum(ei)));
        }
        i += 1;
    }
    Ok(None)
}

/// Helper: "Source/Scene/JActor.h" -> ("JActor.refl.generated.h", "JActor.refl.gen.cpp")
pub fn make_rel_gen_paths(header: &str) -> (PathBuf, PathBuf) {
    let p = Path::new(header);
    let stem = p.file_stem().unwrap_or_default().to_string_lossy();

    let gen_h_name = format!("{}.refl.generated.h", stem);
    let gen_cpp_name = format!("{}.refl.gen.cpp", stem);

    (PathBuf::from(gen_h_name), PathBuf::from(gen_cpp_name))
}

fn main() {
    let mut args = std::env::args().skip(1).collect::<Vec<_>>();
    if args.is_empty() {
        eprintln!("Usage: jreflect_parse <Header1.h> [Header2.h ...]");
        std::process::exit(2);
    }

    for path in args.drain(..) {
        let src = match std::fs::read_to_string(&path) {
            Ok(s) => s,
            Err(e) => {
                eprintln!("\n== {} ==\nERROR: failed to read: {}", path, e);
                continue;
            }
        };

        let tokens = tokenize(&src);

        match parse_reflected_first(&tokens) {
            Ok(Some(decl)) => {
                let (gh, gcpp) = make_rel_gen_paths(&path);
                println!("\n== {} ==\nGEN: {:?}, {:?}\n{:#?}", path, gh, gcpp, decl);
            }
            Ok(None) => {
                println!("\n== {} ==\n(no reflected marker found)", path);
            }
            Err(d) => {
                eprintln!(
                    "\n== {} ==\nERROR at {}:{}: {}",
                    path, d.line, d.col, d.msg
                );
            }
        }
    }
}