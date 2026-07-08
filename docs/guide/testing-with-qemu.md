# Manual smoke test against a real QEMU guest

Irida's transport is unit-tested against an in-process mock gdb server, so QEMU
is **not** required to build or test the project. This document describes an
optional end-to-end check once you have QEMU installed.

## Prerequisites
- QEMU for Windows (`qemu-system-x86_64`) on PATH.
- Any small bootable image (a tiny Linux ISO/qcow2 is easiest).

## Start QEMU with the gdb stub, frozen at startup
```bat
qemu-system-x86_64 -accel tcg -s -S -hda guest.img -m 2G -nographic
```
- `-s` opens the gdb stub on `tcp::1234`.
- `-S` freezes the CPU so nothing runs until a debugger continues it.

## Drive it with a gdb-remote client
Until Irida grows an attach UI, verify the stub with stock `gdb`:
```
gdb
  (gdb) target remote localhost:1234
  (gdb) info registers      # exercises the 'g' packet
  (gdb) x/8xb 0xffffffff...  # exercises 'm'
  (gdb) hbreak *0x...        # hardware breakpoint (Z1)
  (gdb) continue            # 'c' -> stop reply
```
A future integration test will point `irida::transport::GdbClient` at
`localhost:1234` and assert the same round-trips the mock server covers.
