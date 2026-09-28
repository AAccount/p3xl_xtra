# AArch64 Assembly Conventions

## x vs w
- x19 = 64-bit register
- w19 = lower 32 bits of x19

str x19, [...]

str w19, [...]

## Parameters
For object oriented
- x0 = self pointer to the object
- x1 = first parameter to a function
- x2 = second parameter etc.
- 
For plain, non object oriented, x0 = first parameter and so on.

## Other Registers
- x29 frame pointer
- x30 return address (which address to go back to once this function is done)

# AArch64 Basic Instructions
## str: store register
`str source register [the destination address in this register, #additional offset]`

`str x0 [x1, #16]` treat the value of x1 as a memory address, add 0x16 to it, and copy the value of x0 there
- x0 = 123
- x1 = 0x1000
- memory at 0x1016 = 123

Variants
- strb  → 1 byte
- strh  → 2 bytes
- str   → 4 bytes
- strx  → 8 bytes

## ldr: load reigster 
(the opposite of str)
`ldr destination reigster [the source address in this register, #additional offset]`

`ldr x0 [x1, #16]` treat the value of x1 as a memory address, add 0x16 to it, copy whatever 4 bytes is there to x0
- x1 = 0x1000
- memory at 0x1016 = 123
- 123 is copied into x0

Variants
- ldr: copies 4 or 8 bytes into the destination register. x or w determines how many bytes
- ldrb, ldrsb: copy 1 byte extra bytes are either zeros or padded with the sign bit (s stands for signed)
- ldrh, ldrsh: copy 2 bytes ^^^^
- ldrsw: signed counterpart of ldr

## Telltale Sign of Objects
```asm
add x0, sp, #0x68
str w19, [x0]
strh w20, [x0,#4]
strh w21, [x0,#6]
str w22, [x0,#8]
```

Added sp+0x68 to x0. Assume x0 = 0x1000

Through a series of copying from register to memory, you memory looks like:
- 0x1000 - 0x1003 = w19
- 0x1004 - 0x1005 = w20
- 0x1006 - 0x1007 = w21
- 0x1008 - 0x100a = w22

You filled a contiguous 12 byte chunk. This probably reflects an object of
```
{
	uint32_t firstInt;
	uint16_t firstShort;
	uint16_t secondShort;
	uint32_t secondInt
}
```

Gotchas in objects: pay attention to the offsets more than the instructions

```asm
str  wzr, [x0]
strh w19, [x0,#4]
strb w20, [x0,#6]
str  w21, [x0,#8]
```

- 0x0 - 0x3 4 bytes of wzr
- 0x4 - 0x5 some 2 bytes of w19
- 0x6 - 0x6 some 1 byte of w20
- 0x7 - 0x7 actually skipped
- 0x8 - 0xa 4 bytes of w21

