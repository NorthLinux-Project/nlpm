# AGENTS.md - Repository Guidelines for AI Assistants

This file defines the mandatory engineering practices, architecture principles, command invariants, and operational boundaries for AI coding agents working on the `nlpm` codebase.

---

## 1. Project Overview & Architecture

`nlpm` (North Linux Package Manager) is the official package manager for North Linux, written in pure ISO C11. It is a single-binary, daemon-less tool designed for high reliability, deterministic dependency resolution, and fast transactional execution.

### Architectural Tenets
- **Zero Daemon**: All operations run directly in user space using filesystem APIs and SQLite transactional semantics. No background services, daemons, or IPC sockets.
- **Deterministic SAT Solving**: Implements the PubGrub algorithm with Conflict-Driven Clause Learning (CDCL) and unit propagation. Resolution plans are computed and validated before touching disk or network.
- **Fail-Safe Transactions**: Package installations and removals maintain strict atomicity. File ownership collisions are verified prior to extraction, and database state updates run inside SQLite transactions.
- **Minimal External Surface**: Relies only on mature, standard system C libraries (`libarchive`, `libcurl`, `sqlite3`, `libzstd`).

---

## 2. Repository Layout

```text
nlpm/
├── CMakeLists.txt              # Root build configuration
├── CMakePresets.json           # Canonical build presets (debug, release)
├── LICENSE                     # Apache License 2.0
├── README.md                   # Primary user & contributor entry point
├── AGENTS.md                   # Agent guidelines and standards
├── cmake/
│   ├── CompilerWarnings.cmake  # Strict compiler warning interface (-Wall -Wextra -Werror)
│   └── Dependencies.cmake      # External library detection and unity linkage
├── docs/
│   ├── ARCHITECTURE.md         # Detailed subsystem architecture and SQLite schemas
│   ├── CONFIGURATION.md        # /etc/nlpm/nlpm.toml reference guide
│   ├── PACKAGE_SPEC.md         # .nlpkg, .PKGINFO, and .nldb specifications
│   └── SOLVER.md               # PubGrub SAT solving engine internals
├── src/
│   ├── main.c                  # CLI entry point, argument parsing, privilege checks
│   ├── cmd/                    # CLI subcommand handlers (sync, install, remove, etc.)
│   ├── config/                 # /etc/nlpm/nlpm.toml configuration parser
│   ├── crypto/                 # RFC-6234 compliant SHA-256 implementation
│   ├── db/                     # SQLite database interfaces (local.db, sync.db)
│   ├── net/                    # libcurl multi-interface concurrent downloader
│   ├── pkg/                    # .nlpkg archive reader, builder, and installer
│   ├── repo/                   # Repository index generator and synchronization
│   ├── solver/                 # PubGrub SAT solver, terms, and incompatibilities
│   └── util/                   # Collections (stretchy buffer, hash map), version parser
├── tests/
│   ├── CMakeLists.txt          # Test target definitions
│   ├── integration/            # Multi-component end-to-end pipeline tests
│   └── unit/                   # Isolated subsystem unit tests
└── third_party/
    ├── include/                # System library header shims (archive, curl, sqlite3)
    ├── toml-c/                 # Vendored DOM TOML parser
    └── unity/                  # Vendored Unity Test Framework
```

---

## 3. Command Invariants

Agents must maintain strict command stability across the entire development lifecycle. Never mutate flags, rename preset targets, or substitute alternative build systems.

### Configuration
```bash
cmake --preset debug
```

### Compilation
```bash
# Debug build (with compiler warnings and debug symbols)
cmake --build --preset debug

# Release build (optimized)
cmake --build --preset release
```

### Testing
```bash
# Execute all test suites
ctest --preset debug --output-on-failure
```

---

## 4. C11 Language & Compiler Standards

- **Language Standard**: ISO C11 (`-std=c11`). Do not use compiler-specific GNU extensions unless protected by standard feature-test macros.
- **POSIX Conformance**: Always place `#define _POSIX_C_SOURCE 200809L` at the very top of `.c` implementation files before including standard library headers.
- **Zero Warnings**: Code must compile cleanly under `-Wall -Wextra -Werror -pedantic`. Any compiler warning is treated as a fatal build failure.
- **Return Value Safety**: Always inspect return values for system and POSIX calls (`mkdir`, `symlink`, `fwrite`, `strdup`, `sqlite3_*`, `archive_*`). Functions marked with `warn_unused_result` must never be ignored.
- **Buffer Safety**:
  - Unbounded functions (`sprintf`, `strcpy`, `strcat`, `gets`) are strictly forbidden.
  - Use `snprintf` with explicit destination buffer sizes.
  - Size path buffers adequately (e.g. at least 2048 bytes for concatenated directory/file paths) to prevent `-Wformat-truncation` warnings.

---

## 5. Memory Management & Safety

- **Deterministic Lifecycles**: Every heap allocation (`malloc`, `calloc`, `strdup`, `stb_sb_push`) must have a corresponding, well-defined free point.
- **Destructor Idiom**: Every module or composite struct that owns allocated memory must provide a destructor function (e.g. `config_free`, `solver_destroy`, `pkg_meta_free`).
- **SQLite Resource Management**:
  - Always finalize prepared statements with `sqlite3_finalize(stmt)` on both success and error paths.
  - Always rollback uncommitted transactions (`sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL)`) when encountering operational failures.
- **Dynamic Collections**: Use the stretchy buffer macro interface in `src/util/stretchy_buffer.h` for growable arrays, and ensure `sb_free` is called when destroying the container.

---

## 6. Dependency & Library Boundaries

- **Strict Dependency Lock**: Do not introduce any new external runtime dependencies. The core binary is restricted to:
  - `libarchive` (tar/pax archive streaming and extraction)
  - `libcurl` (HTTP/HTTPS downloads and multi-transfer concurrency)
  - `libsqlite3` (database transactions and index queries)
  - `libzstd` (high-performance compression/decompression)
- **Vendored Code**: Third-party code is strictly confined to `third_party/`. Any new internal helper must be pure C, single-header or paired `.h`/`.c`, vetted for license compatibility, and free of external system requirements.

---

## 7. Testing Requirements

- **Mandatory CTest Verification**: Every new feature, bug fix, or refactored module must be accompanied by unit tests in `tests/unit/` or integration tests in `tests/integration/`.
- **100% Pass Rate**: All test targets registered in `tests/CMakeLists.txt` must pass with zero failures before work is marked complete:
  ```bash
  ctest --preset debug --output-on-failure
  ```
- **Hermetic Tests**: Tests must create and clean up their own temporary test directories (`/tmp/test_*`) and never mutate the host system or default database paths.

---

## 8. Git & Staging Protocol

- **No Unsolicited Commits**: Agents must never execute `git add`, `git commit`, `git push`, or alter Git commit history without explicit user instruction.
- **Commit Message Format**:
  - Header: Concise, single-line imperative header (e.g. `Add reverse dependency check before package removal`).
  - Do not use conventional commit prefixes (avoid `feat:`, `fix:`, `chore:`).
  - Commit Body: The commit body is optional. Always ask the user for their preference before committing (whether they prefer a single-line commit or an explanatory body).
