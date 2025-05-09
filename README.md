# casectl

> [!note]
> This document is maintained in casectl uppercase format. A git filter is used to decode it into the version you are currently seeing.

casectl is a program that changes the case of text.

Reads from standard input, writes to standard output.

Supports Unicode.

## Options

casectl takes a single argument that determines the transformation to apply.

Option|Description
-|-
`-l, --lower`|lowercase input
`-u, --upper`|uppercase input
`-i[SUFFIX], --in-place[=SUFFIX]`|edit files in place (makes backup if SUFFIX supplied)

## 🔤 `casectl` Escaping Rules Summary

### Base behavior

- The entire file is **lowercased**.
- You can escape or preserve capitalization using `_` or `¤`.

### 1. `_x` &rarr; preserve `X` as uppercase

- Underscore followed by **a single uppercase letter** is preserved verbatim.
- Example:  

```text
GET_ELEMENT_BY_I_D -> getElementByID
```

- To insert a literal underscore: `__`

```text
SET__CONFIG -> set_config
```

### 2. `¤...¤` &rarr; literal span

- Characters inside `¤`...`¤` are **preserved exactly**.
- Nothing is interpreted inside—**underscores, capitals, whatever** are all literal.
- Example:

```text
FETCH¤_URL_WITH_PARAM_¤AND__LOG -> fetch_URL_WITH_PARAM_and_log
```

- You can escape a literal `¤` inside a span with `¤¤` (like SQL-style quote doubling):

```text
¤A¤¤B¤ -> a¤b
```

- `_` has no special meaning inside a `¤...¤` block:

```text
¤HELLO_WORLD¤ -> HELLO_WORLD
```

- **Nesting is not allowed**. A `¤` always opens or closes the nearest span.

### 3. Summary of Escapes

| Input            | Output           | Meaning                              |
|-------------------|-------------------|---------------------------------------|
| `_x`             | `X`              | Preserve a single capital            |
| `__`            | `_`              | Literal underscore                   |
| `¤...¤`         | `...`             | Preserve span exactly                |
| `¤¤` inside span| `¤`              | Literal currency sign inside span    |

## roadmap

- file arguments
- `-i`, `--in-place` option
- `-f`, `--format` option: canonicalize (equivalent to lowercase + uppercase)

## changelog

### 1.0.1

- file arguments
- in-place transformation

### 1.0.0

initial release
