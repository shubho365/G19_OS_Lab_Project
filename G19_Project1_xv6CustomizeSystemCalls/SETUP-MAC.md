# Setting up xv6 on macOS (Apple Silicon — M1 / M2 / M3)

The original `SETUP.md` is written for Ubuntu / WSL. On an Apple Silicon
Mac it **won't work as-is** because:

- macOS has no `apt`, so `sudo apt install …` fails.
- `gcc-multilib` only exists on Linux — the bundled Apple Clang on
  macOS cannot produce 32-bit ELF binaries at all.
- Your M3 is ARM64; xv6-public targets 32-bit x86. You need a
  **cross-compiler** (`i386-elf-gcc`) that runs on ARM64 but emits
  i386 code, plus a QEMU build that can emulate i386.

You have two clean ways to handle this. Pick one.

---

## Option A — Docker (recommended, most reliable)

This is the fastest and least error-prone path. You run an Ubuntu
container that has the exact same tool-chain as the Linux guide, so
`INSTRUCTIONS.md` applies word-for-word with zero modifications.

### A.1 Install Docker Desktop

Download from <https://www.docker.com/products/docker-desktop/> and
pick the **Apple Silicon** build. Launch it once and wait until the
whale icon in the menu bar says "Docker Desktop is running".

Verify:

```bash
docker --version
```

### A.2 Clone xv6 on your Mac (not inside the container)

```bash
cd ~
git clone https://github.com/mit-pdos/xv6-public.git
cd xv6-public
```

Keeping the source on the Mac side means you can edit files with any
Mac editor (VS Code, nvim, etc.) and the container sees the changes
instantly through a bind mount.

### A.3 Start an Ubuntu container with the xv6 folder mounted

From inside the `xv6-public` directory:

```bash
docker run -it --rm \
    --platform linux/amd64 \
    -v "$PWD":/xv6 \
    -w /xv6 \
    ubuntu:22.04 bash
```

Notes on each flag:

