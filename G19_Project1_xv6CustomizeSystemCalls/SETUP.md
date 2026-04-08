# Setting up xv6-public on Ubuntu / WSL2

xv6 is the MIT teaching operating system. We use the x86 (32-bit) version
known as **xv6-public** because it has the simplest build path on Linux.

## 1. Install the tool-chain

On Ubuntu 22.04 / 24.04 (or WSL2 Ubuntu):

```bash
sudo apt update
sudo apt install -y git build-essential gcc-multilib qemu-system-x86 \
                    qemu-system-i386 gdb-multiarch
```

If `qemu-system-i386` is not available on your distro, `qemu-system-x86`
provides it as a symlink.

## 2. Clone xv6-public

```bash
cd ~
git clone https://github.com/mit-pdos/xv6-public.git
cd xv6-public
```

## 3. Verify the unmodified build runs

```bash
make
make qemu-nox          # use qemu-nox so QEMU runs in your terminal
```

You should see the xv6 banner and a `$` prompt. Press **Ctrl-A** then
**X** to exit QEMU.

If `make qemu-nox` complains about missing `qemu-system-i386`, edit the
xv6 `Makefile` and replace the line

```
QEMU = qemu-system-i386
```

with the full path of your QEMU binary (find it with `which qemu-system-i386`).

## 4. Apply our system-call changes

Follow `INSTRUCTIONS.md` step-by-step. After each step you should be
able to run `make qemu-nox` again and see the system still boots.

## 5. Run the test program

Inside the xv6 shell:

```
$ syscalltest
```

Take a screenshot of the output for `docs/screenshots/`.

To exit QEMU: **Ctrl-A** then **X**.
