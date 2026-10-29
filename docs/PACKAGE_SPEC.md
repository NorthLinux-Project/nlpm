# Package & Repository Specifications

This document defines the binary archive format (`.nlpkg`), package manifest schema (`.PKGINFO`), and repository index format (`.nldb`) utilized by the North Linux packaging ecosystem.

---

## The `.nlpkg` Binary Package Format

An `.nlpkg` file is a POSIX pax-restricted `tar` archive compressed using the Zstandard (`zstd`) algorithm.

### Archive Layout

```text
archive.nlpkg (tar.zst)
├── .PKGINFO              # Package metadata in TOML format (must be first file)
├── usr/
│   ├── bin/
│   │   └── dash
│   └── share/
│       └── man/
│           └── man1/
│               └── dash.1
└── etc/
```

- **Metadata Header**: The `.PKGINFO` manifest must reside at the root of the archive. `nlpm` reads the first archive header to extract metadata without needing to decompress payload contents.
- **Payload Hierarchy**: All subsequent entries represent the filesystem hierarchy to be installed relative to the target sysroot (`/` or `-r <root_dir>`).
- **File Modes & Timestamps**: Preserves standard UNIX permissions (`0755`, `0644`), ownership attributes, and symbolic links.

---

## `.PKGINFO` Manifest Specification

The `.PKGINFO` file (and the maintainer-facing `nlpkg.toml`) uses the standard TOML syntax.

### Example Manifest

```toml
name = "dash"
version = "0.5.12"
base = "dash"
desc = "POSIX-compliant Unix shell designed for speed and small footprint"
url = "http://gondor.apana.org.au/~herbert/dash/"
arch = "x86_64"
license = ["BSD-3-Clause"]
isize = 243920
csize = 118400
sha256 = "16617694d6ff8ceaa265d95017b7adb9704a12970ca3b332840c26fc2e977fc7"
build_date = 1730198400
packager = "North Linux Build System <build@northlinux.org>"

[deps]
depend = ["glibc>=2.30"]
conflict = ["dash-git"]
provides = ["sh"]
```

### Manifest Fields Reference

| Field | Type | Required | Description |
|---|---|---|---|
| `name` | String | Yes | Unique identifier of the package. Lowercase alphanumeric and hyphens. |
| `version` | String | Yes | Version identifier conforming to North Linux SemVer format (e.g. `1.2.3`, `0.5.12-1`). |
| `base` | String | No | Source package base name (for split packages). |
| `desc` | String | Yes | Single-line description of the software. |
| `url` | String | No | Upstream project homepage or repository. |
| `arch` | String | Yes | Target CPU architecture (e.g. `x86_64`, `aarch64`). |
| `license` | Array of Strings | No | Software license identifiers (e.g. SPDX tags). |
| `isize` | Integer | Yes | Unpacked installation size in bytes. |
| `csize` | Integer | Yes | Compressed archive file size in bytes. |
| `sha256` | String | Yes | Lowercase hexadecimal SHA-256 hash of the `.nlpkg` archive. |
| `build_date`| Integer | No | UNIX epoch timestamp of package compilation. |
| `packager` | String | No | Maintainer or automated pipeline identity. |
| `deps.depend` | Array of Strings | No | List of required dependency constraints (e.g. `["glibc>=2.30", "zlib"]`). |
| `deps.conflict`| Array of Strings | No | Packages that cannot coexist on the system with this package. |
| `deps.provides`| Array of Strings | No | Virtual capabilities provided by this package. |

---

## Dependency Constraint Syntax

Dependencies specified in `deps.depend` support semantic version constraints:

| Operator | Meaning | Example |
|---|---|---|
| `>=` | Greater than or equal to version | `glibc>=2.34` |
| `<=` | Less than or equal to version | `python<=3.12` |
| `=` | Exactly equal to version | `llvm=17.0.6` |
| `>` | Strictly greater than version | `openssl>3.0.0` |
| `<` | Strictly less than version | `gcc<14.0.0` |
| `!=` | Not equal to version | `badpkg!=1.0.1` |
| *(none)* | Any version matches | `curl` |

---

## Repository Index Format (`<repo>.nldb`)

Repository indices aggregate package records into a downloadable index archive.

### Archive Structure

A repository database (such as `core.nldb`) is a `tar.zst` archive. It contains one directory per package, named `<name>-<version>`, inside of which resides a `desc.toml` file:

```text
core.nldb (tar.zst)
├── dash-0.5.12/
│   └── desc.toml
├── coreutils-9.4/
│   └── desc.toml
└── glibc-2.38/
    └── desc.toml
```

### `desc.toml` Schema

```toml
name = "dash"
version = "0.5.12"
desc = "POSIX-compliant Unix shell designed for speed and small footprint"
arch = "x86_64"
isize = 243920
csize = 118400
sha256 = "16617694d6ff8ceaa265d95017b7adb9704a12970ca3b332840c26fc2e977fc7"
filename = "dash-0.5.12-x86_64.nlpkg"

[deps]
depend = ["glibc>=2.30"]
```

### Generation and Synchronization
1. **Creation (`nlpm index`)**: Maintains repository state by scanning `.nlpkg` files, parsing their metadata, and writing the compressed `<repo>.nldb` archive.
2. **Synchronization (`nlpm sync`)**: The client downloads `<repo>.nldb`, uncompresses entries in memory, and writes them into `/var/lib/nlpm/sync.db` within a single SQLite transaction.
