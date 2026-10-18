# Optional Linux filesystem allowlists

CPPUI follows PUI `e3b5d16` with a native `LandlockPolicy`, containing `read`,
`write` and `execute` vectors of existing file/directory paths. No Python binding
or AI supervisor is involved. Normal `run()` and `start()` remain unrestricted.

```cpp
CPPUI::LandlockPolicy policy;
policy.read = {"/absolute/application", "/absolute/input"};
policy.write = {"/absolute/output"};
app.run(policy);
```

This illustrates the syntax, not a complete GUI runtime allowlist. Include the
libraries, fonts, themes, configuration and devices the application needs after
startup. CPPUI does not automatically allow system/runtime directories.

`run(policy)` builds the UI and applies the policy before entering the event
loop. `start(policy)` applies it to an already mounted application. Native
initialization can create background threads; the policy then refuses to apply.
For that case, call `CPPUI::applyLandlock(policy)` before creating the toolkit
runtime, with all required runtime paths included.

- `read` permits file reads and directory listing.
- `write` also permits writes, creation/removal, links/renames, truncation and
  device operations covered by the kernel; it excludes execution.
- `execute` permits execution; add read access separately where needed.

Paths are resolved before applying the policy; directory rules include their
hierarchy. Symlinks inside an allowed directory do not grant access outside it.
Rules are additive; an empty policy denies all handled filesystem operations.
Linux requires Landlock ABI 3 or newer, including truncate protection, and
Landlock headers/syscall definitions at compile time. ABI 5+ also restricts
device ioctls. Invalid paths, missing support and failed installation throw
before the event loop; they never silently continue unrestricted on Linux.
Other platforms warn and ignore the option, following PUI.

Restrictions are irreversible for the calling thread and inherited by future
threads/processes. Existing threads are not retroactively restricted, so CPPUI
requires a single thread at application time. Existing descriptors/memory stay
accessible; this does not restrict all filesystem metadata, network activity or
resource usage. These are filesystem allowlists, not a complete hostile-code
sandbox. The regression uses disposable child processes to verify read/write,
truncate, rename, directory escape, empty policies, inherited restrictions and
rejection of preexisting threads.
