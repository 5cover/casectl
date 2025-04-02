# casectl

casectl is a program that changes the case of text.

Reads from standard input, writes to standard output.

Supports Unicode.

## Arguments

casectl takes a single argument that determines the transformation to apply.

Argument|
`L`, `l`|Lowercase input
`U`, `u`|Uppercase input

## 🔤 `casectl` Escaping Rules Summary

### Base behavior

- The entire file is **lowercased**.
- You can escape or preserve capitalization using `_` or `¤`.

### 1. `_X` &rarr; preserve `X` as uppercase

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
FETCH_¤URL_WITH_PARAM¤_AND_LOG -> fetchUrlWithParamAndLog
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

| Input           | Output           | Meaning                              |
|----------------|------------------|--------------------------------------|
| `_X`            | `X`              | Preserve a single capital            |
| `__`            | `_`              | Literal underscore                   |
| `¤...¤`         | `...`            | Preserve span exactly                |
| `¤¤` inside span| `¤`              | Literal currency sign inside span    |