- `-it` — interactive terminal.
- `--rm` — delete the container when you exit (your files stay safe
  because they're on the host).
- `--platform linux/amd64` — forces x86_64 emulation via Rosetta/QEMU.
  This is critical because xv6 builds 32-bit x86 binaries and the
  Ubuntu `gcc-multilib` package only exists for amd64.
- `-v "$PWD":/xv6` — bind-mounts your current folder into `/xv6`
  inside the container.
- `-w /xv6` — makes `/xv6` the container's starting directory.

You're now sitting at a `root@…:/xv6#` prompt inside Ubuntu.

### A.4 Install the tool-chain (inside the container)

```bash
apt update
apt install -y build-essential gcc-multilib qemu-system-x86 \
               qemu-system-i386 gdb make git
```

### A.5 Build and run

```bash
make clean
make qemu-nox
```

xv6 boots and drops you into its `$` prompt. Exit with **Ctrl-A**
then **X**.

### A.6 Day-to-day workflow

- Edit files on the **Mac side** with VS Code.
- Build and run in the **container**.
- The `--rm` flag deletes the container each time you exit, but your
  source tree and builds survive because they live on the Mac. Just
  re-run the `docker run …` command to get a fresh shell.

If you don't want to reinstall the tool-chain every time you restart
the container, drop `--rm` and give it a name:

```bash
docker run -it --name xv6dev --platform linux/amd64 \
    -v "$PWD":/xv6 -w /xv6 ubuntu:22.04 bash
# install tool-chain once, then exit

# subsequent sessions:
docker start -ai xv6dev
```

**Heads-up:** x86_64 emulation under Rosetta is slower than a native
Linux box. A full `make clean && make` takes roughly 30–60 seconds on
an M3, which is fine for this project.

---

## Option B — Native Homebrew cross-compiler

Runs natively on your M3 (faster than Docker) but requires installing
a third-party tap and editing the xv6 `Makefile`. Use this if you
dislike Docker or want faster iteration.

### B.1 Install Homebrew if you don't already have it

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

After it finishes, make sure `/opt/homebrew/bin` is on your PATH:

```bash
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zshrc
source ~/.zshrc
```

### B.2 Install QEMU

```bash
brew install qemu
```

This provides `qemu-system-i386` (the 32-bit x86 emulator you need).

### B.3 Install the i386 cross tool-chain

Homebrew dropped its official i386 cross-compiler formulas years ago.
The well-known community tap `nativeos/i386-elf-toolchain` provides
them:

```bash
brew tap nativeos/i386-elf-toolchain
brew install nativeos/i386-elf-toolchain/i386-elf-binutils
brew install nativeos/i386-elf-toolchain/i386-elf-gcc
brew install i386-elf-gdb        # optional, for GDB debugging
```

After installing, verify:

```bash
i386-elf-gcc --version
i386-elf-ld --version
qemu-system-i386 --version
```

All three should print version info.

### B.4 Clone xv6

```bash
cd ~
git clone https://github.com/mit-pdos/xv6-public.git
cd xv6-public
```

### B.5 Tell xv6 to use the cross-compiler

Open the xv6 `Makefile` and find the block near the top:

```make
# If the makefile can't find QEMU, specify its path here
# QEMU =

...

TOOLPREFIX = $(shell if i386-jos-elf-objdump ...)
```

Replace the `TOOLPREFIX` auto-detection line with a hard-coded prefix:

```make
TOOLPREFIX = i386-elf-
```

Then set the QEMU variable:

```make
QEMU = qemu-system-i386
```

### B.6 Fix two small toolchain incompatibilities

Modern `i386-elf-gcc` is stricter than the compiler xv6 was written
for. You'll hit two warnings-as-errors. Both have one-line fixes.

**Fix 1 — disable the array-bounds warning.** In the `Makefile`, find
the `CFLAGS =` line and append `-Wno-array-bounds -Wno-stringop-overflow`:

```make
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O2 \
         -MD -ggdb -m32 -Werror -fno-omit-frame-pointer \
         -Wno-array-bounds -Wno-stringop-overflow
```

**Fix 2 — `sign.pl` buffer size.** xv6 ships a small Perl script that
signs the boot block. On newer Perls it may warn about the read
buffer. If you hit an error there, open `sign.pl` and change the line
that reads:

```perl
if(!(-s "$ARGV[0]" <= 510)){
```

It's already correct on current xv6-public, but if your build
complains about "boot block too large", it means you've added too
much code to the boot sector and need to remove something — not a
Mac-specific issue.

### B.7 Build and run

```bash
make clean
make qemu-nox
```

Exit QEMU with **Ctrl-A** then **X**.

---

## Which should you pick?

| | **Docker** | **Native Homebrew** |
|---|---|---|
| Setup time | 5 min | 15–20 min |
| Matches `INSTRUCTIONS.md` exactly | Yes | Yes, after Makefile edits |
| Build speed | Slower (~30–60 s via Rosetta) | Faster (native ARM) |
| Likelihood of weird errors | Very low | Moderate (toolchain drift) |
| Disk space | ~1 GB for the image | ~500 MB |
| Works offline after setup | Yes | Yes |

**My recommendation: start with Docker.** It isolates the messy 32-bit
x86 tool-chain from your host OS, matches the Linux instructions
verbatim, and you can throw it away when the course is done. If you
find the build too slow, switch to the native path later — your xv6
source tree is the same either way.

---

## A note on xv6-riscv

You may have seen tutorials mentioning `xv6-riscv`, MIT's newer
64-bit RISC-V port of xv6. It's easier to install on Apple Silicon
(no 32-bit cross-compiler drama), but **the file layout and syscall
internals are different** from xv6-public, so our `INSTRUCTIONS.md`
would not apply directly. Stick with xv6-public for this project
unless your instructor explicitly says otherwise.

---

## Troubleshooting

**"qemu-system-i386: command not found"** — On newer QEMU packages the
binary is named `qemu-system-i386` on Linux but `qemu-system-i386`
(same name) on macOS. Check with `which qemu-system-i386`. If it's
missing, `brew install qemu` again.

**Docker says "no matching manifest for linux/arm64/v8"** — You
forgot `--platform linux/amd64`. Re-run the `docker run …` command
with that flag.

**`make` fails with "cannot find -lgcc"** — You're mixing a native Mac
compiler with 32-bit flags. In Option B, make sure `TOOLPREFIX =
i386-elf-` is set; in Option A, make sure you installed
`gcc-multilib` inside the container, not on the host.

**Infinite reboot loop in QEMU** — Almost always means a broken boot
sector. Run `make clean && make` and try again. If it persists,
you've modified code in `bootasm.S`, `bootmain.c`, or the linker
scripts — revert those.

**`argint: cannot find syscall 22`** — You added the `SYS_*` number
in `syscall.h` but forgot to add the entry in the dispatch table in
`syscall.c`. See Step 2 of `INSTRUCTIONS.md`.
