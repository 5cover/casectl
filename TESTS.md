
# Tests

## ✅ Basic Tests

### 1. Plain ALLCAPS input

```txt
INPUT:    HELLO_WORLD
ENCODED:  hello_world
DECODED:  HELLO_WORLD
```

### 2. CamelCase via `_X`

```txt
INPUT:    GET_ELEMENT_BY_I_D
ENCODED:  getElementByID
DECODED:  GET_ELEMENT_BY_I_D
```

### 3. Underscore escaping

```txt
INPUT:    SET__CONFIG
ENCODED:  set_config
DECODED:  SET__CONFIG
```

### 4. Full literal span with `¤`

```txt
INPUT:    ¤GET_ELEMENT_BY_ID¤
ENCODED:  GET_ELEMENT_BY_ID
DECODED:  ¤GET_ELEMENT_BY_ID¤
```

## 💀 Edge Case Tests

### 5. Mixed escaping

```txt
INPUT:    FETCH_¤URL_WITH_PARAM¤_AND_LOG
ENCODED:  fetchUrlWithParamAndLog
DECODED:  FETCH_¤URL_WITH_PARAM¤_AND_LOG
```

### 6. Literal underscore inside a span

```txt
INPUT:    ¤THIS_IS_LITERAL¤
ENCODED:  THIS_IS_LITERAL
DECODED:  ¤THIS_IS_LITERAL¤
```

### 7. Escaped `¤` inside a literal

```txt
INPUT:    ¤HELLO¤¤WORLD¤
ENCODED:  hello¤world
DECODED:  ¤HELLO¤¤WORLD¤
```

### 8. Escaped `_` outside literal

```txt
INPUT:    SOME__THING
ENCODED:  some_thing
DECODED:  SOME__THING
```

## 🧪 Evil Test Cases

### 9. Multiple single-letter `_X` sequences

```txt
INPUT:    GET_A_B_C_D
ENCODED:  getABCD
DECODED:  GET_A_B_C_D
```

### 10. Underscore before non-uppercase (invalid `_x`)

```txt
INPUT:    GET_x
ENCODED:  get_x     # `_x` not a valid escape
DECODED:  GET_X     # decoding may need to assume `X`
```

### 11. Lone `¤` (unterminated span)

```txt
INPUT:    START_¤UNFINISHED
ENCODED:  start¤unfinished
DECODED:  START_¤UNFINISHED
```

> You may choose to treat this as valid (open span until EOF) or throw an error/warning.

## 🧹 Cleanup / Meta

### 12. Empty literal span

```txt
INPUT:    ¤¤
ENCODED:  (empty string)
DECODED:  ¤¤
```
