# Irida

### *Light refracted into meaning.*

**Irida** is a **standalone reverse-engineering debugger** (Qt 6 GUI + headless C++ core) that attaches to a **running target externally via QEMU's gdbstub** and shows you the **real value beside every line, updating as you step** — the Visual Studio debugging feel, applied to reverse engineering.

Its differentiator is a **live-value annotation layer**: operands resolved against current guest state and shown as real meaning inline (instead of `[rax+8]`, the actual string / pointer chain / symbol), including observed decrypted values (the `xor("somestring")` case) pinned per-address.

> **The name:** *Irida* means "rainbow / of Iris" — from **Iris**, the Greek goddess of the rainbow and **messenger between worlds**. Both meanings map onto the tool: raw bytes **refracted into their real meaning** (like light through a prism), and a **messenger that crosses the boundary** into a running process to bring back its truth.

It attaches to a target running in a QEMU guest — that's the environment the debugger drives; easy to stand up (install QEMU + a guest and go).

> **Status:** early development. Milestone 1 — the layered build, the stable C ABI, and a Qt 6 GUI proving the live-value annotation against a mock backend — is in `main`. Next up: the real gdb-remote transport that talks to a live QEMU guest. See the [roadmap](#roadmap).

---

## The vision

A reverse engineer breakpoints into a packed, self-modifying binary. In every other debugger they see raw operands: `mov rcx, [rax+8]`, a wall of hex registers, ciphertext buffers. In **this** tool they see meaning:

```
mov  rcx, [rax+8]         ; rax+8 → 0x7ff6...  → "C:\Users\victim\ntdll.dll"
lea  rdx, [rip+0x2f1c]    ; → kernel32.dll+0x1a20  (CreateFileW)
xor  ... (decrypt loop)   ; observed: "http://c2.evil.example/beacon"
```

The tool reads the **live guest state right now**, resolves each operand against real registers and memory, follows pointer chains, detects strings, resolves symbols — and shows the result inline. When execution passes through a decryption routine, it **observes** the plaintext that genuinely exists in memory at that instant, **caches it per-address**, and keeps showing it even after you've run past. Not emulation — memory. It shows you what actually happened.

**The feel we're after is Visual Studio's debugger** — code on the left, live values right beside the line you're on and the lines around it, everything updating as you step. Irida brings that to reverse engineering:

- **v1:** that feel on **disassembly** — current + surrounding instructions, each annotated with live resolved values, updating every step.
- **v2–v3:** the same feel on **pseudocode** — a decompiler / IL view with **live values pinned next to each source line** of a *running, morphing* process (e.g. `local_18 = "somestring"`, `if (x == 0x1400)` with `x`'s actual current value beside it). Almost nothing does live-value-annotated decompilation of a live target — that's the endgame.

Irida is built from the ground up around **live, morphing memory as the source of truth** — and around making that live state *legible*.

---

## What it is (and isn't)

**It is:**
- A **standalone** RE debugger — **Qt 6 GUI** + **headless C++ core** + **stable C ABI** + (later) **Python** scripting.
- An **external, out-of-guest** debugger: it speaks **QEMU's gdbstub** over TCP. There is no debugger *inside* the guest, so the target's anti-debug checks (`IsDebuggerPresent`, DR0–DR7 reads, `0xCC` scans) come up clean — and under TCG, hardware breakpoints leave the guest's debug registers genuinely empty.
- Built around a **live-memory model**: correct under self-modifying, packed, JIT'd, morphing code. The bytes on disk are a lie; guest memory *now* is the truth.
- Differentiated by a **live-value annotation layer**: operands → real strings / pointer chains / symbols / observed decrypted values, inline.

**It is not (in v1):**
- A static analysis tool that happens to attach. It's live-first.
- An IL / decompiler view (that's later). v1 is live disassembly + annotations.
- An emulator that predicts values. It observes and remembers.

---

## Architecture at a glance

```
frontends/{gui, cli}  ──►  capi (stable C ABI)  ──►  core  ──►  QEMU gdbstub (TCP)
  Qt 6 GUI                  opaque handles +          transport / target /
  live-value disasm         POD structs               disasm / breakpoints /
                                                      eval / annotate / valuecache
```

Strict, one-way layering (`frontends → capi → core → libs`), enforced by the build: the core never depends on Qt (a headless build proves it), and a configure-time layer-check fails on any illegal dependency edge.

Everything anchors to a **stop-epoch** — a counter that ticks each time the guest halts. On each epoch the core re-reads registers/memory, detects what changed, invalidates stale disassembly, and records observed operand values. That's what makes self-modifying code correct and the annotations trustworthy.

---

## Building

Requires **Visual Studio 2022** (Desktop C++ workload) and **Qt 6** (MSVC 2022 64-bit). See [docs/guide/building.md](docs/guide/building.md) for full instructions.

```bat
cmake --preset gui-debug
cmake --build --preset gui-debug
build\gui-debug\frontends\gui\irida_gui.exe
```

A headless build (no Qt) is also supported: `cmake --preset headless-debug`.

---

## Roadmap

- **Milestone 1 — done:** layered build with enforcement, stable C ABI, MockBackend, and a Qt 6 GUI showing the live-value annotation column.
- **Milestone 2:** the gdb-remote transport — talk to a live QEMU guest; live registers, memory, and hardware breakpoints.
- **Later:** the operand evaluator + value cache (real live-value annotations and observed-value pinning), OS-aware module/symbol resolution, then the decompiler/IL view with live values beside pseudocode, and Python scripting.

---

## License

**Business Source License 1.1 (BSL 1.1)** — source-available.

- **Free** for **personal, non-commercial, research, and educational** use.
- **Commercial use requires a license** — contact the maintainer.
- Each released version **converts to Apache 2.0** four years after its release (rolling per-release Change Date).

See [LICENSE](LICENSE) for the full terms.
