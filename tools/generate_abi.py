#!/usr/bin/env python3
"""Generate declarations only; no calculator implementation or build workflow."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "contracts/calculator-abi-v1.json"
TYPES = {
    "u32": ("uint32_t", "u32"),
    "size": ("size_t", "usize"),
    "const_u8_ptr": ("const uint8_t *", "*const u8"),
    "f64_ptr": ("double *", "*mut f64"),
    "char_ptr": ("char *", "*mut core::ffi::c_char"),
    "size_ptr": ("size_t *", "*mut usize"),
}


def render():
    raw = SOURCE.read_bytes()
    spec = json.loads(raw)
    if spec["schema_version"] != 1:
        raise ValueError("unsupported schema_version")
    if spec["abi_version"] != spec["constants"]["CALC_ABI_VERSION"]:
        raise ValueError("abi_version differs from CALC_ABI_VERSION")
    digest = hashlib.sha256(raw).hexdigest()
    notice = f"Generated from contracts/calculator-abi-v1.json; SHA-256 {digest}. DO NOT EDIT."
    c = [f"/* {notice} */", "#ifndef CALCULATOR_ABI_V1_H", "#define CALCULATOR_ABI_V1_H",
         "#include <stddef.h>", "#include <stdint.h>", "#ifdef __cplusplus", 'extern "C" {', "#endif"]
    rust = [f"// {notice}"]
    checks = [f"// {notice}", "// Include at crate root; producer exports live in crate::ffi."]
    for name, value in spec["constants"].items():
        if value == "size_max":
            c.append(f"#define {name} SIZE_MAX")
            rust.append(f"pub const {name}: usize = usize::MAX;")
        else:
            if type(value) is not int or not 0 <= value <= 0xFFFFFFFF:
                raise ValueError(f"invalid u32 constant: {name}")
            c.append(f"#define {name} UINT32_C({value})")
            rust.append(f"pub const {name}: u32 = {value};")
    for fn in spec["functions"]:
        name, ret, params = fn["name"], fn["return"], fn["params"]
        cp = ", ".join(f"{TYPES[t][0]} {n}" for n, t in params) or "void"
        rp = ", ".join(TYPES[t][1] for _, t in params)
        alias = "".join(word.title() for word in name.split("_")) + "Fn"
        c.append(f"{TYPES[ret][0]} {name}({cp});")
        rust.append(f'pub type {alias} = unsafe extern "C" fn({rp}) -> {TYPES[ret][1]};')
        checks.append(f"const _: crate::abi_generated::{alias} = crate::ffi::{name};")
    c.extend(["#ifdef __cplusplus", "}", "#endif", "#endif"])
    return {"include/calculator_abi_v1.h": "\n".join(c) + "\n",
            "contracts/generated/abi_generated.rs": "\n".join(rust) + "\n",
            "contracts/generated/producer_checks.rs": "\n".join(checks) + "\n"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    stale = []
    for name, content in render().items():
        path = ROOT / name
        if args.check:
            if not path.exists() or path.read_text() != content:
                stale.append(name)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content)
    if stale:
        parser.exit(1, "Stale ABI artifacts: " + ", ".join(stale) + "\n")
    print("ABI generated artifacts match" if args.check else "ABI artifacts generated")


if __name__ == "__main__":
    main()
