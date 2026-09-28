# Object functions and variables are separated.

## Basic Case
```C++
class Something 
{
public:
    int a;
    int b;

    void doSomething();
};
```
For basic classes like Something, int a and b are store together in 1 chunk of memory.
```
0x0 - 0x3 = a
0x4 = 0x7 = b
```

Assume Something is stored at 0x1000.

When calling `something.doSomething()`, it is more like `Something::doSomething(&itself)`

The class is treated more like this C code

```C
struct Something
{
	int a;
	int b;
};

void Something_doSomething(struct Something* itself)
{
	// whatever it does
}
```
This is reflected in the Ghidra C slop of `void __thiscall IzatApiV02::injectXtraData(IzatApiV02 *this, char *param_1, uint param_2)`.

## Inheritance

```C++
class Base 
{
public:
	int i;
	short s;
	virtual void a();
};

class Derived : public Base 
{
public:
	int extra;
	void a() override;
};

Base* derived = new Derived();
derived->a();
```
`a()` is virtual in Base which means, you should be using `Derived::a(&itself)`

For the derived object to know this, its memory layout gets an extra VTable entry before all the variables:

```C
struct Derived
{
	char** vtable; // 0x0 - 0x7 
	int i; // 0x8 - 0xa
	short s; // 0xb - 0xc
	byte[2] padding; // 0xd - 0xf
	int extra // 0x10
};
```

The VTable is just a list (not an actual table) with each 8 byte spot representing the offset to a function of derived. In this case the vtable is short with just 1 entry for funciton a.
The assembly tell tale sign of using a vtable to find the "correct" version of a function (virtual dispatch)

```asm
ldr x8, [x0]
ldr x9, [x8, #0x10]
blr x9
```

x0 is the address of some vtable, store that in x8
store in x9, x8 + 0x10 (16)
branch link reigster x9: jump to whatever address is in x9, and start running that stuff
You can also guess there are 2 other functions at x8, x8 + 8

# Constructors
## Inheritance
```asm
adrp x8, 0x10000
add  x8, x8, #0x123
str  x8, [x0]
```
- `adrp`: loads 4kb page region starting with the first 12 digits of 0x10000 into "some register"
- `add` "some register", (itself), offset: get the exact spot in that page. You're looking for the exact address of some class's vtable
- store the vtable pointer at the very beginning of "self" (x0 for objects)

## Simple class
Suppose the constructor contains:

```asm
mov  w8, #0
str  w8, [x0,#0x10]
str  x1, [x0,#0x18]
```

This is strong evidence that the object contains something like:

```cpp
this->field_10 = 0;
this->field_18 = argument1;
```

Why?

Because construction is when object members are normally initialized.

If you see a bunch of stores into:

```text
this + 0x00
this + 0x08
this + 0x10
this + 0x18
...
```

you can start reconstructing the object's layout.

That is often easier than trying to understand a random method first.

# Examples
## `obj->somefunction()`
```asm
ldr x8, [x0]
ldr x9, [x8,#0x18]
blr x9
```

If you assume x0 is a pointer to some object, you are treating the first 8 bytes of that object as an address, then adding 0x18 to it. Lastly, you run wherever that is.
`obj->somefunction()`

## `obj->innerObj->(something in the inner object)`
```asm
ldr x8, [x0,#0x58]
ldr x0, [x8,#0x08]
```
- 1st line: same as above, you treat the first 8 bytes of the object at x0 as a memory address then load that + 0x58: `obj->innerObject()`
- 2nd line: treat the first 8 bytes of the 2nd object as a memory address again and load that + 0x8: obj2->(something inside).
You don't know what that inside thing at x8 + 0x58 is without seeing how it is used later.

In both these examples immediately treating the beginning of an object as a memory address, this strongly hints this address is the vtable since it is at the very beginning of the object.

As for parameters, you'll need to check what was in x(w)1, x(w)2, etc.

## `obj->function(something_8_bytes a, int b)`
```asm
ldr x8, [x0]
ldr x9, [x8,#0x20]
mov x1, x20
mov w2, w21
blr x9
```
- Treat the memory address at x0 as a pointer.
- Take that pointer = 0x20 and save that to x9
- Move stuff from w20 and w21 into the traditional parmameter 1 and 2
- Run the address in x9: `obj->function(something_8_bytes a, int b)`

Because aarch64 is 64 bytes, you don't know if the 8 bytes in a is a pointer or a long

Note x8, and x9, and even x0 don't mean anything. x1, x(w)2 DO mean something in the last example because it is lining up parameters according to convention.

# Typical patterns

(Direct copy and paste examples from Chat GPT.)

When you see:

```asm
ldr x8, [x0]
ldr x9, [x8,#offset]
blr x9
```

think:

> **potential virtual call**

When you see:

```asm
ldr x8, [x0,#offset1]
ldr x0, [x8,#offset2]
```

think:

> **nested object/member pointer dereference**

When you see:

```asm
mov x0, ...
mov x1, ...
mov x2, ...
bl something
```

think:

> **argument preparation followed by a normal function call**


# Connecting the dots example from ChatGPT
Consider 2 assembly snipets near each other.
```asm
ldr x8, [x0]
ldr x9, [x8,#0x20]
blr x9
```

in one place, and:

```asm
ldr x8, [x0]
ldr x9, [x8,#0x28]
blr x9
```
You can see what looks like a vtable running function5 and function6

# x0's source

```asm
ldr x8, [x0]
ldr x9, [x8,#0x20]
mov x1, x20
mov w2, w21
blr x9
```
In this AArch64 snippet, you are looking at an indirect function call (`blr x9`), which is commonly how **virtual method calls** or function pointers are implemented in compiled languages like C++.

Here, **`x0` acts as the `this` pointer** (the object instance) passed into the function being called, or the primary argument. How `x0` is produced (`mov` vs. `ldr`) tells you fundamentally different things about where that object comes from.

---

### 1. When `x0` is produced via `mov x0, x19` (Register Forwarding)

* **Direct Copy:** The value already exists in another register (`x19`), and it is simply being copied into `x0`.
* **Implication:** The current function is **forwarding an existing reference** it received. For example, `x19` might hold the `this` pointer passed into the *current* function, and it is being passed along unchanged to the *next* function call.
* **Analysis Takeaway:** No new memory lookups are happening; the object reference is just being passed down the call chain.

---

### 2. When `x0` is produced via `ldr x0, [x27,#...]` (Memory Load)

* **Dereferencing Memory:** The value for `x0` is being **loaded from memory** relative to a base register (`x27`, which often points to a global offset table, a base structure, or a context block).
* **Implication:** The program is fetching a stored reference—such as a **member variable** of the current object, a global variable, or an entry from a lookup table.
* **Analysis Takeaway:** The function is operating on a *different* object or sub-component stored in memory, rather than just passing along its own input argument.

---

### Summary

Looking at the source of `x0` helps you track data lineage. A `mov` means you are chaining or passing existing pointers, while an `ldr` means you are navigating a data structure (like fetching an object field) before making the call.

### ChatGPT callback single example
Here's the twist.

This C code:

```cpp
using Callback = void (*)(int);

Callback cb = ...;
cb(42);
```

can also produce:

```asm
mov w0, #42
blr x9
```

So:

> **`blr` does not mean "virtual function."**

It means:

> **"call the address contained in this register."**

Possible reasons include:

```text
virtual dispatch
function pointer
callback
jump table
interface dispatch
compiler-generated machinery
```

Context tells you which.

That's a very important lesson.

# ChatGPT Handles

A **handle** is deliberately opaque.

For example:

C++
```cpp
using LocClientHandle = void*;
```
C
```C
typedef void* LocClientHandle;
```
The program might treat it like:

```cpp
LocClientHandle h;
```

without caring what the pointed-to structure looks like.

That is extremely common in C APIs.

And you'll see things such as:

```asm
ldr x0, [x20,#0x08]
bl some_api
```

where the loaded value isn't necessarily "an object member" in the C++ sense.

It may simply be:

```cpp
some_api(this->handle);
```

That's why our earlier Qualcomm expression:

```c
*(undefined8 *)(*(long *)(this + 0x58) + 8)
```

needs to be interpreted cautiously.

It might be:

```cpp
this->client->handle
```

or:

```cpp
this->context->opaqueHandle
```

or something else entirely.

The machine code tells us **how the data is used**, but names and source-level abstractions have to be earned.

# How to tell a pointer from an ordinary integer

This is one of the most useful practical tricks.

Suppose you see:

```asm
ldr x8, [x0,#0x18]
```

That only tells us it's an 8-byte value.

It might be:

```text
uint64_t
pointer
file descriptor + something weird
timestamp
function address
handle
```

Then look at what happens next.

### If we see:

```asm
ldr x9, [x8,#0x10]
```

that strongly suggests `x8` is a pointer to another object/structure.

Why?

Because we're dereferencing it again.

Conceptually:

```cpp
x8 = obj->field;
x9 = x8->field2;
```

### If instead:

```asm
add x9, x8, #123
```

we can't conclude it's a pointer.

### If:

```asm
blr x8
```

then we're treating `x8` as a **code address**, suggesting:

```text
function pointer
```

or virtual dispatch/etc.

The operation performed on a value tells you much more than its width.

# Object layout vs vtables

Let's keep these firmly separated:

## Object layout

```text
object + offset → member
```

Example:

```asm
ldr x8, [x0,#0x58]
```

Think:

```cpp
x8 = this->member;
```

## Vtable dispatch

```text
object
  ↓ first pointer
vtable
  ↓ slot
function pointer
  ↓
call
```

Example:

```asm
ldr x8, [x0]
ldr x9, [x8,#0x20]
blr x9
```

Think:

```cpp
this->virtualMethod(...);
```

The first one accesses **data**.

The second accesses **code through a table**.
