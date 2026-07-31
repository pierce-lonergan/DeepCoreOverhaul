# NERPs — Language Reference and Assembler Specification

**Status:** research + specification. Nothing in this document has been implemented; §5 is a design
to be built, §1–§4 and §6 describe code that exists today.
**Verification ceiling:** we cannot run the game. Everything here is derived by reading the
interpreter and the built-in bodies, which are OURS (C++ in this tree). Claims that depend on data
inside `LegoRR.exe` that we cannot read are marked **UNDETERMINED** and never quietly assumed.

**Naming note:** every citation below is `file:line` against this tree. Where a symbol is a raw
address macro rather than C++ we own, it is labelled **EXE macro** at first use.

---

## Table of contents

1. [The machine](#1-the-machine)
2. [The bytecode format](#2-the-bytecode-format)
3. [The function table, and why the assembler binds to it live](#3-the-function-table-and-why-the-assembler-binds-to-it-live)
4. [Complete categorised reference — all 293 callable built-ins](#4-complete-categorised-reference--all-293-callable-built-ins)
5. [The assembler specification](#5-the-assembler-specification)
6. [The four traps an author must design around](#6-the-four-traps-an-author-must-design-around)
7. [Error handling the assembler must implement](#7-error-handling-the-assembler-must-implement)
8. [Decision and ranked plan](#8-decision-and-ranked-plan)

---

## 1. The machine

### 1.1 One program, re-run every tick

A NERPs script is loaded once at level load and the **entire program is executed from instruction 0
on every game tick**. There is no program counter that survives a tick. The loop is
`NERPsRuntime_Execute` (`game/mission/NERPsFile.cpp:346`, OURS, installed over `0x004535e0` at
`interop.cpp:3119`), called from the main loop at `game/GameState.cpp:1423`, gated only by the
`GAME1_DEBUG_NONERPS` debug flag (`GameState.cpp:1422`).

```
NERPsFile.cpp:355   const NERPsInstruction* instructions = nerpsfileGlobs.instructions;
NERPsFile.cpp:356   const uint32 instrCount = (nerpsfileGlobs.scriptSize / sizeof(NERPsInstruction));
NERPsFile.cpp:366   for (uint32 instrIdx = 0; instrIdx < instrCount; instrIdx++, currCmp = nextCmp) {
```

Consequences an author must internalise:

* **`Stop` means "end this tick's pass", not "end the script."** The next tick starts again at
  instruction 0.
* **All persistent state lives in 8 registers and 4 timers.** `NERPS_REGISTERCOUNT == 8`,
  `NERPS_TIMERCOUNT == 4` (`NERPsFile.h:49-50`), stored in `NERPsRuntime_Globs`
  (`NERPsFile.h:323-343`, bound to `0x00500958` at `NERPsFile.cpp:66`).
* **Anything you want to happen once must be latched into a register**, because the surrounding code
  will run again in 40 ms.

### 1.2 The register model

The interpreter has exactly four pieces of per-tick state, all local to `NERPsRuntime_Execute`:

| Name | Declared | Meaning |
| --- | --- | --- |
| `regA` | `NERPsFile.cpp:360` | the **condition register** — the accumulated value of the expression so far. Initialised to **0** every tick. |
| `regB` | `NERPsFile.cpp:361` | scratch — the value the current instruction just produced |
| `negate` | `NERPsFile.cpp:362` | pending boolean NOT, applied to the next `regB` |
| `currCmp` / `nextCmp` | `NERPsFile.cpp:363-364` | how the next produced value combines into `regA` |

`regA` and `regB` are declared **`uint32`** (`NERPsFile.cpp:360-361`). Every comparison in the
interpreter is therefore an **unsigned** comparison — see §2.6, this is a real trap.

### 1.3 ⚠ The single most important rule: `regA` starts at 0

`regA = 0` at the top of every tick (`NERPsFile.cpp:360`), and every action (void) built-in is
guarded:

```
NERPsFile.cpp:481   case NERPS_ARGS_0_NORETURN:
NERPsFile.cpp:482       if (regA) { // Action if expression is true
```

⇒ **The first statement of a script cannot be a bare action. It will never execute, ever.**

```
SetMessagePermit 0        ; DEAD CODE — regA is 0 at this point on every tick
True ? SetMessagePermit 0 ; correct
```

Every unconditional action needs a truthy expression in front of it. The idiom is `True ? Action`.

### 1.4 The evaluation rule

Exactly one rule governs everything. When an instruction **produces a value** (a literal load, or a
value-returning built-in), the interpreter does (`NERPsFile.cpp:435-469` for calls, `:593-629` for
literals — the two blocks are identical):

```
regB = <the value>
if (negate) regB = !regB
regA = combine(currCmp, regA, regB)     // currCmp == None  =>  regA = regB
negate = false
nextCmp = And
```

`combine` is the switch at `NERPsFile.cpp:438-466`:

| `currCmp` | result |
| --- | --- |
| `None` | `regB` |
| `And` | `regA && regB` |
| `Or` | `regA \|\| regB` |
| `Cgt` `Clt` `Ceq` `Cge` `Cle` `Cne` | `regA > regB`, `<`, `==`, `>=`, `<=`, `!=` (all unsigned) |

When an instruction is an **operator**, it only sets `nextCmp` (and `negate` for `#`); `regA` is
untouched (`NERPsFile.cpp:527-573`).

When an instruction is an **action, a label, or a jump**, it resets: `regA = 0`, `negate = false`,
`nextCmp = None` (`NERPsFile.cpp:518-520`, `:577-579`, `:587-589`).

Three things fall straight out of this and none of them are obvious:

1. **There is no operator precedence and no grouping.** An expression is a strict left fold.
   `GetR0 > 5 + GetR1 = 2` is `(((GetR0 > 5) && GetR1) == 2)`.
2. **Two values with no operator between them are ANDed**, because every value sets
   `nextCmp = And`. `GetR0 GetR1` means `GetR0 && GetR1`. The `+` operator is therefore redundant
   between two values — it is there for readability.
3. **After a comparison, `regA` is 0 or 1**, not the original number. So `GetR0 > 5 > 3` is
   `(GetR0 > 5) > 3`, which is always false.

### 1.5 Timers

`NERPsRuntime_UpdateTimers` (`NERPsFile.cpp:697`, OURS, installed `interop.cpp:3127`) runs once per
tick from `GameState.cpp:1666`:

```
NERPsFile.cpp:699   const real32 delta = elapsed * 1000.0f / STANDARD_FRAMERATE;
NERPsFile.cpp:700   for (uint32 i = 0; i < NERPS_TIMERCOUNT; i++) nerpsruntimeGlobs.timers[i] += delta;
```

`STANDARD_FRAMERATE == 25.0f` (`common.h:109`), so **timers count up in milliseconds**. They are
`real32` internally (`NERPsFile.h:336`) and truncated to `sint32` on read
(`NERPsFunctions.cpp:182`). All four timers always run; there is no stop, only `SetTimerN`.

⚠ **Timers cannot be compared against more than 32767 directly.** See §2.6 — a standalone literal is
sign-extended from 16 bits, so `GetTimer0 > 60000` compares against `0xFFFFEA60`, not 60000. §5.6
gives the two-register workaround.

---

## 2. The bytecode format

### 2.1 The file

There is no header, no magic number, no version, no symbol table. `NERPsFile_LoadScriptFile`
(`NERPsFile.cpp:95`, OURS, installed over `0x004530b0` at `interop.cpp:3107`) does exactly one
meaningful thing:

```
NERPsFile.cpp:110   nerpsfileGlobs.instructions = (NERPsInstruction*)Gods98::File_LoadBinary(filename, &nerpsfileGlobs.scriptSize);
NERPsFile.cpp:111   return (nerpsfileGlobs.instructions != nullptr);
```

The instruction count is `scriptSize / 4` (`NERPsFile.cpp:356`). **A file whose length is not a
multiple of 4 silently loses its tail.** Nothing validates opcodes, operands, function ids or jump
targets at load time.

The script filename comes from the level's `NERPFile` config key, read by `Lego_LoadLevel`
(`Game.h:1373`, **EXE macro** `0x004297c0`) — so the key list itself is not derivable from this tree.

### 2.2 The instruction

Four bytes, little-endian, fixed width:

```
NERPsFile.h:232   struct NERPsInstruction
NERPsFile.h:234       /*0,2*/ uint16 operand;
NERPsFile.h:235       /*2,2*/ NERPsOpcode opcode;
NERPsFile.h:238   assert_sizeof(NERPsInstruction, 0x4);
```

As a DWORD: `(opcode << 16) | operand`. The interpreter genuinely treats it as one DWORD in places —
that is why `NERPsRuntime_LoadLiteral` can overwrite a whole instruction with a 32-bit return value
(`NERPsFile.cpp:338-341`).

Byte layout on disk:

```
offset 0 : operand low  byte
offset 1 : operand high byte
offset 2 : opcode  low  byte
offset 3 : opcode  high byte
```

### 2.3 Opcodes

```
NERPsFile.h:129   flags_scoped(NERPsOpcode) : uint16
NERPsFile.h:131       Load     = 0,
NERPsFile.h:133       Operator = 0x1,
NERPsFile.h:134       Function = 0x2,
NERPsFile.h:135       Label    = 0x4,
NERPsFile.h:136       Jump     = 0x8,
NERPsFile.h:138       Mask     = (Operator|Function|Label|Jump),
```

They are tested with **bitwise AND, in a fixed priority order** — Function, then Operator, then
Label, then Jump, then Load as the fallthrough (`NERPsFile.cpp:380`, `:527`, `:576`, `:582`, `:592`).
An opcode word with two bits set dispatches as the highest-priority one and the rest are ignored.
An assembler must emit exactly one bit.

| Opcode | Bytes 2-3 | Operand means | Handled at |
| --- | --- | --- | --- |
| `Load` | `00 00` | a 16-bit literal, **sign-extended** | `NERPsFile.cpp:592-593` |
| `Operator` | `01 00` | index into `c_nerpsOperators`, 0-10 | `NERPsFile.cpp:527-573` |
| `Function` | `02 00` | function id into `c_nerpsFunctions` | `NERPsFile.cpp:380-526` |
| `Label` | `04 00` | **ignored entirely** | `NERPsFile.cpp:576-580` |
| `Jump` | `08 00` | destination instruction index | `NERPsFile.cpp:582-590` |

### 2.4 Operators

```
NERPsFile.h:145   enum class NERPsOperator : uint16
NERPsFile.h:147       Plus = 0, Pound = 1, FSlash = 2, BSlash = 3, Test = 4,
NERPsFile.h:152       Cgt = 5, Clt = 6, Ceq = 7, Cge = 8, Cle = 9, Cne = 10
```

and the spellings live in the exe's own string table, `c_nerpsOperators` at `0x004a7710`
(`NERPsFile.cpp:37`):
`{ "+", "#", "/", "\\", "?", ">", "<", "=", ">=", "<=", "!=" }`.

| Id | Text | Effect (`NERPsFile.cpp:530-573`) |
| ---: | --- | --- |
| 0 | `+` | `nextCmp = And`, `negate = false` |
| 1 | `#` | `negate = true`, `nextCmp = currCmp` — boolean NOT of the **next** value only |
| 2 | `/` | `nextCmp = Or` |
| 3 | `\` | **inert** (default branch, `:569-572`) — but it clears `negate` |
| 4 | `?` | **inert** (default branch, `:569-572`) — but it clears `negate` |
| 5 | `>` | `nextCmp = Cgt` |
| 6 | `<` | `nextCmp = Clt` |
| 7 | `=` | `nextCmp = Ceq` |
| 8 | `>=` | `nextCmp = Cge` |
| 9 | `<=` | `nextCmp = Cle` |
| 10 | `!=` | `nextCmp = Cne` |

⚠ `?` is **purely cosmetic**. The interpreter's own comment says so
(`NERPsFile.cpp:400-403`: "the Test operator '?' seems to be COMPLETELY USELESS!!"), and there is a
commented-out experiment at `NERPsFile.cpp:371-375` to prove it by skipping the opcode entirely.
`COND ? Action` and `COND Action` compile to different bytes and behave identically.

⚠ `# ?` cancels the negation. `#` sets `negate = true`; `?` immediately falls into the default
branch and sets `negate = false` (`NERPsFile.cpp:570`). Write `# GetR0 ? Action`, never
`# ? GetR0 Action`.

### 2.5 Calling a built-in

```
NERPsFile.cpp:380   if (instr.opcode & NERPsOpcode::Function) {
NERPsFile.cpp:381       uint32 funcId = (uint32)instr.operand;
NERPsFile.cpp:382       if (funcId == NERPS_FUNCID_STOP /*0*/ && (regA || currCmp == NERPsComparison::None)) break;
NERPsFile.cpp:384       NERPsFunctionArgs nargs = c_nerpsFunctions[funcId].arguments;
```

Arity is **data read from the exe's table**, not syntax (`NERPsFile.h:85-96`):

| `NERPsFunctionArgs` | Value | Class | Behaviour |
| --- | ---: | --- | --- |
| `NERPS_ARGS_0` | 0 | expression | 0 args, returns a value |
| `NERPS_ARGS_1` | 1 | expression | 1 arg, returns a value |
| `NERPS_ARGS_2` | 2 | expression | 2 args, returns a value |
| `NERPS_ARGS_0_NORETURN` | 3 | action | 0 args, gated on `regA` |
| `NERPS_ARGS_1_NORETURN` | 4 | action | 1 arg, gated on `regA` |
| `NERPS_ARGS_2_NORETURN` | 5 | action | 2 args, gated on `regA` |
| `NERPS_ARGS_3_NORETURN` | 6 | action | 3 args, gated on `regA` |
| `NERPS_END_OF_LIST` | 7 | — | falls into the `else` at `NERPsFile.cpp:522-524`: **a complete no-op** that only preserves `currCmp` |

**Arguments are the raw instruction words that follow the call.** They are copied verbatim
(`NERPsFile.cpp:419`, `:426-427`, `:488`, `:496-497`, `:506-508`) into a 3-slot array and the array
is reinterpreted as `sint32*`:

```
NERPsFile.cpp:419   argsStack[0] = instructions[instrIdx + 1];
NERPsFile.cpp:420   NERPsRuntime_LoadLiteral(&argsStack[0]);
NERPsFile.cpp:421   regB = c_nerpsFunctions[funcId].function((sint32*)argsStack);
```

`NERPsRuntime_LoadLiteral` (`NERPsFile.cpp:333`, OURS but **not installed** — `interop.cpp:3116` is
commented out) rewrites an argument word **only** when it is a `Function` opcode whose table arity is
`NERPS_ARGS_0`:

```
NERPsFile.cpp:336   if (instruction->opcode == NERPsOpcode::Function && c_nerpsFunctions[instruction->operand].arguments == NERPS_ARGS_0) {
NERPsFile.cpp:341       *(sint32*)instruction = c_nerpsFunctions[instruction->operand].function(nullptr);
```

Therefore, precisely:

* **A literal argument** is a `Load` word, so the DWORD is `0x0000xxxx` — **unsigned 0..65535, never
  negative**.
* **A zero-argument built-in as an argument** (`True`, `False`, `Null`, `GetR0`, `GetTimer0`,
  `GetCrystalsCurrentlyStored`, …) is substituted with its 32-bit return value. This is the *only*
  form of nesting the language supports.
* **Anything else used as an argument is passed as its raw DWORD.** The interpreter documents the
  worked example itself: `SetR2 SetR2` puts `0x0002001B` = 131099 into R2, "where `0x20000` is the
  opcode for functions, and `0x1b` is the function ID for `SetR2`" (`NERPsFile.cpp:391-392`).
* **Arguments are always consumed, even when the action does not fire.** The `instrIdx += N`
  statements sit outside the `if (regA)` (`NERPsFile.cpp:492`, `:502`, `:514`), so a skipped action
  never desynchronises the stream.

After an expression call: `regA = combine(...)`, `nextCmp = And` (`NERPsFile.cpp:467-469`).
After an action call: `regA = 0`, `nextCmp = None` (`NERPsFile.cpp:518-520`).

⚠ **Value-returning built-ins ignore the condition register.** `COND ? GetR0` does not gate anything —
`GetR0` always runs. Only the action class is conditional. The interpreter says this in prose at
`NERPsFile.cpp:394-398`.

### 2.6 ⚠ The two literal ranges are different

This is the sharpest edge in the format.

| Position | Path | Effective range |
| --- | --- | --- |
| **Statement** (a value in an expression) | `regB = (sint32)(sint16)instr.operand;` (`NERPsFile.cpp:593`) | **−32768 … 32767**, sign-extended |
| **Argument** (after a call) | raw DWORD `0x0000xxxx`, `LoadLiteral` leaves it alone (`NERPsFile.cpp:336`) | **0 … 65535**, unsigned |

So `SetTimer0 60000` works (argument position, 60000). `GetTimer0 > 60000` does **not** (statement
position: operand `0xEA60` → `(sint16)` → −5536 → as `uint32` → 4294961760).

And because `regA`/`regB` are `uint32` (`NERPsFile.cpp:360-361`), **negative values compare as huge
positives**. `GetR0 < 0` is never true. To test "is R0 negative", use `GetR0 > 32767`.

### 2.7 Labels and jumps

```
NERPsFile.cpp:576   else if (instr.opcode & NERPsOpcode::Label) {
NERPsFile.cpp:577       regA = 0; negate = false; nextCmp = NERPsComparison::None;
NERPsFile.cpp:582   else if (instr.opcode & NERPsOpcode::Jump) {
NERPsFile.cpp:583       if (regA) instrIdx = (uint32)instr.operand;
NERPsFile.cpp:587       regA = 0; negate = false; nextCmp = NERPsComparison::None;
```

* A `Label` instruction does nothing except reset the expression state. Its operand is never read.
* A `Jump` sets `instrIdx = operand`, and then the `for` increment adds 1
  (`NERPsFile.cpp:366`) — **execution resumes at `operand + 1`**. So a jump whose operand is the index
  of the target `Label` instruction resumes on the instruction *after* the label, which is what an
  author means. The assembler should record label indices and use them verbatim.
* ⚠ **`Jump` is conditional**, despite `NERPsFile.h:136` calling it "Unconditional jump". It only
  fires when `regA` is non-zero. Since the instruction immediately before a jump is very often an
  action (which zeroes `regA`), a bare `:Loop` at the end of a script usually does **nothing**.
  For a true unconditional jump write `True :Loop`.
* ⚠ **A backward unconditional jump is an infinite loop inside one tick** — the game hangs. There is
  no iteration counter and no watchdog. Because the whole program re-runs next tick anyway, backward
  jumps are almost never what you want; use registers instead.

### 2.8 `Stop`

`NERPS_FUNCID_STOP == 0` (`NERPsFile.h:47`), so function id 0 is `Stop` and the interpreter special-cases
it *before* reading arity:

```
NERPsFile.cpp:382   if (funcId == NERPS_FUNCID_STOP && (regA || currCmp == NERPsComparison::None)) break;
```

Read that carefully — `Stop` fires when `regA` is true **or** when there is no pending comparison:

| Preceding instruction | `currCmp` on reaching `Stop` | Result |
| --- | --- | --- |
| nothing (instruction 0) | `None` (`NERPsFile.cpp:363`) | **always halts** |
| an expression / literal | `And` | halts **iff** `regA` |
| an operator | unchanged | as above |
| an action, a label, a not-taken jump | `None` | **always halts** |

⇒ `COND ? Stop` is conditional. A `Stop` on the line after an action is **not** — it halts every
time. This is the second most common authoring mistake after §1.3.

When the special case does *not* fire, control falls through into the ordinary dispatch and
`c_nerpsFunctions[0]` is actually invoked. Our body is a no-op (`NERPsFunctions.cpp:1877-1880`,
`return_DUMMY()`). The **table arity of slot 0 is UNDETERMINED** (it is data inside the exe). Under
either plausible value — `ARGS_0` or `ARGS_0_NORETURN` — the observable effect is the same, `regA`
ends at 0 and no arguments are consumed. Only a slot-0 arity with arguments would desynchronise the
stream, which is why §7 requires the assembler to *check* this at load time rather than assume it.

---

## 3. The function table, and why the assembler binds to it live

### 3.1 The table

```
NERPsFile.h:222   struct NERPsFunctionSignature   // 0xc bytes
NERPsFile.h:224       /*0,4*/ const char* name;
NERPsFile.h:225       /*4,4*/ NERPsFunction function;      // sint32 (__cdecl*)(sint32* stack)
NERPsFile.h:226       /*8,4*/ NERPsFunctionArgs arguments;
NERPsFile.h:229   assert_sizeof(NERPsFunctionSignature, 0xc);
```

Two live references into the exe's data segment (both **EXE data**, bound by us):

```
NERPsFile.cpp:34  LegoRR::c_nerpsFunctions  = *(NERPsFunctionSignature(*)[294])0x004a6948;
NERPsFile.cpp:37  LegoRR::c_nerpsOperators  = *(const char*(*)[11])        0x004a7710;
```

The `const` is deliberately omitted at `NERPsFile.h:353-355` ("No const so that we can hook
functions") — this array is writable, and we write to it.

**The table has zero slack and cannot be extended in place.** Verified arithmetic:

```
0x004a6948 + (294 * 0xc = 0xdc8) = 0x004a7710   ==  &c_nerpsOperators   (NERPsFile.cpp:37)
0x004a7710 + (11  * 0x4 = 0x2c)  = 0x004a773c   ==  &nerpsHasNextButton (NERPsFile.cpp:40)
```

Both tables butt directly against their neighbours. Growing either **overwrites** the next region —
exactly the CARDINAL-RULE failure mode. Neither region appears in `docs/ADDRESS-MAP.md`, because
`tools/addrlint/addrlint.py` indexes `assert_sizeof` types and scalar bindings and an
array-of-struct reference binding is neither; the arithmetic above is the check that map would have
performed. Adding built-ins is a separate, later problem (see §8).

### 3.2 How a call resolves

Resolution is by **index only** at runtime — `funcId` is the array index
(`NERPsFile.cpp:381`, `:384`, `:415`). Names exist purely so tooling can find slots. That is exactly
what our hook installer does:

```
NERPsFile.cpp:80   bool LegoRR::NERPs_HookFunction(const char* name, NERPsFunction function)
NERPsFile.cpp:82       for (uint32 i = 0; i < _countof(c_nerpsFunctions); i++) {
NERPsFile.cpp:83           if (::_stricmp(c_nerpsFunctions[i].name, name) == 0) {
NERPsFile.cpp:84               c_nerpsFunctions[i].function = function;
NERPsFile.cpp:88       std::printf("NERPFunc: \"%s\" not found.\n", name);
```

Note `::_stricmp` — **name matching is case-insensitive**. `setr0`, `SetR0` and `SETR0` are the same
built-in.

`interop.cpp:3181-3506` makes exactly **293** `NERPs_hook_function(...)` calls (counted from the
tree). The 294th slot is the terminator, installed by name at `interop.cpp:3432`:

```
interop.cpp:3432   result &= LegoRR::NERPs_HookFunction("**End Of List**", LegoRR::NERPFunc__End_Of_List);
```

⇒ **293 callable built-ins occupy ids 0…292; id 293 is the sentinel.** Calling id 293 is a harmless
no-op (`NERPsFile.cpp:522-524`, because its arity is `NERPS_END_OF_LIST`).

### 3.3 Why the assembler must read this table instead of shipping an id list

The only two facts the assembler needs to turn a name into bytes are **the slot index** and **the
arity**. Both are already in memory, in the running process, before any script is loaded:

```cpp
// game/mission/NERPsFile.cpp — OURS.
// Mirrors NERPs_HookFunction (NERPsFile.cpp:82-87) exactly, including the _stricmp.
static sint32 NERPs_FindFunctionID(const char* name)
{
    for (uint32 i = 0; i < _countof(c_nerpsFunctions); i++) {
        const char* n = c_nerpsFunctions[i].name;
        if (n != nullptr && ::_stricmp(n, name) == 0)
            return static_cast<sint32>(i);
    }
    return -1;
}

static NERPsFunctionArgs NERPs_GetArity(uint32 id)   // id < 294, caller checks
{
    return c_nerpsFunctions[id].arguments;
}

static sint32 NERPs_FindOperatorID(const char* tok)
{
    for (uint32 i = 0; i < _countof(c_nerpsOperators); i++) {
        if (c_nerpsOperators[i] != nullptr && std::strcmp(c_nerpsOperators[i], tok) == 0)
            return static_cast<sint32>(i);
    }
    return -1;
}
```

That is the whole symbol resolver: about twenty lines, no data of its own. What it buys:

1. **No drift, structurally.** An external assembler carries a hard-coded name→id dictionary — the
   archived `jgrip/legorr` `npl.py` is the well-known one. Such a table is a *copy* of the exe's
   table, and copies rot. Ours cannot: it *is* the table. If a future build of the DLL re-points a
   slot, or a different localisation of `LegoRR.exe` orders the table differently, the assembler
   follows automatically.
2. **Arity validation for free.** The `arguments` field tells the assembler how many words to
   consume after a call, whether the call is an expression or an action, and whether it is legal in
   argument position (only `NERPS_ARGS_0` is). Without the table, an assembler has to hard-code
   arity too — a second copy that can rot independently of the first.
3. **No external toolchain.** Authoring becomes "edit a text file next to the level", which is the
   same workflow as every other piece of level data. No Python, no archived repo, no build step.
4. **It works with any build of the original.** The assembler never assumes an address for a
   *function*; it only reads the table it was already given. The two addresses it depends on,
   `0x004a6948` and `0x004a7710`, are the two this DLL already depends on to boot
   (`NERPsFile.cpp:34`, `:37`) — if they were wrong, nothing would work at all.
5. **A disassembler is the same twenty lines run backwards** (`id → c_nerpsFunctions[id].name`), so
   the stock 1999 campaign scripts become readable — the fastest available way to learn the idioms
   the original designers actually used, and the only way to empirically settle the UNDETERMINED
   items in §4.

The one thing the table cannot give us is the **id ordering**, which we cannot read from this tree
(no `LegoRR.exe` is present in the repo; `bin/*.exe` are the OpenLRR launchers). We do not need it.
Two ids are nevertheless independently confirmed from our own source and can be used as a self-test:
`Stop == 0` (`NERPsFile.h:47`) and `SetR2 == 27` (`0x1b`, from the interpreter's worked example at
`NERPsFile.cpp:392`).

---

## 4. Complete categorised reference — all 293 callable built-ins

**How to read this section.**

* **Arity** is given in the `NERPsFunctionArgs` vocabulary of §2.5. It is **inferred from the OURS
  body** in `game/mission/NERPsFunctions.cpp`: a body using `stack[0..n-1]` takes *n* arguments; a
  body ending in `return_VOID(...)` (`NERPsFunctions.cpp:38`) is an **action**, one ending in a plain
  `return` is an **expression**. The authoritative arity lives in the exe table and we cannot read
  it here — which is precisely why the assembler in §5 reads it at runtime and never trusts this
  column. Where the two could disagree, the runtime wins.
* **`file:line`** is the body in `game/mission/NERPsFunctions.cpp` unless stated. **All 293 bodies
  are OURS**; several delegate to **EXE macros** declared in `NERPsFile.h`, noted where it matters.
* Anything whose behaviour cannot be determined from this tree is marked **UNDETERMINED**.
* Total is exactly 293, partitioned with no leftovers and no duplicates.

### 4.0 Cross-cutting hazards

Before the tables — five properties that recur and will bite:

* ⚠ **Exact-boolean arguments.** Several built-ins compare `stack[0] == TRUE` rather than testing
  truthiness, so `2` does not mean "on": `SetMonsterAttackPowerstation` (`:365`),
  `SetMonsterAttackNowt` (`:389`), `SetMessagePermit` (`:2208`). Always pass exactly `0` or `1`.
* ⚠ **`AllowCameraMovement` is arithmetic, not a flag.** It stores `stack[0] << 12`
  (`:97`) and the `ClickOnly*`/`DisallowAll` family **subtracts** that value from the tutorial flags
  (`:108`, `:121`, `:134`, `:147`, `:160`). A value other than 0 or 1 corrupts the flag word.
* ⚠ **Block coordinates in `SetRockMonster` are 1-based.** `bx = stack[0] - 1`, `by = stack[1] - 1`
  (`:511-512`). Everything else in the engine is 0-based.
* ⚠ **Block-pointer "is ground / is path" queries are not counts.** They accumulate `0x8` per ground
  block and `0x20000000` per path block, not 1 (`NERPsFile.cpp:1159-1160`, `:1168-1169`, both flagged
  `/// FIXME:` and deliberately preserved). Test them with `!= 0`, never `= 1`.
* ⚠ **`GetOxygenLevel` is not a pure query.** If oxygen has fallen below 1.0 it calls
  `Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED)` — the level-failure teleport-up — as a side
  effect of being *read* (`:458-463`; the body's own comment: "WHY ARE WE TRIGGERING THE LEVEL
  FAILURE TELEPORTING HERE!??"). Reading oxygen every tick in a low-oxygen level is not free.

### 4.1 Flow control — 4

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `Stop` | see §2.8 | `:1877` | ends this tick's pass. Special-cased at `NERPsFile.cpp:382` before arity is read. |
| `True` | `ARGS_0` | `:2464` | returns 1. The canonical unconditional guard. |
| `False` | `ARGS_0` | `:1863` | returns 0. |
| `Null` | `ARGS_0` | `:1870` | returns 0. Identical to `False`; the exe merges them at `0x004564d0` (`NERPsFunctions.h:1009-1015`). |

(`**End Of List**`, table slot 293, is not callable-by-name from a script in any useful sense — it is
a no-op. `NERPFunc__End_Of_List` at `:1884` is the only one of the 294 bodies not installed by
`NERPs_hook_function`; it is installed by literal name at `interop.cpp:3432`.)

### 4.2 Registers and arithmetic — 32

Eight 32-bit registers, `NERPsRuntime_Globs::registers[8]` (`NERPsFile.h:325`), zeroed when the
script is loaded (`NERPsFile.cpp:102`).

| Family | Arity | Bodies | Behaviour |
| --- | --- | --- | --- |
| `GetR0` … `GetR7` | `ARGS_0` | `:1938`-`:1980` | read register *n* |
| `SetR0` … `SetR7` | `ARGS_1_NORETURN` | `:2101`-`:2150` | `Rn = arg` |
| `AddR0` … `AddR7` | `ARGS_1_NORETURN` | `:1987`-`:2036` | `Rn += arg` |
| `SubR0` … `SubR7` | `ARGS_1_NORETURN` | `:2044`-`:2093` | `Rn -= arg` |

There is no multiply, divide, or register-to-register move. `SetRn GetRm` works, because `GetRm` is
`ARGS_0` and is substituted by `LoadLiteral` (§2.5) — that is the register-to-register move.

### 4.3 Timers — 8

| Family | Arity | Bodies | Behaviour |
| --- | --- | --- | --- |
| `GetTimer0` … `GetTimer3` | `ARGS_0` | `:180`-`:198` | milliseconds since last set, truncated to `sint32` |
| `SetTimer0` … `SetTimer3` | `ARGS_1_NORETURN` | `:205`-`:226` | set to *arg* ms (0…65535) |

All four always count up (`NERPsFile.cpp:700-701`). See §1.5 and §2.6 for the 32767 comparison ceiling.

### 4.4 Randomness — 4

| Name | Arity | Body | Range |
| --- | --- | --- | --- |
| `GetRandom` | `ARGS_0` | `:1825` | `Maths_Rand() & 0xfff` → 0…4095 |
| `GetRandomTrueFalse` | `ARGS_0` | `:1831` | 0 or 1 |
| `GetRandom10` | `ARGS_0` | `:1837` | 0…9 |
| `GetRandom100` | `ARGS_0` | `:1843` | 0…99 |

### 4.5 Mission outcome — 7

⚠ **All four setters route through `Objective_SetStatus`, and §6.2 makes that a no-op on any level
with a `CrystalObjective`.** Read §6 before using any of them.

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `SetLevelCompleted` | `ARGS_0_NORETURN` | `:659` | `Objective_SetStatus(LEVELSTATUS_COMPLETE)` |
| `SetLevelFail` | `ARGS_0_NORETURN` | `:676` | `Objective_SetStatus(LEVELSTATUS_FAILED)` |
| `SetGameFail` | `ARGS_0_NORETURN` | `:683` | `Objective_SetStatus(LEVELSTATUS_FAILED)` |
| `SetGameCompleted` | `ARGS_0_NORETURN` | `:667` | ⚠ **fails the level.** See §6.4. |
| `SetObjectiveSwitch` | `ARGS_1_NORETURN` | `:2305` | arms a one-shot latch |
| `GetObjectiveSwitch` | `ARGS_0` | `:2312` | **consumes** the latch: returns 1 once, then 0 until re-armed (`:2315-2319`). Set by `Objective_StopShowing` (`Objective.cpp:791`), i.e. "the briefing just closed". |
| `GetObjectiveShowing` | `ARGS_0` | `:2323` | briefing / complete / fail window is up (`Objective.cpp:805-808`) |

### 4.6 Messages and narration — 8

The message file is separate data, loaded by `NERPsFile_LoadMessageFile` (`NERPsFile.cpp:115`, OURS,
installed `interop.cpp:3109`) from the level's `NERPMessageFile` key. Its format:

* any control character (`< ' '`) ends a line, **including tab** (`NERPsFile.cpp:153-157`);
* **underscores become spaces** (`NERPsFile.cpp:150`) — text cannot contain a literal `_`;
* `:key  path\to\image.bmp` defines an image (`:169-207`);
* `$key  path\to\sound` defines a sound, **no extension** (`:211-252`);
* anything else is a text line, appended in order (`:253-268`);
* inside a text line, `#soundKey#` plays a sound and `<imageKey>` draws an icon; both are stripped
  from the displayed text by `NERPs_Level_NERPMessage_Parse` (`NERPsFile.cpp:998`, OURS).

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `SetMessage` | `ARGS_2_NORETURN` | `:2242` | arg0 = **1-based** message-file line (`:2248-2251`); arg1 non-zero disables the "next" arrow. ⚠ see the two warnings below. |
| `SetMessagePermit` | `ARGS_1_NORETURN` | `:2204` | exact boolean (`:2208`). Set to `true` when a script is loaded (`NERPsFile.cpp:105-108`). |
| `SetMessageWait` | `ARGS_1_NORETURN` | `:2219` | ⚠ see below |
| `SetMessageTimerValues` | `ARGS_3_NORETURN` | `:2226` | `{sample-length multiplier, ms added after a sample, ms for a silent line}`; defaults `{1000, 500, 4000}` (`NERPsFile.cpp:47`) |
| `GetMessageTimer` | `ARGS_0` | `:2236` | ms remaining on the current message |
| `GetMessagesAreUpToDate` | `ARGS_0` | `:63` | all message lines have been advanced past |
| `AdvanceMessage` | `ARGS_0_NORETURN` | `:82` | force-advance to the next queued line |
| `SupressArrow` | `ARGS_1_NORETURN` | `:69` | hide the "next" arrow (spelling is the table's, with one `p`) |

⚠ **`SetMessageWait 1` freezes game logic.** `SetMessage` then enters a loop that runs only the
input pump until a shortcut key is pressed (`:2282-2299`). Our version is far better behaved than the
original — it calls `Gods98::Main_LoopUpdate2` instead of spinning on `Input_ReadKeys` — but it is
still a blocking modal wait inside a per-tick script. Use it only for a deliberate "press to
continue" beat, and never inside a branch that can re-enter next tick.

⚠ **`SetMessage` early-returns when `NERPsRuntime_IsMessagePermit()` is true** (`:2244-2246`).
`NERPsRuntime_IsMessagePermit` is an **EXE macro** at `0x00456900` (`NERPsFile.h:648`), so we cannot
read what it returns. If it simply returns `nerpsruntimeGlobs.messagePermit`, then — because the
loader sets that flag to `true` (`NERPsFile.cpp:105-108`) — **`SetMessage` does nothing until a
script calls `SetMessagePermit 0`**, which contradicts the field comment "allows NERPs messages to
display" (`NERPsFile.h:326`). **UNDETERMINED.** Practical guidance: put
`True ? SetMessagePermit 0` at the top of any script that displays messages and treat the flag as
"a message is currently held", not as "messages are allowed".

### 4.7 Camera — 9

Camera state lives in `NERPsFile_Globs` (`NERPsFile.h:292-301`) and is stepped once per tick by
`NERPsRuntime_EndExecute` (`NERPsFile.cpp:739`, OURS, called at `:636`).

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `CameraLockOnObject` | `ARGS_1_NORETURN` | `:253` | arg is a **1-based** record-object pointer; `0` is a no-op (`:255-256`) |
| `CameraLockOnMonster` | `ARGS_1_NORETURN` | `:234` | non-zero locks to the first rock monster found; zero unlocks |
| `CameraUnlock` | `ARGS_0_NORETURN` | `:263` | clears lock and record pointer |
| `CameraZoomIn` | `ARGS_1_NORETURN` | `:272` | zoom in by *arg*, spread over time |
| `CameraZoomOut` | `ARGS_1_NORETURN` | `:292` | zoom out by *arg* |
| `CameraRotate` | `ARGS_1_NORETURN` | `:313` | degrees; **> 180 becomes negative** (counter-clockwise), `:325-327` |
| `AllowCameraMovement` | `ARGS_1_NORETURN` | `:92` | ⚠ 0 or 1 only — see §4.0 |
| `SetCameraGotoTutorial` | `ARGS_1_NORETURN` | `:2374` | move the camera to a block-pointer group |
| `GetCameraAtTutorial` | `ARGS_1` | `:2385` | is the camera on a block in that group |

Both zoom and rotate were fixed by us relative to 1999 (`NERPsFile.cpp:770-779`, `:800-814`): zoom
used to complete instantly on fast machines, and counter-clockwise rotation over-sped.

### 4.8 Game pacing — 3

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `SetGameSpeed` | `ARGS_2_NORETURN` | `:49` | arg0 = percent (100 = normal, `:56`), arg1 = lock. `NERPsRuntime_EndExecute` force-restores `{100, false}` once the level status leaves `INCOMPLETE` (`NERPsFile.cpp:830-832`). |
| `SetPauseGame` | `ARGS_1_NORETURN` | `:619` | `Lego_SetPaused(false, arg)` |
| `GetAnyKeyPressed` | `ARGS_0` | `:626` | `Gods98::Input_AnyKeyPressed()` |

### 4.9 Tutorial framing and UI restriction — 7

`TutorialFlags` (`NERPsFile.h:63-81`), a 13-bit mask; every bit is a **prohibition**:

| Bit | Name | Bit | Name |
| ---: | --- | ---: | --- |
| `0x1` | `NOICONS` | `0x100` | `NOINFO` |
| `0x2` | `NOBLOCKACTION` | `0x200` | `NOMULTISELECT` |
| `0x4` | `NOMAP` | `0x400` | `NOCYCLEUNITS` |
| `0x8` | `NOOBJECTS` | `0x800` | `NOHELPWINDOW` |
| `0x10` | `NORADAR` | `0x1000` | `NOCAMERA` |
| `0x20` | `NOOPTIONS` | | |
| `0x40` | `NOPRIORITIES` | `0x1fff` | `TUTORIAL_FLAGS_ALL` |
| `0x80` | `NOCALLTOARMS` | `0` | `TUTORIAL_FLAG_NONE` |

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `SetTutorialFlags` | `ARGS_1_NORETURN` | `:2158` | raw mask. 0 leaves tutorial mode entirely (`NERPsFile.h:415`). |
| `GetTutorialFlags` | `ARGS_0` | `:1931` | current mask |
| `DisallowAll` | `ARGS_0_NORETURN` | `:154` | `ALL` minus `allowCameraMovement` |
| `ClickOnlyObjects` | `ARGS_0_NORETURN` | `:102` | `ALL & ~(NOMAP\|NOOBJECTS)` |
| `ClickOnlyMap` | `ARGS_0_NORETURN` | `:115` | `ALL & ~NOMAP` |
| `ClickOnlyIcon` | `ARGS_0_NORETURN` | `:128` | `ALL & ~NOICONS` |
| `ClickOnlyCalltoarms` | `ARGS_0_NORETURN` | `:141` | `ALL & ~NOCALLTOARMS` |

⚠ Being in tutorial mode at all (`tutorialFlags != 0`) changes engine behaviour well outside NERPs —
it suppresses `GAME2_INMENU` handling in `Objective_SetStatus` (`Objective.cpp:641-643`) and
`Objective_StopShowing` (`Objective.cpp:797-799`), and suppresses the in-game advisor for message
sounds (`NERPsFile.cpp:1087-1090`). A non-tutorial mission should leave the flags at 0.

### 4.10 Tutorial HUD and pointer — 4

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `SetIconPos` | `ARGS_2_NORETURN` | `:632` | screen x,y of the tutorial icon strip. ⚠ the default `{260, 386}` is hard-coded for a 640×480 screen (`NERPsFile.cpp:49-51`). |
| `SetIconSpace` | `ARGS_1_NORETURN` | `:643` | pixel spacing |
| `SetIconWidth` | `ARGS_1_NORETURN` | `:651` | pixel width |
| `SetTutorialPointer` | `ARGS_2_NORETURN` | `:690` | `NERPsRuntime_SetTutorialPointer(blockPointerIdx, mode)` — **EXE macro** `0x00457430` (`NERPsFile.h:705`). Meaning of `mode` (0 or 1) is **UNDETERMINED**. |

### 4.11 Spawning, threat and combat — 17

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `SetRockMonster` | `ARGS_2_NORETURN` | `:506` | spawn the level's `EmergeCreature` at **1-based** block (arg0, arg1) (`:511-512`). No-op if the level has no valid emerge creature (`:510`). |
| `SetRockMonsterAtTutorial` | `ARGS_1_NORETURN` | `:2363` | spawn one per block in a block-pointer group; the spawned monster gets `PainThreshold = 60` (`NERPsFile.cpp:1206`) |
| `GenerateSlug` | `ARGS_0_NORETURN` | `:468` | spawn a slug. ⚠ uses the hard-coded name `OBJECT_NAME_SLUG` rather than the level's `Slug` key — `/// TODO:` at `:470-471`. |
| `SetCongregationAtTutorial` | `ARGS_2_NORETURN` | `:2349` | arg1 non-zero = start monsters congregating on the group, zero = stop (`:2352-2355`) |
| `SetMonsterAttackPowerstation` | `ARGS_1_NORETURN` | `:357` | ⚠ exact boolean. `1` = stop attacking power-generating buildings; anything else = attack all buildings (`:365-375`). |
| `SetMonsterAttackNowt` | `ARGS_1_NORETURN` | `:385` | ⚠ exact boolean. `1` = stop attacking everything, else attack everything. |
| `SetRockMonsterHealth` | `ARGS_1_NORETURN` | `:606` | ⚠ **misnamed.** Routes to `SetHealth`, whose handler sets `PainThreshold = 60.0f` and **ignores the argument** (`NERPsFile.cpp:931-935`). |
| `SetRockMonsterPainThreshold` | `ARGS_1_NORETURN` | `:593` | ⚠ **also misnamed.** Routes to `SetPainThreshold`, whose handler assigns `liveObj->health = arg` (`NERPsFile.cpp:927-929`). The two are swapped relative to their names; this is preserved 1999 behaviour. |
| `GetRockMonsterRunningAway` | `ARGS_0` | `:579` | count of monsters below the pain threshold; **also ends their route** as a side effect (`NERPsFile.cpp:921`) |
| `GetRockMonstersDestroyed` | `ARGS_0` | `:519` | from the reward counters |
| `GetMonstersOnLevel` | `ARGS_0` | `:2178` | ⚠ hard-coded sum of `RockMonster` + `IceMonster` + `LavaMonster` by name (`:2180-2185`). Custom species are invisible to it. |
| `GetSlugsOnLevel` | `ARGS_0` | `:1459` | |
| `GetMonsterAtTutorial` | `ARGS_1` | `:2451` | monsters standing on a block-pointer group |
| `SetAttackDefer` | `ARGS_1_NORETURN` | `:486` | `Lego_SetAttackDefer` |
| `SetCallToArms` | `ARGS_1_NORETURN` | `:493` | force action stations on/off |
| `GetCallToArmsButtonClicked` | `ARGS_0` | `:500` | `legoGlobs.flags2 & GAME2_CALLTOARMS` |
| `FlashCallToArmsIcon` | `ARGS_1_NORETURN` | `:167` | non-zero starts the advisor pointing at the button; zero ends the advisor |

### 4.12 Resources, counters and level state — 17

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `AddPoweredCrystals` | `ARGS_1_NORETURN` | `:838` | add *arg* stored crystals and request a power-grid update |
| `AddStoredOre` | `ARGS_1_NORETURN` | `:850` | add *arg* stored ore |
| `GetCrystalsCurrentlyStored` | `ARGS_0` | `:1855` | `Level_GetCrystalCount(true)` |
| `GetCrystalsPickedUp` | `ARGS_0` | `:1849` | |
| `GetOreCurrentlyStored` | `ARGS_0` | `:1925` | |
| `GetOrePickedUp` | `ARGS_0` | `:1919` | |
| `GetCrystalsUsed` | `ARGS_0` | `:1891` | ⚠ **always 0** — unimplemented in 1999, `return_NOT_IMPLEMENTED()` |
| `GetCrystalsStolen` | `ARGS_0` | `:1898` | ⚠ always 0 |
| `GetOreUsed` | `ARGS_0` | `:1905` | ⚠ always 0 |
| `GetOreStolen` | `ARGS_0` | `:1912` | ⚠ always 0 |
| `GetStudCount` | `ARGS_0` | `:1429` | count of `OBJECT_NAME_STUD` on the level |
| `GetPathsBuilt` | `ARGS_0` | `:1423` | count of `OBJECT_NAME_PATH` |
| `GetOxygenLevel` | `ARGS_0` | `:447` | ⚠ has a side effect — see §4.0 |
| `GetHiddenObjectsFound` | `ARGS_0` | `:526` | counter incremented by `NERPsRuntime_IncHiddenObjectsFound`, **EXE macro** `0x00454b40` (`NERPsFile.h:529`) |
| `SetHiddenObjectsFound` | `ARGS_1_NORETURN` | `:532` | overwrite that counter |
| `GetBuildingsTeleported` | `ARGS_0` | `:2189` | |
| `SetBuildingsTeleported` | `ARGS_1_NORETURN` | `:2197` | |

### 4.13 Block pointers — the map-anchored primitives — 11

A block pointer is `{Point2I blockPos; uint32 id;}` (`NERPsFile.h:195-201`). The level's
`blockPointerMap` assigns an id to map cells; `Lego_Level::blockPointers[LEGO_MAXBLOCKPOINTERS]`
holds them (`Game.h:439`, `LEGO_MAXBLOCKPOINTERS == 56`, `GameCommon.h:115`). Every function below
takes a **group id** and iterates every block carrying it, via `NERPsRuntime_EnumerateBlockPointers`
(**EXE macro** `0x00456f70`, `NERPsFile.h:682`). The 13 verbs are dispatched by
`NERPsRuntime_TutorialActionCallback` (`NERPsFile.cpp:1148`, OURS).

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `GetTutorialBlockIsGround` | `ARGS_1` | `:2396` | ⚠ adds **8** per floor block, not 1 (`NERPsFile.cpp:1160`) |
| `SetTutorialBlockIsGround` | `ARGS_1_NORETURN` | `:2418` | `Level_DestroyWall` on every block in the group |
| `GetTutorialBlockIsPath` | `ARGS_1` | `:2407` | ⚠ adds **0x20000000** per path block (`NERPsFile.cpp:1169`) |
| `SetTutorialBlockIsPath` | `ARGS_1_NORETURN` | `:2429` | clears rubble, then lays path |
| `GetTutorialCrystals` | `ARGS_1` | `:865` | crystals lying on the group |
| `SetTutorialCrystals` | `ARGS_2_NORETURN` | `:713` | place arg1 crystals across group arg0 |
| `GetTutorialBlockClicks` | `ARGS_1` | `:877` | accumulated clicks on the group |
| `SetTutorialBlockClicks` | `ARGS_2_NORETURN` | `:701` | set/reset the click counter |
| `SetOreAtIconPositions` | `ARGS_2_NORETURN` | `:725` | place arg1 ore across group arg0 |
| `GetUnitAtBlock` | `ARGS_1` | `:2440` | units standing on the group |
| `MakeSomeoneOnThisBlockPickUpSomethingOnThisBlock` | `ARGS_1_NORETURN` | `:2334` | a mini-figure on the group collects a crystal on the group (`NERPsFile.cpp:1241-1262`) |

Also block-pointer based, listed elsewhere by topic: `SetRockMonsterAtTutorial` (§4.11),
`SetCongregationAtTutorial` (§4.11), `GetMonsterAtTutorial` (§4.11), `SetCameraGotoTutorial` (§4.7),
`GetCameraAtTutorial` (§4.7), `GetRecordObjectAtTutorial` (§4.14),
`GetRecordObjectAmountAtTutorial` (§4.14).

### 4.14 Record objects — 4

`OBJECT_MAXRECORDOBJECTS == 10` (`GameCommon.h:124`). Record-object pointers are **1-based** in NERPs;
`0` means "none".

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `GetSelectedRecordObject` | `ARGS_0` | `:335` | 1-based index of the selected record object, or 0 |
| `GetRecordObjectAtTutorial` | `ARGS_1` | `:402` | 1-based index of a record object standing on a block-pointer group, or 0 |
| `GetRecordObjectAmountAtTutorial` | `ARGS_1` | `:421` | how many record objects are on the group |
| `SetRecordObjectPointer` | `ARGS_1_NORETURN` | `:439` | stores into `nerpsfileGlobs.RecordObjectPointer` (`NERPsFile.h:316`). ⚠ **UNDETERMINED** — nothing in this tree reads that field; the reader is either exe code or it is dead. |

### 4.15 Unit selection queries — 7

`GetMiniFigureSelected` counts selected pilots (`NERPsRuntime_CountSelectedUnits_ByObjectName`,
**EXE macro** `0x00455070`). The six vehicle queries test only the **primary** selected unit
(`Interface_GetPrimarySelectedUnit`) and return a boolean.

| Name | Arity | Body | Object name |
| --- | --- | --- | --- |
| `GetMiniFigureSelected` | `ARGS_0` | `:737` | `PILOT` (a count, not a boolean) |
| `GetSmallTruckSelected` | `ARGS_0` | `:743` | `SMALLTRUCK` |
| `GetSmallDiggerSelected` | `ARGS_0` | `:758` | `SMALLDIGGER` |
| `GetRapidRiderSelected` | `ARGS_0` | `:774` | `RAPIDRIDER` |
| `GetSmallHelicopterSelected` | `ARGS_0` | `:789` | `TUNNELSCOUT` |
| `GetGraniteGrinderSelected` | `ARGS_0` | `:804` | `GRANITEGRINDER` |
| `GetChromeCrusherSelected` | `ARGS_0` | `:819` | `CHROMECRUSHER` |

### 4.16 Driver queries — 6

All route to `NERPs_Game_DoCallbacks_ByObjectName` (**EXE macro** `0x00455320`,
`NERPsFile.h:593`): is a mini-figure currently driving one of these?

`GetMiniFigureinGraniteGrinder` (`:889`), `GetMiniFigureinChromeCrusher` (`:895`),
`GetMiniFigureinSmallDigger` (`:901`), `GetMiniFigureinRapidRider` (`:907`),
`GetMiniFigureinSmallTruck` (`:913`), `GetMiniFigureinSmallHelicopter` (`:919`).
All `ARGS_0`. Note the lower-case `in` in the table's spelling (matching is case-insensitive anyway).

### 4.17 Units and vehicles on the level — 5

All `ARGS_0`, all `NERPsRuntime_GetLevelObjectsBuilt(name, NERPS_GETLEVELS_ALL)` (**EXE macro**
`0x004554b0`; `NERPS_GETLEVELS_ALL == -1`, `NERPsFile.h:52`):

`GetMiniFiguresOnLevel` (`:1465`), `GetSmallDiggersOnLevel` (`:1453`),
`GetSmallHelicoptersOnLevel` (`:1435`), `GetGraniteGrindersOnLevel` (`:1441`),
`GetRapidRidersOnLevel` (`:1447`).

### 4.18 Building counts — 38

Four parallel families over the same nine building names, plus two legacy names. All `ARGS_0`.
`NERPS_GETLEVELS_POWERED == -2` (`NERPsFile.h:53`).

| Family | Selector | Bodies |
| --- | --- | --- |
| `Get{X}Built` | all upgrade levels | `:1096`-`:1144` |
| `GetPowered{X}Built` | powered only | `:1042`-`:1090` |
| `GetLevel1{X}Built` | upgrade level 1 | `:1150`-`:1198` |
| `GetLevel2{X}Built` | upgrade level 2 | `:1204`-`:1252` |

`{X}` ∈ `Barracks`, `Docks`, `Geodome`, `Powerstations`, `ToolStores`, `Gunstations`, `Teleports`
(= Teleport Pad), `VehicleTeleports` (= Super Teleport), `UpgradeStations`. Note the pluralisation is
irregular and comes from the table verbatim — `GetBarracksBuilt`, `GetGeodomeBuilt`,
`GetPowerstationsBuilt`. Name lookup is case-insensitive but the *spelling* must match a table entry.

Plus two that will always return 0:

| Name | Body | Why |
| --- | --- | --- |
| `GetOreRefineriesBuilt` | `:1471` | ⚠ queries `OBJECT_LEGACY_NAME_OREREFINERY` ("Refinery"), an object name the shipped game never defines — `/// FIXME:` at `:1473` |
| `GetCrystalRefineriesBuilt` | `:1478` | ⚠ same, `"CrystalRefinery"` — `/// TODO:` at `:1480` |

### 4.19 Building selection — 9

All `ARGS_0`, all `NERPsRuntime_CountSelectedUnits_ByObjectName`:
`GetBarracksSelected` (`:988`), `GetDocksSelected` (`:994`), `GetGeoDomeSelected` (`:1000`),
`GetPowerstationSelected` (`:1006`), `GetToolStoreSelected` (`:1012`),
`GetGunstationSelected` (`:1018`), `GetTeleportPadSelected` (`:1024`),
`GetSuperTeleportSelected` (`:1030`), `GetUpgradeStationSelected` (`:1036`).

### 4.20 Building upgrade levels — 9

All `ARGS_1_NORETURN`, all `NERPs_SetObjectsLevel(name, arg)` (**EXE macro** `0x004546d0`) — set
*every* building of that type to upgrade level *arg*:
`SetBarracksLevel` (`:925`), `SetDocksLevel` (`:932`), `SetGeoDomeLevel` (`:939`),
`SetPowerstationLevel` (`:946`), `SetToolStoreLevel` (`:953`), `SetGunstationLevel` (`:960`),
`SetTeleportPadLevel` (`:967`), `SetSuperTeleportLevel` (`:974`), `SetUpgradeStationLevel` (`:981`).

### 4.21 Icons — 81

The largest family, and the whole reason guided tutorials are possible. Every icon has a
**Get / Set / Flash** triple with a uniform contract:

* `Get…` — `ARGS_0`, returns the click count since it was last cleared.
* `Set…` — `ARGS_1_NORETURN`, writes the click count (pass `0` to consume/clear a click).
* `Flash…` — `ARGS_1_NORETURN`, non-zero makes the icon flash and points the advisor at it.

**Build-menu icons — 24** (8 × 3). These route through the submenu helpers
`NERPs_SubMenu_GetBuildingVehicleIcon_ByObjectName` / `NERPsRuntime_SetSubmenuIconClicked` /
`NERPsRuntime_FlashSubmenuIcon` (all **EXE macros**, `NERPsFile.h:636`, `:550`, `:554`):

| Get (`:1258`-`:1305`) | Set (`:1311`-`:1360`) | Flash (`:1367`-`:1416`) | Object |
| --- | --- | --- | --- |
| `GetBarracksIconClicked` | `SetBarracksIconClicked` | `FlashBarracksIcon` | Barracks |
| `GetGeodomeIconClicked` | `SetGeodomeIconClicked` | `FlashGeodomeIcon` | Geo Dome |
| `GetPowerstationIconClicked` | `SetPowerstationIconClicked` | `FlashPowerStationIcon` | Power Station |
| `GetToolStoreIconClicked` | `SetToolStoreIconClicked` | `FlashToolStoreIcon` | Tool Store |
| `GetGunstationIconClicked` | `SetGunstationIconClicked` | `FlashGunstationIcon` | Gunstation |
| `GetTeleportPadIconClicked` | `SetTeleportPadIconClicked` | `FlashTeleportPadIcon` | Teleport Pad |
| `GetVehicleTransportIconClicked` | `SetVehicleTransportIconClicked` | `FlashVehicleTransportIcon` | Super Teleport |
| `GetUpgradeStationIconClicked` | `SetUpgradeStationIconClicked` | `FlashUpgradeStationIcon` | Upgrade Station |

⚠ Note `FlashPowerStationIcon` capitalises the `S` where the other two do not. Resolution is
case-insensitive, so this only matters if you grep.

**Unit-action icons — 51** (17 × 3). These route through `Interface_GetIconClicked` /
`Interface_SetIconClicked` / `NERPsRuntime_FlashIcon`:

| Get (`:1485`-`:1581`) | Set (`:1587`-`:1699`) | Flash (`:1706`-`:1818`) | `Interface_MenuItemType` |
| --- | --- | --- | --- |
| `GetTeleportIconClicked` | `SetTeleportIconClicked` | `FlashTeleportIcon` | `TeleportMan` |
| `GetDynamiteClicked` | `SetDynamiteClicked` | `FlashDynamiteIcon` | `Dynamite` |
| `GetMountIconClicked` | `SetMountIconClicked` | `FlashMountIcon` | `GetIn` |
| `GetDismountIconClicked` | `SetDismountIconClicked` | `FlashDismountIcon` | `GetOut` |
| `GetTrainIconClicked` | `SetTrainIconClicked` | `FlashTrainIcon` | `TrainSkill` |
| `GetTrainDriverIconClicked` | `SetTrainDriverIconClicked` | `FlashTrainDriverIcon` | `TrainDriver` |
| `GetTrainPilotIconClicked` | `SetTrainPilotIconClicked` | `FlashTrainPilotIcon` | `TrainPilot` |
| `GetTrainSailorIconClicked` | `SetTrainSailorIconClicked` | `FlashTrainSailorIcon` | `TrainSailor` |
| `GetGetToolIconClicked` | `SetGetToolIconClicked` | `FlashGetToolIcon` | `GetTool` |
| `GetGetLaserIconClicked` | `SetGetLaserIconClicked` | `FlashGetLaserIcon` | `GetLaser` |
| `GetGetPusherIconClicked` | `SetGetPusherIconClicked` | `FlashGetPusherIcon` | `GetPusherGun` |
| `GetGetSonicBlasterIconClicked` | `SetGetSonicBlasterIconClicked` | `FlashGetSonicBlasterIcon` | `GetBirdScarer` |
| `GetDropSonicBlasterIconClicked` | `SetDropSonicBlasterIconClicked` | `FlashDropSonicBlasterIcon` | `DropBirdScarer` |
| `GetDigIconClicked` | `SetDigIconClicked` | `FlashDigIcon` | `Dig` |
| `GetBuildIconClicked` | `SetBuildIconClicked` | `FlashBuildIcon` | `BuildBuilding` |
| `GetLayPathIconClicked` | `SetLayPathIconClicked` | `FlashLayPathIcon` | `LayPath` |
| `GetPlaceFenceIconClicked` | `SetPlaceFenceIconClicked` | `FlashPlaceFenceIcon` | `PlaceFence` |

**Panel icons — 6** (2 × 3), same contract:
`Get/Set/FlashUpgradeBuildingIcon…` → `Interface_MenuItem_UpgradeBuilding` (`:546`, `:539`, `:552`);
`Get/Set/FlashGoBackIcon…` → `Interface_MenuItem_Back` (`:566`, `:559`, `:572`).

(`FlashCallToArmsIcon` is counted in §4.11 because it is not part of a triple — there is no
`GetCallToArmsIconClicked`; use `GetCallToArmsButtonClicked`.)

### 4.22 Priorities and training — 3

| Name | Arity | Body | Behaviour |
| --- | --- | --- | --- |
| `SetCrystalPriority` | `ARGS_1_NORETURN` | `:350` | `AITask_Game_SetPriorityOff(AI_Priority_Crystal, !arg)` — note the inversion: `1` turns crystal collection **on** |
| `GetTrainFlags` | `ARGS_0` | `:2165` | `objectGlobs.NERPs_TrainFlags` (`Object.h:465`) |
| `SetTrainFlags` | `ARGS_1_NORETURN` | `:2171` | overwrite that ability mask. Also OR-ed into by `Object.cpp:3847`. |

### 4.23 Count check

4 + 32 + 8 + 4 + 7 + 8 + 9 + 3 + 7 + 4 + 17 + 17 + 11 + 4 + 7 + 6 + 5 + 38 + 9 + 9 + 81 + 3 = **293**.
Verified by partitioning the 293 names extracted from `interop.cpp:3181-3506`; zero leftovers, zero
duplicates.

---

## 5. The assembler specification

### 5.1 Design principle: transliteration, not compilation

The assembler performs **no** reordering, **no** precedence insertion, and **no** implicit
instructions. One source token becomes one instruction word, except that a label definition becomes
one `Label` word and a jump becomes one `Jump` word. There is no register allocator, no expression
tree, no peephole pass.

That is deliberate on a project that cannot run the game. A one-to-one lowering has exactly one
failure mode — a wrong opcode or operand — and both are checkable by eye against §2. Anything
smarter would introduce semantics we could never test.

The disassembler is the exact inverse, and round-tripping stock campaign scripts through
disassemble → assemble → byte-compare is the acceptance test that does not require running the game.

### 5.2 Lexical rules

* One statement per line. Tokens are separated by whitespace (spaces or tabs).
* `;` begins a comment that runs to end of line. Comments may follow a statement.
* Blank lines and comment-only lines emit nothing.
* Both LF and CRLF are accepted (unlike the objective-text parser, `Objective.cpp:167`).
* **Identifiers and operators are matched case-insensitively**, mirroring `::_stricmp` at
  `NERPsFile.cpp:83`. Label names are likewise case-insensitive, so `Loop:` and `:loop` match.
* Integers: decimal (`123`), optional leading `-`, or hex (`0x1F`).

### 5.3 Grammar (BNF)

```bnf
<program>        ::= { <line> }
<line>           ::= [ <statement> ] [ <comment> ] <newline>
<comment>        ::= ";" { <any-char-but-newline> }

<statement>      ::= <label-def> | <jump> | <expr-stmt>

<label-def>      ::= <identifier> ":"          ; trailing colon: defines
<jump>           ::= ":" <identifier>          ; leading  colon: jumps to

<expr-stmt>      ::= <term> { <term> }
<term>           ::= <operator> | <literal> | <call>

<operator>       ::= "+" | "#" | "/" | "\" | "?" | ">" | "<" | "=" | ">=" | "<=" | "!="

<literal>        ::= <integer>                 ; statement position: -32768 .. 32767
<call>           ::= <identifier> { <argument> }
                     ; the count of <argument> is exactly arity(<identifier>),
                     ; read from c_nerpsFunctions[id].arguments at assemble time
<argument>       ::= <integer>                 ; argument position: 0 .. 65535
                   | <identifier>              ; must resolve to a NERPS_ARGS_0 built-in

<integer>        ::= [ "-" ] <digit> { <digit> } | "0x" <hexdigit> { <hexdigit> }
<identifier>     ::= <letter> { <letter> | <digit> | "_" }
```

**The grammar is not context-free.** `<call>` cannot be parsed without the arity of the identifier,
which comes from the live table. That is the same property §3.3 sells as a feature: the instruction
set is discovered, not declared.

### 5.4 Lowering

| Source | Emitted word(s) | Rule |
| --- | --- | --- |
| integer `N` (statement position) | `{ opcode 0x0000, operand (uint16)N }` | `NERPsFile.cpp:593` sign-extends on read; range-check −32768…32767 |
| operator `tok` | `{ 0x0001, NERPs_FindOperatorID(tok) }` | `NERPsFile.cpp:529` |
| call `F a1 … an` | `{ 0x0002, id(F) }`, then one word per argument | `NERPsFile.cpp:381`, `:419` |
| — argument integer `A` | `{ 0x0000, (uint16)A }` | passed as raw DWORD, range-check 0…65535 |
| — argument built-in `G` | `{ 0x0002, id(G) }` | substituted by `LoadLiteral`, `NERPsFile.cpp:336-341`; **`G` must be `NERPS_ARGS_0`** |
| `Name:` | `{ 0x0004, own instruction index }` | operand is never read (`NERPsFile.cpp:576-580`); storing the index makes the disassembly self-checking |
| `:Name` | `{ 0x0008, index of `Name`'s Label word }` | execution resumes at that index + 1 (`NERPsFile.cpp:366`, `:584`) |

Two passes:

1. Emit everything. Record each `Name:` → instruction index in a table. Emit each `:Name` with
   operand 0 and record a fixup `{word index, name, source line}`.
2. Resolve fixups. An unresolved name is a **warn-and-skip** error (§7) — patch the operand to point
   at the jump's own index, which makes the jump a self-loop only if `regA` is true; safer still,
   rewrite the whole word to `{0x0000, 0}`, a harmless literal-0 load. **Take the second option.**

### 5.5 Worked encodings

`SetR2 5` — the one call whose id we can state from our own source
(`NERPsFile.cpp:392`: id 27 = `0x1b`):

```
1B 00 02 00      Function 0x001B   ; SetR2, arity ARGS_1_NORETURN
05 00 00 00      Load     0x0005   ; argument, raw DWORD 0x00000005 -> 5
```

`SetR2 SetR2` — the interpreter's own worked example of the argument trap
(`NERPsFile.cpp:391-392`):

```
1B 00 02 00      Function 0x001B
1B 00 02 00      Function 0x001B   ; NOT a call. Raw DWORD 0x0002001B = 131099.
```

R2 receives 131099. The assembler must reject this at source level, because `SetR2` is not
`NERPS_ARGS_0`.

`GetR0 >= 3 ? Stop`:

```
xx xx 02 00      Function id(GetR0)   ; ARGS_0 -> regA = R0,  nextCmp = And
08 00 01 00      Operator 8           ; ">=",  nextCmp = Cge
03 00 00 00      Load     3           ; regA = (R0 >= 3),     nextCmp = And
04 00 01 00      Operator 4           ; "?",   inert
00 00 02 00      Function 0           ; Stop,  fires iff regA (currCmp is And, not None)
```

A label and a jump, where `Loop:` landed at instruction index 12:

```
0C 00 04 00      Label  0x000C        ; operand ignored by the runtime
...
0C 00 08 00      Jump   0x000C        ; if regA: instrIdx = 12, loop increments to 13
```

### 5.6 Idioms every author needs

**Unconditional action** (see §1.3):

```
True ? SetMessagePermit 0
```

**Run-once initialisation**, using R7 as the sentinel. Order matters — the latch must be last:

```
GetR7 = 0 ? SetMessagePermit 0
GetR7 = 0 ? DisallowAll
GetR7 = 0 ? SetR7 1          ; latch LAST, or the lines below it never run
```

**Long intervals**, working around the 32767 comparison ceiling (§2.6). Count 30-second ticks in a
register instead of comparing a big number:

```
GetTimer0 > 30000 ? AddR1 1
GetTimer0 > 30000 ? SetTimer0 0
GetR1 >= 2 ? SetMessage 5 0   ; fires at 60 s
```

**Negative numbers.** Argument position cannot express them at all (§2.6). Build them:

```
True ? SetR0 0
True ? SubR0 1               ; R0 == -1
GetR0 > 32767 ? …            ; "R0 is negative", because comparisons are unsigned
```

**Register-to-register move**, using the `ARGS_0` substitution:

```
True ? SetR3 GetR0           ; R3 = R0
```

**Consume a click.** Icon `Get…` returns a count that persists; clear it or the branch re-fires
every tick:

```
GetDigIconClicked > 0 ? SetMessage 4 0
GetDigIconClicked > 0 ? SetDigIconClicked 0
```

### 5.7 Worked example — an escalating wave director

```
; wave-director.npl
; R0 = waves spawned. T0 = ms since the last spawn.
; First wave at 30 s, then every 30 s, four waves total, then stop.
;
; Block coordinates for SetRockMonster are 1-BASED (NERPsFunctions.cpp:511-512),
; so this spawns on map block (12, 9).

GetR0 >= 4 ? Stop              ; all waves sent: cheap early-out, every tick from now on

GetTimer0 < 30000 ? Stop       ; not time yet

True ? SetRockMonster 13 10    ; time is up
True ? SetTimer0 0
True ? AddR0 1
True ? SetMessage 7 0          ; "Seismic activity detected."
```

Trace of one firing tick, against §1.4:

| # | Instruction | `regA` after | `nextCmp` |
| ---: | --- | --- | --- |
| 0 | `GetR0` | R0 | `And` |
| 1 | `>=` | R0 | `Cge` |
| 2 | `4` | `R0 >= 4` → 0 | `And` |
| 3 | `?` | 0 | `And` |
| 4 | `Stop` | does not fire (`regA` 0, `currCmp` ≠ `None`); falls through, leaves `regA` 0 | `And` |
| 5-9 | `GetTimer0 < 30000 ?` | `T0 < 30000` → 0 | `And` |
| 10 | `Stop` | does not fire | `And` |
| 11 | `True` | 1 | `And` |
| 12 | `?` | 1 | `And` |
| 13 | `SetRockMonster` + 2 args | fires; then `regA = 0` | `None` |
| … | each remaining `True ? Action` | same shape | |

Note instruction 4's fall-through: the assembler must have verified at load time that slot 0's arity
consumes no arguments (§7, check **C7**), because if it did the stream would desynchronise here.

### 5.8 Worked example — a resource objective that actually completes

This is the pattern §6.2 forces. The level's `.cfg` must **not** set `CrystalObjective`; the count is
done in the script instead, and `ObjectiveText` still describes it to the player.

```
; collect25.npl   -- cfg must NOT define CrystalObjective (see NERPS-LANGUAGE.md §6.2)

GetR7 = 0 ? SetMessagePermit 0
GetR7 = 0 ? SetR7 1

; progress nudges, each latched so they fire once
GetCrystalsCurrentlyStored >= 10 + GetR1 = 0 ? SetMessage 2 0
GetCrystalsCurrentlyStored >= 10 + GetR1 = 0 ? SetR1 1

; NOTE the left-fold: this reads as ((stored >= 10) && (GetR1)) == 0.
; That is NOT what we want. See below.
```

⚠ That last pair is **wrong**, and it is wrong in the way §1.4 predicts. `A >= 10 + GetR1 = 0`
folds to `(((A >= 10) && GetR1) == 0)`, which is true whenever R1 is 0 *or* A < 10. Because there is
no grouping, a two-condition guard must be built with a register instead:

```
; correct version
GetR1 != 0 ? Stop                       ; already announced: nothing more to do this tick
GetCrystalsCurrentlyStored >= 10 ? SetMessage 2 0
GetCrystalsCurrentlyStored >= 10 ? SetR1 1

GetCrystalsCurrentlyStored >= 25 ? SetMessage 3 0
GetCrystalsCurrentlyStored >= 25 ? SetLevelCompleted
```

**Rule of thumb: one comparison per statement.** Chain conditions with early-out `Stop`s and
registers, never with `+` between two comparisons.

### 5.9 Worked example — a tutorial beat

```
; tutorial-dig.npl
; R0 is the step counter: 0 = intro, 1 = waiting for the Dig icon, 2 = done.

GetR0 = 0 ? SetMessagePermit 0
GetR0 = 0 ? DisallowAll
GetR0 = 0 ? SetMessage 1 0
GetR0 = 0 ? SetR0 1

GetR0 = 1 ? ClickOnlyIcon
GetR0 = 1 ? FlashDigIcon 1

GetDigIconClicked > 0 ? SetDigIconClicked 0
GetDigIconClicked > 0 ? SetR0 2          ; NOTE: dead. The line above already cleared it.
```

⚠ The last pair shows the ordering hazard: `SetDigIconClicked 0` clears the counter, so the next
statement's `GetDigIconClicked` reads 0 and the step never advances. Latch first, clear last:

```
GetDigIconClicked > 0 ? SetR0 2
GetR0 = 2 ? FlashDigIcon 0
GetR0 = 2 ? SetDigIconClicked 0
GetR0 = 2 ? SetTutorialFlags 0           ; leave tutorial mode
GetR0 = 2 ? SetMessage 2 0
```

### 5.10 Where the assembler hooks in

`NERPsFile_LoadScriptFile` is **OURS** (`NERPsFile.cpp:95`), installed over `0x004530b0`
(`interop.cpp:3107`). It is the only place a script is read. The change is contained to that
function:

```cpp
// NERPsFile.cpp:110 becomes:
    uint32 rawSize = 0;
    void* raw = Gods98::File_LoadBinary(filename, &rawSize);
    if (raw == nullptr)
        return false;

    if (DeepCore::settings.nerpsTextScripts && !NERPs_LooksLikeBytecode(raw, rawSize)) {
        uint32 codeSize = 0;
        NERPsInstruction* code = NERPs_AssembleText((const char*)raw, rawSize, &codeSize);
        Gods98::Mem_Free(raw);
        nerpsfileGlobs.instructions = code;          // may be nullptr -> level loads with no script
        nerpsfileGlobs.scriptSize   = codeSize;      // instrCount = scriptSize / 4 (NERPsFile.cpp:356)
        return (code != nullptr);
    }

    nerpsfileGlobs.instructions = (NERPsInstruction*)raw;
    nerpsfileGlobs.scriptSize   = rawSize;
    return true;
```

Three binding constraints:

1. **The output buffer must come from `Gods98::Mem_Alloc`** (`engine/core/Memory.h:111`), because
   `NERPsFile_Free` releases it with `Gods98::Mem_Free` (`NERPsFile.cpp:297`). A `new[]` or
   `std::vector` buffer would be freed by the wrong allocator.
2. **`scriptSize` must be a multiple of 4**, since the instruction count is `scriptSize / 4`
   (`NERPsFile.cpp:356`).
3. **The setting must default off** so the vanilla path is byte-identical. `DeepCore::Settings`
   (`game/DeepCore.hpp:56`) is the existing home for exactly this kind of gate; every current member
   defaults to `false`/vanilla.

Detection (`NERPs_LooksLikeBytecode`) should be conservative and cheap: the buffer is bytecode if its
length is a non-zero multiple of 4 **and** every word's high half-word is a legal opcode
(`0x0000`–`0x0008`, and at most one bit of `NERPsOpcode::Mask` set). Real ASCII source fails that on
its first word almost always, and any ambiguity resolves in favour of the vanilla path.

---

## 6. The four traps an author must design around

Each is verified against this tree. Each is *preserved 1999 behaviour* that this project has
deliberately not "fixed", so a mission author must design around it rather than wait for it.

### 6.1 A level that shows a briefing advisor can never complete on its own

```
Objective.cpp:1084   bool32 __cdecl LegoRR::Objective_CheckCompleted(Lego_Level* level, OUT bool32* timerStillRunning, real32 elapsed)
Objective.cpp:1086       // No briefing == sandbox or something... WHAT???
Objective.cpp:1087       if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR) {
Objective.cpp:1088           return false;
```

`Objective_CheckCompleted` is the **only** caller of the timer, crystal, ore, construction and
block-objective evaluators — everything below line 1088 (`:1090`-`:1123`). It is called from exactly
one place, `Objective_Update` (`Objective.cpp:981`), which is where a level is completed by its own
declared objectives (`:982`).

And the flag is on by default. `Objective_LoadLevel` sets it unless the level explicitly opts out:

```
Objective.cpp:388   if (!Config_GetBoolOrFalse(config, Config_ID(gameName, levelName, "DontShowObjectiveAdvisor"))) {
Objective.cpp:389       objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR;
```

and sets it again as the fallback when a level declares no flags at all:

```
Objective.cpp:576   if (objectiveGlobs.flags == OBJECTIVE_GLOB_FLAG_NONE) {
Objective.cpp:577       objectiveGlobs.flags = OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR;
```

⇒ **On a normal level, `TimerObjective`, `ConstructionObjective` and `BlockObjective` never fire.**

**Workaround.** Either (a) set `DontShowObjectiveAdvisor  TRUE` in the level's cfg section — which
costs you the briefing advisor — or (b) **do not use the cfg objective keys at all** and end the
level from NERPs with `SetLevelCompleted` / `SetLevelFail`, using `ObjectiveText` purely to describe
the goal to the player. Option (b) is the one to standardise on: it is the only approach that gives
an author full control, it works regardless of the advisor setting, and it is almost certainly what
the 1999 campaign did.

Note the interaction: option (b) is only viable if §6.2 is also respected.

### 6.2 `CrystalObjective` makes `Objective_SetStatus` a permanent no-op

```
Objective.cpp:640   void __cdecl LegoRR::Objective_SetStatus(LevelStatus status)
Objective.cpp:648       // What's the purpose of the OBJECTIVE_GLOB_FLAG_CRYSTAL flag check??
Objective.cpp:650       if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CRYSTAL)
Objective.cpp:651           return;
```

The flag is overloaded. It means **both** "this level has a crystal objective", set at load —

```
Objective.cpp:579   const uint32 crystals = Config_GetIntValue(config, Config_ID(gameName, levelName, "CrystalObjective"));
Objective.cpp:581   Objective_SetCryOreObjectives(level, crystals, ore);
Objective.cpp:584   if (crystals > 0) {
Objective.cpp:585       objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_CRYSTAL;
```

— **and** "this level failed *because of* crystals", set at status time (`Objective.cpp:709`) and
cleared when that failure screen closes (`Objective.cpp:786`). The early return at `:650` was
written for the second meaning and fires for the first.

⇒ **On any level with `CrystalObjective > 0`, `Objective_SetStatus` returns before doing anything.**
That kills the normal completion path at `Objective.cpp:982` *and* all four NERPs outcome built-ins
(§4.5), since every one of them calls `Objective_SetStatus` (`NERPsFunctions.cpp:661`, `:670`,
`:678`, `:685`). The level cannot be won or lost by any route.

**Workaround.** **Never set `CrystalObjective` (or, for symmetry of authoring, `OreObjective`) in a
level you intend to end.** Count crystals in the script with `GetCrystalsCurrentlyStored`
(§4.12) and end the level with `SetLevelCompleted`, exactly as in §5.8. Describe the target in
`ObjectiveText` so the player still sees it.

### 6.3 Changing the mission list blanks every save file

```
FrontEnd.cpp:4727   Front_LevelLink_RunThroughLinks(frontGlobs.startMissionLink, Front_LevelLink_Callback_IncCount, &missionsCount);
FrontEnd.cpp:4735   Gods98::File_Read(saveData, (sizeof(SaveData) - 0x8), 1, file);
FrontEnd.cpp:4738   if (missionsCount != saveData->missionsCount && !readOnly) {
FrontEnd.cpp:4740       Gods98::File_Close(file);
FrontEnd.cpp:4743       Front_Save_WriteSaveFiles(saveIndex, nullptr);   // blank the save
FrontEnd.cpp:4748       std::memset(saveData, 0, sizeof(SaveData));
FrontEnd.cpp:4750       return false;
```

`Front_Save_ReadSaveFile` counts the levels reachable through `LevelLinks` from the start level and
compares that count against the one recorded in the save. A mismatch is treated as corruption, and
the save is **overwritten with a blank one** — not merely rejected. `Front_Save_LoadAllSaveFiles`
(`FrontEnd.cpp:4853`) runs this over every slot at startup, so **adding or removing one mission
destroys all six saves**.

Worse, the count alone is not sufficient: completion is recorded by `setIndex`, the position in the
`NextLevel` chain, while the table is sized by link-reachable count. **Reordering the chain silently
remaps which levels a surviving save thinks are complete.** And tutorials are hard-coded at eight
(`FrontEnd.cpp:4912`, `:4915`); a ninth tutorial gets `levelIndex == 8` and matches neither branch,
so its completion is never recorded at all.

**Workaround for an author:** if you add a mission, tell players their saves will be cleared, and do
it once rather than incrementally. **Workaround for this project:** give the overhaul its own save
namespace. Both save paths are OURS and build the directory inline
(`FrontEnd.cpp:4721`, `:4780`, `:4842`); replacing the literal `"Saves"` with a
`DeepCore::settings` value (default `"Saves"`, so vanilla stays byte-identical) makes mission-list
changes harmless. One wrinkle: `ObjectRecall_LoadRROSFile` (`object/ObjectRecall.cpp:154`, OURS) is
*called* by the EXE `Lego_LoadLevel`, so the `.osf` read path must be redirected inside that function
rather than at the call site, in the same change.

### 6.4 `SetGameCompleted` fails the level

```
NERPsFunctions.cpp:667   sint32 __cdecl LegoRR::NERPFunc__SetGameCompleted(sint32* stack)
NERPsFunctions.cpp:669       if (!(legoGlobs.flags1 & GAME1_LEVELENDING)) {
NERPsFunctions.cpp:670           Objective_SetStatus(LEVELSTATUS_FAILED);
NERPsFunctions.cpp:671       }
NERPsFunctions.cpp:672       return_VOID(1);
```

`LEVELSTATUS_FAILED`, not `LEVELSTATUS_COMPLETE`. The built-in named "SetGameCompleted" **fails the
mission**, guarded only by a check that the level is not already ending
(`GAME1_LEVELENDING == 0x8000000`, `Game.h:128`).

The body is installed into the table slot named `SetGameCompleted` at `interop.cpp:3250`, so this is
what a script calling that name gets today.

One honest discrepancy, worth recording. `NERPsFunctions.h:296-299` labels this an
`ALIAS: NERPFunc__SetLevelCompleted` at `0x00454e30` — but `SetLevelCompleted`'s body
(`NERPsFunctions.cpp:659-663`) is different code, and only `SetLevelCompleted` is hooked directly at
that address (`interop.cpp:3160`). The neighbouring addresses are `0x00454e30` (`SetLevelCompleted`),
`0x00454e40` (`SetLevelFail`), `0x00454e60` (`SetGameFail`), `0x00454e70` (`SetTutorialPointer`) —
leaving `0x00454e50` unaccounted for and exactly the right size for this function. The strong
reading is that `0x00454e50` **is** the real `SetGameCompleted` and the header comment is stale;
the alternative — that the decompilation is wrong — would still leave the shipped DLL behaving as
written. Either way the behaviour to design against is the same. Marked **UNDETERMINED** as to
provenance, **determined** as to effect.

**Workaround.** **Never call `SetGameCompleted`.** Use `SetLevelCompleted` (§4.5), subject to §6.2.
The assembler should emit a warning on any use of the name (§7, check **W3**).

---

## 7. Error handling the assembler must implement

### 7.1 The rule, and why it is not negotiable

**Every malformed input must warn and skip. Nothing may terminate, and nothing may corrupt.**

"Fatal" in this codebase is a *log level*, not a description of the code path — but the fatal path
really does end the process:

```
Errors.h:111    #define Error_FatalF2(b, s, ...)  do { if (Gods98::Error_IsFatalVisible() && (b)) { Gods98::Error_Out(true, (s), __VA_ARGS__); } } while (0)
Errors.h:118    #define Error_Fatal(b, s)         Error_FatalF2((b), "%s(%i): Fatal: %s\n", __FILE__, __LINE__, (s))
Errors.cpp:177      if (ErrFatal) Error_TerminateProgram(msg);
Errors.cpp:26   Gods98::Error_LogLevels Gods98::errorLogLevels = { false, true, true, true, true }; // ... fatalVisible
```

`fatalVisible` defaults to **true**, so `Error_Fatal` in the assembler would kill the game on a typo
in a level file. **The assembler must never call `Error_Fatal`, `Error_FatalF`, `Error_FatalF2`, or
`Config_FatalItemF` (`engine/core/Config.h:348`).**

Use the warning path, which logs and continues and is already the project's precedent
(`game/DeepCore.cpp:24`):

```cpp
// NERPsFile.cpp — OURS.
#define NERPs_WarnF(b, s, ...)  Error_WarnF2((b), "NERPs: %s\n", Gods98::Error_Format((s), __VA_ARGS__))
```

Include the **source line number** in every message. The author is editing a text file; a line number
is the whole difference between a usable and an unusable diagnostic.

### 7.2 The checks

Each check states the condition, the recovery, and why the recovery is safe. "Skip the statement"
always means: emit nothing for it, and continue at the next line — never abandon the file, and never
emit a partial call (a call word without its arguments would desynchronise every following
instruction).

| # | Condition | Recovery | Safety argument |
| ---: | --- | --- | --- |
| **W1** | Unknown identifier — `NERPs_FindFunctionID` returns −1 | Warn `unknown built-in '%s' at line %u`; skip the whole statement | Emitting the statement is impossible without an id; skipping keeps every later index correct because nothing was emitted. |
| **W2** | Unknown operator token | Warn; skip the token, keep the rest of the statement | An operator only sets `nextCmp`; omitting it changes the expression but cannot desynchronise the stream. |
| **W3** | Use of `SetGameCompleted` | Warn `SetGameCompleted FAILS the level (see NERPS-LANGUAGE.md §6.4)`; **still emit it** | The author may genuinely want that behaviour; a warning informs without overriding. |
| **W4** | Too few arguments before end of line for the arity read from the table | Warn `'%s' needs %u argument(s), got %u`; skip the whole statement | Emitting the call word without its arguments would make the interpreter consume the *next* statement's words as arguments — the one failure that silently corrupts everything after it. |
| **W5** | Extra tokens after a call's arguments that are not operators or values | Warn; treat them as ordinary terms of the same expression statement | Matches the language: consecutive values AND together (§1.4). |
| **W6** | Integer out of range in **statement** position (not −32768…32767) | Warn `literal %d does not fit a signed 16-bit operand`; clamp to the nearest bound and emit | §2.6. Clamping is visible and predictable; wrapping silently would produce the `GetTimer0 > 60000` bug. |
| **W7** | Integer out of range in **argument** position (not 0…65535) | Warn; clamp and emit | §2.6, and note the range is *different* here — say so in the message. |
| **W8** | Identifier used as an argument whose table arity is not `NERPS_ARGS_0` | Warn `'%s' cannot be an argument; only 0-argument built-ins can`; substitute a `Load 0` word | This is the `SetR2 SetR2` trap (`NERPsFile.cpp:391-392`). Substituting keeps the argument count correct, which W4's safety argument depends on. |
| **W9** | Duplicate label definition | Warn; keep the **first** definition, ignore the later one | First-wins is deterministic and matches a single forward pass. |
| **W10** | Jump to an undefined label | Warn; rewrite that word to `{0x0000, 0}` — a literal `0` load | A dangling `Jump` with operand 0 would silently restart the program mid-tick, an infinite loop. A literal 0 load is inert: it sets `regA` to 0 (or ANDs it to 0), which is the same thing the jump's own reset does. |
| **W11** | Instruction count would exceed 65535 | Warn `script too large; jump operands are 16-bit`; stop emitting and assemble what fits | Beyond 65535 no label is addressable. |
| **W12** | Assembled program is empty (every statement was skipped, or the file was only comments) | Warn; return `nullptr` and `size = 0` | `NERPsRuntime_Execute` returns immediately on a null `instructions` (`NERPsFile.cpp:348-349`), so the level plays with no script rather than crashing. |
| **W13** | Source contains a byte ≥ 0x80, or an unterminated construct | Warn; skip the line | Cheap guard against a binary file misdetected as text. |

### 7.3 Load-time self-checks against the live table

These verify the environment rather than the source, and they are the payoff of binding to the live
table. Run once, at the first assemble.

| # | Check | Recovery |
| ---: | --- | --- |
| **C1** | `c_nerpsFunctions[i].name != nullptr` for all `i` before any `_stricmp` | Skip null entries. `NERPs_HookFunction` (`NERPsFile.cpp:83`) does **not** null-check, and would fault on a malformed table; ours must. |
| **C2** | `c_nerpsOperators[i] != nullptr` for all 11 | Same. |
| **C3** | Slot 293's arity is `NERPS_END_OF_LIST` | Warn if not — the table is not what we think it is. Do not refuse to assemble. |
| **C4** | `_stricmp(c_nerpsFunctions[0].name, "Stop") == 0` | Warn if not. Confirms `NERPS_FUNCID_STOP == 0` (`NERPsFile.h:47`) against the real table. |
| **C5** | Every arity value is ≤ `NERPS_END_OF_LIST` (7) | Warn and treat an out-of-range arity as "unusable", refusing calls to that slot. Prevents reading past `argsStack[3]` (`NERPsFile.cpp:353`). |
| **C6** | No duplicate names in the table | Warn; first match wins, matching `NERPs_HookFunction`'s loop order. |
| **C7** | **Slot 0's arity consumes no arguments** (`ARGS_0` or `ARGS_0_NORETURN`) | Warn loudly. §2.8 and §5.7 depend on the `Stop` fall-through consuming nothing. This is the one **UNDETERMINED** fact in the whole design, and this check settles it at runtime on the machine that has the exe. |

### 7.4 What must never happen

* **Never a partial call.** See W4 — a call word without its arguments corrupts every later
  instruction, which is the only failure mode here that produces a *plausible-looking but wrong*
  program rather than an obviously broken one.
* **Never a jump into the middle of a call's arguments.** Not currently possible, because labels can
  only be defined between statements, but a future `.org`-style feature would break it.
* **Never a non-multiple-of-4 buffer.** `instrCount = scriptSize / 4` (`NERPsFile.cpp:356`) truncates
  silently.
* **Never a buffer from the wrong allocator.** §5.10 constraint 1.
* **Never `Error_Fatal`.** §7.1.

---

## 8. Decision and ranked plan

**DECISION: build the in-DLL text assembler, gated off by default, and treat this document as its
specification.** The alternative — authoring against an archived external Python tool carrying a
hard-coded id table — is strictly worse on every axis that matters to this project: it can drift
from the executable, it cannot validate arity, it needs a build step, and it produces bytecode nobody
can read back. The assembler removes all four problems with roughly 300 lines in a function we
already own, adds no struct, reimplements no exe function, and touches no address.

Ranked, smallest and safest first:

1. **Table dumper — ~10 lines, zero risk.** Iterate `c_nerpsFunctions`, print `i`, `name`,
   `arguments`. Wire it to an existing debug shortcut. This alone converts the whole of §4's
   "inferred arity" column, plus every **UNDETERMINED** id question and check **C7**, into measured
   fact — on any machine that has the game installed. **Do this first; it is the cheapest possible
   way to falsify this document.**
2. **Disassembler — ~80 lines.** `id → c_nerpsFunctions[id].name`, operand → operator spelling,
   labels reconstructed from jump targets. Run it over the stock campaign's `.npl` files. This is
   how we learn the idioms the 1999 designers actually used, and it is the *exact inverse* of the
   assembler, which gives us a round-trip acceptance test that does not require running the game.
3. **Assembler — ~300 lines in `NERPsFile.cpp`,** gated by a new `DeepCore::Settings` member
   defaulting to `false` (§5.10). Vanilla path stays byte-identical. Implement §7's checks as
   written; W4 and C7 are the two that matter most.
4. **Author the traps into the tooling.** The assembler should warn on `SetGameCompleted` (W3), and
   a level-lint pass should warn when a level defines `CrystalObjective` (§6.2) or leaves
   `DontShowObjectiveAdvisor` unset while declaring `TimerObjective`/`ConstructionObjective`/
   `BlockObjective` (§6.1). Both are pure config reads through code we already own
   (`Objective.cpp:388`, `:579`).
5. **Campaign-scoped save directory** (§6.3). Independent of the assembler, but it is the change
   that makes adding missions safe, so it should land before any new mission ships.
6. **Extending the function table beyond 293 — deferred, and not required.** The exe table has zero
   slack (§3.1), so extension means a DLL-side table dispatched for `funcId >= 294`, with every
   `c_nerpsFunctions[funcId]` access in `NERPsRuntime_Execute` routed through an accessor. One thing
   must be settled first: `NERPsRuntime_LoadLiteral` is implemented but **not installed**
   (`interop.cpp:3116` is commented "internal, no need to hook these"), so the exe's copy at
   `0x004535a0` is still live and indexes `c_nerpsFunctions[operand]` unbounded. Either install it —
   a one-line change to an already-written function — or restrict extended ids to non-`ARGS_0`
   forms, which `LoadLiteral` ignores (`NERPsFile.cpp:336`). **Whether any exe code still reaches
   `0x004535a0` is UNDETERMINED.** 293 built-ins is already a large vocabulary; this can wait.

Nothing above adds a compiler warning, and nothing above requires running the game to verify.
