use qe6502::{Cpu, Model};
use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::time::Instant;

const STANDARD_ROM: &str = "6502_functional_test.hex";
const EXTENDED_ROM: &str = "65C02_extended_opcodes_test.hex";

const STANDARD_SUCCESS_ADDRESS: u16 = 0x3469;
const STANDARD_EXPECTED_CYCLES: u64 = 30_646_176;
const EXTENDED_SUCCESS_ADDRESS: u16 = 0x24F1;
const EXTENDED_EXPECTED_CYCLES: u64 = 21_986_985;

const START_ADDRESS: u16 = 0x0400;
const MEMORY_SIZE: usize = 0x10000;

const DEFAULT_SUITE: &[(&str, &str)] = &[
    ("nmos", "standard"),
    ("wdc", "standard"),
    ("wdc", "extended"),
    ("rw", "standard"),
    ("rw", "extended"),
    ("st", "standard"),
];

#[derive(Clone, Copy)]
struct ModelInfo {
    model: Model,
    display_name: &'static str,
}

struct TestImage {
    image: Vec<u8>,
    success_address: u16,
    expected_cycles: u64,
}

struct RomSet {
    standard: Vec<u8>,
    extended: Vec<u8>,
}

struct RunResult {
    passed: bool,
    message: &'static str,
    mhz: f64,
    bus_ticks: u64,
    opcode_cycles: u64,
}

fn main() {
    if let Err(err) = run(env::args().collect()) {
        eprintln!("qe6502 Rust Klaus2m5: FAIL");
        eprintln!("{err}");
        std::process::exit(1);
    }
}

fn run(args: Vec<String>) -> Result<(), String> {
    let (rom_dir, single_test) = parse_command_line(&args)?;
    let roms = load_roms(&rom_dir)?;
    let mut failures = 0u32;

    if let Some((model, test)) = single_test {
        if !run_one(&roms, &model, &test)? {
            failures += 1;
        }
    } else {
        for (model, test) in DEFAULT_SUITE {
            if !run_one(&roms, model, test)? {
                failures += 1;
            }
        }
    }

    if failures == 0 {
        Ok(())
    } else {
        Err(format!("{failures} Klaus2m5 Rust test(s) failed"))
    }
}

fn parse_command_line(args: &[String]) -> Result<(PathBuf, Option<(String, String)>), String> {
    let default_rom_dir = current_exe_dir()?.join("klaus2m5");

    match args.len() {
        1 => Ok((default_rom_dir, None)),
        2 => Ok((PathBuf::from(&args[1]), None)),
        3 => Ok((default_rom_dir, Some((args[1].clone(), args[2].clone())))),
        4 => Ok((PathBuf::from(&args[1]), Some((args[2].clone(), args[3].clone())))),
        _ => Err(usage(args.first().map_or("qe6502_rust_klaus2m5", String::as_str))),
    }
}

fn usage(script_name: &str) -> String {
    format!(
        "Usage: {script_name} [<rom-dir>] [<model> <test>]\n\
         {script_name} <model> <test>\n\n\
         With no model/test arguments, runs the default v2 Klaus suite.\n\
         With no rom-dir, uses ./klaus2m5 next to the executable.\n\n\
         Models: nmos, wdc, rw, st\n\
         Tests: standard, extended"
    )
}

fn current_exe_dir() -> Result<PathBuf, String> {
    let exe = env::current_exe().map_err(|err| format!("current_exe failed: {err}"))?;
    exe.parent()
        .map(Path::to_path_buf)
        .ok_or_else(|| "current executable has no parent directory".to_string())
}

fn load_roms(rom_dir: &Path) -> Result<RomSet, String> {
    if !rom_dir.is_dir() {
        return Err(format!("Klaus ROM directory not found: {}", rom_dir.display()));
    }

    Ok(RomSet {
        standard: load_hex_rom(&rom_dir.join(STANDARD_ROM))?,
        extended: load_hex_rom(&rom_dir.join(EXTENDED_ROM))?,
    })
}

fn load_hex_rom(path: &Path) -> Result<Vec<u8>, String> {
    let text = fs::read_to_string(path)
        .map_err(|err| format!("failed to read {}: {err}", path.display()))?;
    let bytes = parse_hex_bytes(&text)?;

    if bytes.len() != MEMORY_SIZE {
        return Err(format!(
            "{} contains {} bytes; expected {}",
            path.display(),
            bytes.len(),
            MEMORY_SIZE
        ));
    }

    Ok(bytes)
}

