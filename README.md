# GOBLIN

[![Tests Linux](https://github.com/Nyjako/GOBLIN/actions/workflows/tests-linux.yml/badge.svg)](https://github.com/Nyjako/GOBLIN/actions/workflows/tests-linux.yml)
[![Tests Windows](https://github.com/Nyjako/GOBLIN/actions/workflows/tests-windows.yml/badge.svg)](https://github.com/Nyjako/GOBLIN/actions/workflows/tests-windows.yml)

A tiny header-only collection of helpers, utilities, and questionable ideas.

> **Not a framework. Not a standard library replacement. Just a stash of stuff.**

## What is this?

This repository is a personal-style collection of header-only helpers that may be useful in your projects or mine. Think of it as a compact grab bag of small utilities: the kind of things you keep rewriting, copy-pasting, or forgetting until you need them again.

The goal is simple:

- keep the code **small**,
- keep it **easy to include**,
- keep it **easy to ignore** if you do not need it,

## Philosophy

- **Header-only**: drop it in and go.
- **Low ceremony**: minimal setup, minimal dependencies.
- **Practical over perfect**: useful beats elegant when it matters.
- **Small helpers first**: no giant abstraction layers.
- **Portable by default**: write for boring, common toolchains when possible.

## Installation

Since everything is header-only, installation is just:

1. Copy the `include/` folder into your project, or
2. Add this repository as a submodule, or
3. Add the repo path to your compiler include directories.

## Headers

**Strings & text**

| Header | What it does |
| --- | --- |
| `cstr.h` | Small helpers for C strings (trim, case, search) |
| `ascii_transliterate.h` | Turns text into plain ASCII letters |
| `sanitize_filename.h` | Makes a string safe to use as a file name |

**Encoding & hashing**

| Header | What it does |
| --- | --- |
| `hex.h` | Converts bytes to and from hex text |
| `base64.h` | Encodes and decodes Base64 |
| `url.h` | Encodes and decodes URLs |
| `fnv1a.h` | Fast FNV-1a hash function |
| `md5.h` | Computes MD5 hashes |
| `secure_compare.h` | Compares secrets without leaking timing info |

**Files & system**

| Header | What it does |
| --- | --- |
| `fileio.h` | Helpers for files and paths (big files, line reading) |
| `appdirs.h` | Finds the app's data, config, and cache folders |
| `env.h` | Reads and writes environment variables |

**Concurrency & timing**

| Header | What it does |
| --- | --- |
| `thread.h` | Cross-platform threads, mutexes, and conditions |
| `par_for_each.h` | Runs a loop over an array using multiple threads |
| `rate_limit.h` | Limits how often something can run |
| `high_res_timer.h` | A high-resolution timer for measuring time |

**Other**

| Header | What it does |
| --- | --- |
| `dyn_array.h` | A growable array (like a simple vector) |
| `diagnostic.h` | Macros for TODOs, unreachable code, and fatal errors |
| `window.h` | Opens a window and draws to its pixels |

## License

This project is dual-licensed under:

- **[MIT License](LICENSE-MIT)**, and
- **[Public Domain](LICENSE-Unlicense)**

You may use either license at your option.

## Contributing

Suggestions, bug reports, and small improvements are welcome.

If you add something, keep the spirit of the project in mind:

- small
- useful
- header-only
- easy to read

## Disclaimer

These helpers are provided as-is. They may be tiny, convenient, and occasionally terrible. Use what helps, modify what does not, and do not expect magic.
