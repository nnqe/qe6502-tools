use std::env;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};

struct NativeSource {
    include_dir: PathBuf,
    src_dir: PathBuf,
}

fn main() {
    let manifest_dir = PathBuf::from(
        env::var_os("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR is set by Cargo"),
    );

    let native_source = resolve_native_source(&manifest_dir);
    emit_rerun_if_changed(&native_source);

    let out_dir = PathBuf::from(env::var_os("OUT_DIR").expect("OUT_DIR is set by Cargo"));
    let staged_native_dir = out_dir.join("native");
    stage_native_source(&native_source, &staged_native_dir)
        .expect("failed to stage qe6502 native source for Rust build");

    let staged_include_dir = staged_native_dir.join("include");
    let staged_src_dir = staged_native_dir.join("src");
    let staged_source_file = staged_src_dir.join("qe6502.c");

    cc::Build::new()
        .file(staged_source_file)
        .include(staged_include_dir)
        .include(staged_src_dir)
        .define("QE6502_STATIC", "1")
        .std("c11")
        .compile("qe6502");
}

fn resolve_native_source(manifest_dir: &Path) -> NativeSource {
    let repo_cpu_dir = manifest_dir.join("..").join("..").join("cpu");
    let repo_source_file = repo_cpu_dir.join("src").join("qe6502.c");

    if repo_source_file.is_file() {
        return NativeSource {
            include_dir: repo_cpu_dir.join("include"),
            src_dir: repo_cpu_dir.join("src"),
        };
    }

    let packaged_native_dir = manifest_dir.join("native");
    let packaged_source_file = packaged_native_dir.join("src").join("qe6502.c");

    if packaged_source_file.is_file() {
        return NativeSource {
            include_dir: packaged_native_dir.join("include"),
            src_dir: packaged_native_dir.join("src"),
        };
    }

    panic!(
        "qe6502 native source not found; expected canonical repository source at {} or packaged source at {}",
        repo_source_file.display(),
        packaged_source_file.display()
    );
}

fn emit_rerun_if_changed(native_source: &NativeSource) {
    println!(
        "cargo:rerun-if-changed={}",
        native_source.src_dir.join("qe6502.c").display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        native_source
            .include_dir
            .join("qe6502")
            .join("qe6502.h")
            .display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        native_source
            .include_dir
            .join("qe6502")
            .join("qe6502_version.h")
            .display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        native_source
            .include_dir
            .join("qe6502")
            .join("qe6502_abi.h")
            .display()
    );
    println!(
        "cargo:rerun-if-changed={}",
        native_source.src_dir.join("control_store").display()
    );
}

fn stage_native_source(native_source: &NativeSource, staged_native_dir: &Path) -> io::Result<()> {
    if staged_native_dir.exists() {
        fs::remove_dir_all(staged_native_dir)?;
    }

    copy_file(
        &native_source.src_dir.join("qe6502.c"),
        &staged_native_dir.join("src").join("qe6502.c"),
    )?;
    copy_file(
        &native_source
            .include_dir
            .join("qe6502")
            .join("qe6502.h"),
        &staged_native_dir
            .join("include")
            .join("qe6502")
            .join("qe6502.h"),
    )?;
    copy_file(
        &native_source
            .include_dir
            .join("qe6502")
            .join("qe6502_version.h"),
        &staged_native_dir
            .join("include")
            .join("qe6502")
            .join("qe6502_version.h"),
    )?;
    copy_file(
        &native_source
            .include_dir
            .join("qe6502")
            .join("qe6502_abi.h"),
        &staged_native_dir
            .join("include")
            .join("qe6502")
            .join("qe6502_abi.h"),
    )?;
    copy_dir_recursive(
        &native_source.src_dir.join("control_store"),
        &staged_native_dir.join("src").join("control_store"),
    )?;

    Ok(())
}

fn copy_file(from: &Path, to: &Path) -> io::Result<()> {
    if let Some(parent) = to.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::copy(from, to)?;
    Ok(())
}

fn copy_dir_recursive(from: &Path, to: &Path) -> io::Result<()> {
    fs::create_dir_all(to)?;

    for entry in fs::read_dir(from)? {
        let entry = entry?;
        let source_path = entry.path();
        let target_path = to.join(entry.file_name());
        let file_type = entry.file_type()?;

        if file_type.is_dir() {
            copy_dir_recursive(&source_path, &target_path)?;
        } else if file_type.is_file() {
            copy_file(&source_path, &target_path)?;
        }
    }

    Ok(())
}
