# nlpm - North Linux Package Manager

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)

`nlpm` is the primary package manager for the North Linux operating system, written in C11. It provides single-binary, daemon-less package management with a PubGrub SAT dependency solver, transactional installations, and Zstandard-compressed binary archives (`.nlpkg`).

---

## Features

- **SAT-Based Dependency Resolution**: Implements the PubGrub algorithm with Conflict-Driven Clause Learning (CDCL) and unit propagation, handling complex version constraints and diamond dependencies.
- **Transactional State Management**: SQLite-backed local state (`local.db`) with WAL mode and foreign key constraints ensuring atomic package installation and removal.
- **Fast Binary Archive Format**: Native `.nlpkg` format using POSIX pax-restricted tar with Zstandard compression.
- **Cryptographic Verification**: Built-in RFC-6234 SHA-256 validation for downloaded archives and package manifests.
- **Concurrent Networking**: High-throughput mirror synchronization and parallel downloads backed by libcurl multi-interface.
- **Zero-Daemon Architecture**: Operates directly on the filesystem and local databases with no background daemons or IPC overhead.
- **Chroot & Sysroot Support**: Native `-r <root_dir>` flag allowing package operations on alternate root directories for OS bootstrapping and container provisioning.

---

## Technical Specifications

| Component | Technology / Standard |
|---|---|
| **Language** | ISO C11 (`-std=c11`, `-Wall -Wextra -Werror -pedantic`) |
| **Build System** | CMake >= 3.20 with `CMakePresets.json` |
| **Package Format** | `.nlpkg` (tar archive compressed with Zstandard) |
| **Manifest Format** | TOML (`.PKGINFO` / `nlpkg.toml`) |
| **Database Engine** | SQLite 3 (`WAL` journal mode, `foreign_keys = ON`) |
| **Network Engine** | libcurl multi interface |
| **Archive Engine** | libarchive (pax restricted format, zstd filter) |
| **Solver** | PubGrub SAT solver with CDCL and unit propagation |
| **Unit Testing** | Unity Test Framework + CTest |

---

## Documentation Index

- [Architecture & Internals](docs/ARCHITECTURE.md): Module hierarchy, transactional lifecycle, and SQLite database schemas.
- [Package & Repository Specification](docs/PACKAGE_SPEC.md): Binary `.nlpkg` format, `.PKGINFO` manifest format, and `.nldb` repository index format.
- [PubGrub Solver Engine](docs/SOLVER.md): Mathematical principles, incompatibility terms, decision levels, and conflict resolution.
- [Configuration Reference](docs/CONFIGURATION.md): Complete guide to `/etc/nlpm/nlpm.toml` options and repository settings.

---

## Building from Source

### Dependencies

Ensure the following development libraries and utilities are available:
- C11 compatible compiler (GCC >= 10 or Clang >= 11)
- CMake >= 3.20
- Ninja or GNU Make
- `libarchive`
- `libcurl`
- `libsqlite3`
- `libzstd`

On Debian/Ubuntu systems:
```bash
sudo apt-get install build-essential cmake ninja-build \
    libarchive-dev libcurl4-openssl-dev libsqlite3-dev libzstd-dev
```

### Build via CMake Presets

`nlpm` includes predefined build presets in `CMakePresets.json`:

```bash
# Configure debug build
cmake --preset debug

# Compile binary and test suite
cmake --build --preset debug

# Execute unit and integration tests
ctest --preset debug --output-on-failure
```

To compile an optimized release binary:
```bash
cmake --preset release
cmake --build --preset release
```

The resulting executable is generated at:
```text
build/release/src/nlpm
```

---

## Command-Line Usage

```text
Usage: nlpm [options] <command> [arguments...]

Options:
  -c <config_path>  Use alternate configuration file (default: /etc/nlpm/nlpm.toml)
  -r <root_dir>     Set an alternate installation root (default: /)
  -h, --help        Show usage help
  -v, --version     Show version information
```

### Subcommands

#### 1. Synchronize Repository Databases (`sync`)
Downloads repository index archives (`<repo>.nldb`) from active mirrors and populates `/var/lib/nlpm/sync.db`.
```bash
sudo nlpm sync
```

#### 2. Install Packages (`install`)
Resolves dependencies via PubGrub, downloads packages into `/var/cache/nlpm/`, verifies SHA-256 sums, extracts files to the target root, and updates `/var/lib/nlpm/local.db`.
```bash
sudo nlpm install dash coreutils
```
Install into an alternate root directory (useful for system installation / chroot provisioning):
```bash
nlpm -r /mnt/target install base-system
```

#### 3. Remove Packages (`remove`)
Checks reverse dependencies to prevent broken installations, safely removes owned files, and updates `local.db`.
```bash
sudo nlpm remove dash
```

#### 4. Upgrade System Packages (`upgrade`)
Checks installed packages against the synchronized repository databases, builds a resolution plan, and upgrades all outdated packages transactionally.
```bash
sudo nlpm upgrade
```

#### 5. Search Repository (`search`)
Searches package names and descriptions in the synchronized database.
```bash
nlpm search shell
```

#### 6. Package Information (`info`)
Displays package metadata (version, architecture, dependencies, size, SHA-256) from `sync.db`, `local.db`, or directly from a local `.nlpkg` archive file.
```bash
nlpm info dash
nlpm info /path/to/dash-0.5.12-x86_64.nlpkg
```

#### 7. List Installed Packages (`list`)
Lists all packages currently installed in `local.db` along with their versions and install reasons.
```bash
nlpm list
```

#### 8. Query File Ownership (`owns`)
Determines which installed package owns a given file path.
```bash
nlpm owns /bin/dash
```

#### 9. Build Binary Package (`build`)
Builds a `.nlpkg` package from a staging directory containing an `nlpkg.toml` manifest.
```bash
nlpm build /path/to/pkg_staging out/dash-0.5.12-x86_64.nlpkg
```

#### 10. Generate Repository Index (`index`)
Scans a directory containing `.nlpkg` files and generates a compressed repository database index (`<repo>.nldb`).
```bash
nlpm index /srv/repo/core /srv/repo/core/core.nldb
```

---

## Filesystem Layout

```text
/etc/nlpm/
└── nlpm.toml            # Primary configuration file

/var/lib/nlpm/
├── local.db             # SQLite database of installed packages and file paths
└── sync.db              # SQLite database of synchronized repository metadata

/var/cache/nlpm/         # Downloaded .nlpkg package archives
```

---

## License

This project is licensed under the Apache License 2.0. See [LICENSE](LICENSE) for details.
