# Symbol Tables
## "standard" and dynamic
The Standard Symbol Table `(.symtab)`: Contains every symbol compiled into the file—local functions, static variables, helper functions, file names, and global/external symbols. 
This table is heavy, not needed at runtime, and is usually stripped from production binaries to save space and obscure internal code structure.

The Dynamic Symbol Table `(.dynsym)`: Contains only the subset of symbols needed for dynamic linking (functions and variables imported from shared libraries, or exported so other binaries can link to them). 
This table cannot be stripped from dynamically linked binaries or shared libraries because the operating system's dynamic linker (ld.so) needs it at runtime to resolve dependencies.


## Relocation Tables
Think of a relocation as a placeholder for any address that cannot be hardcoded at compile time, which includes both external dependencies and internal code/data that needs to be position-independent.

Example:
```
$ aarch64-linux-gnu-readelf --symbols libexample.so | grep "type_a"
Num:    Value          Size Type    Bind   Vis      Ndx Name
    42: 0000000000018f20    32 OBJECT  GLOBAL DEFAULT   16 vtable for type_a

$ aarch64-linux-gnu-readelf --relocs libexample.so
Relocation section '.rela.dyn' at offset 0x3d0 contains 15 entries:
  Offset          Info------------ Type------------- Sym. Value---- Sym. Name + Addend
  00000000018f20  000000000403 R_AARCH64_RELATIVE                    000000000000a1fc
  00000000018f28  000000000403 R_AARCH64_RELATIVE                    000000000000a350
  00000000018f30  000dc0000401 R_AARCH64_ABS64    0000000000021000 other_class::foo()
```
If you assume the vtable has 2 function f1 and f2, and assume the library was loaded into 0x10000
- f1's "real" address will be 0x10000 + 0xa1fc = 0x1a1fc
- f2's "real" address will be 0x10000 + 0xa350 = 0xa350

(f2 is NOT really located at 0x1000 + 0xa1fc + (2-1)*8.)

The entries in the relocation table are 1:1 of original address -> real address offset. 
Real address offset is added to where the library or executable is loaded to.

You can continue to find f3 and so on assuming there are entries in the relocation table. 
If there is no entry for what should've been f3, you can ??probably?? assume there is no f3. 

Telltale sign of a function when you get to `0xa1fc` for f1 in this example
```asm
stp x29, x30, [sp, #-16]!
```

store pair x29 (stack frame pointer), AND x30 (return adddress) into the stack pointer, move it DOWN 16 bytes (2*8byte addresses)