fn parse_hex_bytes(text: &str) -> Result<Vec<u8>, String> {
    let mut out = Vec::with_capacity(MEMORY_SIZE);
    let bytes = text.as_bytes();
    let mut index = 0usize;

    while index + 3 < bytes.len() {
        if bytes[index] == b'0' && (bytes[index + 1] == b'x' || bytes[index + 1] == b'X') {
            let high = hex_digit(bytes[index + 2]);
            let low = hex_digit(bytes[index + 3]);
            if let (Some(high), Some(low)) = (high, low) {
                out.push((high << 4) | low);
                index += 4;
                continue;
            }
        }
        index += 1;
    }

    Ok(out)
}

fn hex_digit(byte: u8) -> Option<u8> {
    match byte {
        b'0'..=b'9' => Some(byte - b'0'),
        b'a'..=b'f' => Some(byte - b'a' + 10),
        b'A'..=b'F' => Some(byte - b'A' + 10),
        _ => None,
    }
}

fn run_one(roms: &RomSet, model_name: &str, test_name: &str) -> Result<bool, String> {
    let model = parse_model(model_name)?;
    let test_image = select_test_image(roms, test_name, model)?;
    let result = run_klaus_test(model.model, test_image)?;

    println!(
        "{} CPU {} Rust test {} {} ({:.2} MHz, {} bus ticks, {} opcode cycles)",
        model.display_name,
        test_name,
        if result.passed { "[PASS]" } else { "[FAIL]" },
        result.message,
        result.mhz,
        result.bus_ticks,
        result.opcode_cycles
    );

    Ok(result.passed)
}

fn parse_model(model_name: &str) -> Result<ModelInfo, String> {
    match model_name {
        "nmos" | "mos" => Ok(ModelInfo {
            model: Model::Nmos,
            display_name: "NMOS 6502",
        }),
        "wdc" => Ok(ModelInfo {
            model: Model::Wdc,
            display_name: "WDC 65C02",
        }),
        "rw" | "rockwell" => Ok(ModelInfo {
            model: Model::Rw,
            display_name: "Rockwell 65C02",
        }),
        "st" | "synertek" => Ok(ModelInfo {
            model: Model::St,
            display_name: "Synertek 65C02",
        }),
        _ => Err(format!("Unknown model: {model_name}")),
    }
}

fn select_test_image(roms: &RomSet, test_name: &str, model: ModelInfo) -> Result<TestImage, String> {
    match test_name {
        "standard" => Ok(TestImage {
            image: roms.standard.clone(),
            success_address: STANDARD_SUCCESS_ADDRESS,
            expected_cycles: STANDARD_EXPECTED_CYCLES,
        }),
        "extended" => {
            if model.model != Model::Wdc && model.model != Model::Rw {
                return Err(format!(
                    "Extended test is only valid for WDC/Rockwell 65C02 v2 models, not {}.",
                    model.display_name
                ));
            }
            Ok(TestImage {
                image: roms.extended.clone(),
                success_address: EXTENDED_SUCCESS_ADDRESS,
                expected_cycles: EXTENDED_EXPECTED_CYCLES,
            })
        }
        _ => Err(format!("Unknown test: {test_name}")),
    }
}

fn run_klaus_test(model: Model, test_image: TestImage) -> Result<RunResult, String> {
    let mut memory = test_image.image;
    let mut cpu = Cpu::new(model);
    let mut opcode_cycles = 0u64;
    let mut bus_ticks = 0u64;

    cpu.restart();
    cpu.jump_to(START_ADDRESS);
    let start = Instant::now();

    let (passed, message) = loop {
        let address = cpu.bus_address() as usize;
        let data = if cpu.is_write() {
            cpu.bus_data()
        } else {
            memory[address]
        };

        if address == test_image.success_address as usize {
            break (true, "OK");
        }

        if cpu.is_write() {
            memory[address] = data;
        }

        cpu.tick(data);
        bus_ticks += 1;

        if cpu.is_opcode_fetch() {
            opcode_cycles += 1;
            if opcode_cycles > 2 * test_image.expected_cycles {
                break (false, "Test fail, takes too many cycles!");
            }
        }
    };

    let seconds = start.elapsed().as_secs_f64();
    Ok(RunResult {
        passed,
        message,
        mhz: emulated_mhz(bus_ticks, seconds),
        bus_ticks,
        opcode_cycles,
    })
}

fn emulated_mhz(bus_ticks: u64, seconds: f64) -> f64 {
    if seconds <= 0.0 {
        0.0
    } else {
        bus_ticks as f64 / seconds / 1_000_000.0
    }
}

