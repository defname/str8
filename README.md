# str8 - UTF-8 sensitive string library

## Goals

- Minimal overhead
- Fast access to single characters
- Full compatibility with C strings
- Decreased memory allocations
- Maybe providing a string arena for multiple small edits
- Const correctness (so no lazy-caching or anything like this)

## Strategy

- Preserve additional memory for adding strings without reallocation
- Header before the actual string that stores:
    - Size in bytes
    - Capacity available in bytes
    - Length in characters
    - A `checkpoints` list holding the number of characters up to specified positions
- Use different header versions for minimal memory use
- Use a variable size per entry of the checkpoints list

## Header Layout

### Type 0

For strings up to 31 bytes.

```
│ 5 Bits                                    │ 3 Bits      │
┌───────────────────────────────────────────┬─────────────┐
│ size                                      │ Type 0      │
└───────────────────────────────────────────┴─────────────┘
```

### Other Types

For strings larger than 31 bytes, with increasing capacity:

```
                                      Type 1    Type 2    Type 4    Type 8
              ╭ ┌───────────────┐ 
              │ │ checkpoints   │
only used     │ ┆               ┆
if not        ⎨ ┆               ┆
pure ASCII    │ │               │
              │ ├───────────────┤
              │ │ length        │     1 Byte    2 Byte    4 Byte    8 Byte
              ╰ ├───────────────┤
                │ capacity      │     1 Byte    2 Byte    4 Byte    8 Byte
                ├───────────────┤
                │ size          │     1 Byte    2 Byte    4 Byte    8 Byte
                ├───────────────┤
                │ type          │     1 Byte    1 Byte    1 Byte    1 Byte
                └───────────────┘
```

The `type` byte contains multiple pieces of information, interpreted contextually:
- The lowest 3 bits (`byte & 0x07`) always store the string type (`TYPE0`, `TYPE1`, etc.).
- **For `TYPE0` strings:** Bits 3-7 store the string's size.
- **For `TYPE1` and higher strings:** The highest bit (`type & 0x80`) is a flag. If not set, the string is pure ASCII, and the `length` field and `checkpoints` list are omitted to save space.

## Checkpoints List: A Packed, Variable-Size Structure

To maximize memory efficiency, the `checkpoints` list is not a simple array of a fixed type. Instead, it's a highly optimized, packed data structure where the size of each entry is variable.

**The Principle:** The size of an entry is determined by its *position* (index) in the list, not by the overall string's type. For each entry, the smallest possible integer type (`uint16_t`, `uint32_t`, `uint64_t`) is used that can store the maximum possible character count at that checkpoint's byte position.

**How it Works and Transition Points:**

A checkpoint at index `i` corresponds to a byte position of `(i + 1) * CHECKPOINTS_GRANULARITY` (where `CHECKPOINTS_GRANULARITY` is 512 bytes). The character count at this position can never be greater than the byte position itself. This leads to fixed points where the entry size changes:

*   **`uint16_t` Zone:** The maximum value for a `uint16_t` is 65,535. The last checkpoint whose character count can safely be stored in a `uint16_t` is at index 126 (corresponding to byte position `127 * 512 = 65,024`). Therefore, the **first 127 entries** (indices 0-126) of the list are always stored as `uint16_t`.

*   **`uint32_t` Zone:** The 128th entry (index 127) corresponds to byte position `128 * 512 = 65,536`. This character count requires at least a `uint32_t`. All subsequent entries are stored as `uint32_t` until their corresponding byte position exceeds the `uint32_t` limit.

*   **`uint64_t` Zone:** For extremely large strings, entries will eventually be stored as `uint64_t`.

**Layout Example:**

```
<-- 127 entries of 2 Bytes --> | <--  N entries of 4 Bytes  --> | <-- M entries of 8 Bytes -->
┌────┬────┬.............┬──────┬────────┬..............┬────────┬──────────┬...
│ E0 │ E1 │.............│ E126 │  E127  │..............│  EN-1  │    EN    │... 
└────┴────┴.............┴──────┴────────┴..............┴────────┴──────────┴...
```

**Advantages of this Design:**

1.  **Maximum Memory Efficiency:** It uses the absolute minimum required memory for the `checkpoints` list.
2.  **Efficient Reallocation:** When appending to a string, the existing part of the `checkpoints` list can be copied with a single `memcpy`, as its layout is static and does not change when the string's main `type` is promoted.

## Further Tweaks

Use SIMD instructions to analyze the string and build the checkpoints list.

## Interface

```C
typedef char* str8;

str8 str8new(const char *s);
void str8free(str8 s);

size_t str8len(const str8 s);
size_t str8size(const str8 s);
size_t str8cap(const str8 s);

str8 str8append(str8 s1, const char *s2);
....
```