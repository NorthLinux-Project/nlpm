# Configuration Reference

`nlpm` is configured via a single TOML configuration file located at `/etc/nlpm/nlpm.toml`. This document details all available configuration directives and repository settings.

---

## Configuration Location

By default, `nlpm` loads configuration from:
```text
/etc/nlpm/nlpm.toml
```

An alternate configuration file may be supplied via the command-line flag `-c`:
```bash
nlpm -c /path/to/custom-nlpm.toml <command>
```

---

## File Structure

The configuration file is divided into a `[general]` table and one or more `[[repo]]` array tables.

```toml
[general]
rootdir = "/"
cachedir = "/var/cache/nlpm"
dbdir = "/var/lib/nlpm"
parallel = 4

[[repo]]
name = "core"
url = "https://pkg.northlinux.org/core"
enabled = true

[[repo]]
name = "extra"
url = "https://pkg.northlinux.org/extra"
enabled = true
```

---

## Directive Reference

### `[general]` Section

Global settings affecting filesystem paths and concurrency.

| Directive | Type | Default | Description |
|---|---|---|---|
| `rootdir` | String | `"/"` | The root installation directory where packages and files are unpacked. Can be overridden on the command line using `-r <dir>`. |
| `cachedir`| String | `"/var/cache/nlpm"` | Directory used to store downloaded `.nlpkg` package archives and temporary download buffers. |
| `dbdir` | String | `"/var/lib/nlpm"` | Directory containing the SQLite database files (`local.db` and `sync.db`). |
| `parallel`| Integer| `4` | Maximum number of concurrent network connections utilized during parallel package downloads. |

---

### `[[repo]]` Section

Defines remote package repositories. Multiple repositories can be declared using TOML array-of-tables syntax (`[[repo]]`).

| Directive | Type | Default | Description |
|---|---|---|---|
| `name` | String | *(Required)* | Short identifier for the repository (e.g. `"core"`, `"extra"`). This name matches the database index filename (e.g. `core.nldb`). |
| `url` | String | *(Required)* | Base HTTP or HTTPS URL where the repository index (`<name>.nldb`) and package archives are hosted. |
| `enabled` | Boolean | `true` | Toggles whether this repository is synchronized and queried during package searches and installations. |

---

## Common Configuration Scenarios

### 1. Isolated Build or Chroot Sysroot
When creating a chroot environment or building an image in `/mnt/northlinux`, configure `rootdir`:
```toml
[general]
rootdir = "/mnt/northlinux"
cachedir = "/mnt/northlinux/var/cache/nlpm"
dbdir = "/mnt/northlinux/var/lib/nlpm"
parallel = 8

[[repo]]
name = "core"
url = "https://pkg.northlinux.org/core"
enabled = true
```

### 2. Local File Mirror
`nlpm` supports local filesystem paths using `file://` URLs for offline installs or test repositories:
```toml
[general]
rootdir = "/"
cachedir = "/var/cache/nlpm"
dbdir = "/var/lib/nlpm"

[[repo]]
name = "local"
url = "file:///srv/repo/core"
enabled = true
```
