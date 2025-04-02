#!/usr/bin/env bash

set -u
cd "$(dirname "${BASH_SOURCE[0]}")" || return 1

pass=0
fail=0

test_case_stderr() {
    test_case_full "$1" "$2" "$3" "$2" "$4"
}

test_case() {
    test_case_full "$1" "$2" "$3" "$2" ''
}

test_case_canonical() {
    test_case_full "$1" "$2" "$3" "$4" ''
}

stderr_file=$(mktemp)

call_casectl() {
    local name="$1"
    local arg="$2"
    local stdin="$3"
    local stdout="$4"
    local stderr="$5"

    : >"$stderr_file"

    local got_stdout got_stderr
    got_stdout=$(./casectl "$arg" <<<"$stdin" 2>"$stderr_file")
    got_stderr=$(cat "$stderr_file")

    local ok=0

    if [[ "$stderr" != "$got_stderr" ]]; then
        echo >&2 "❌ failed: $name $arg: stderr"
        echo >&2 "   stdin   : $stdin"
        echo >&2 "   expected: $stderr"
        echo >&2 "   got     : $(cat "$stderr_file")"
        ok=1
    fi
    if [[ "$stdout" != "$got_stdout" ]]; then
        echo >&2 "❌ failed $name $arg: stdout"
        echo >&2 "   stdin   : $stdin"
        echo >&2 "   Expected: $stdout"
        echo >&2 "   Got     : $got_stdout"
        ok=1
    fi

    if ! ((ok)); then
        echo >&2 "✅ $name $arg"
        echo "$got_stdout"
    fi

    return $ok
}

test_case_full() {
    local name="$1"
    local input="$2"
    local expected_encoded="$3"
    local expected_canonical="$4"
    local stderr="$5"

    local got_stdout
    
    if ! got_stdout=$(call_casectl "$1" -l "$input" "$expected_encoded" "$stderr"); then
        ((++fail))
        return
    fi

    if ! call_casectl "$1" -u "$got_stdout" "$expected_canonical" ""; then
        ((++fail))
        return
    fi

    ((++pass))
}

echo "== Running casectl tests =="

test_case "Plain ALLCAPS" \
    "HELLO_WORLD" \
    "helloWorld"

test_case_canonical "CamelCase with _I_D" \
    "GET_ELEMENT_BY_I_D" \
    "getElementByID" \
    "GET_ELEMENT_BY¤ID¤"

test_case "CamelCase with ¤ID¤" \
    "GET_ELEMENT_BY¤ID¤" \
    "getElementByID"

test_case "CamelCase with _¤" \
    "GET_ELEMENT_BY_¤ID¤" \
    "getElementBy_ID"

test_case "Escaped underscore __" \
    "SET__CONFIG" \
    "set_config"

test_case "Literal span ¤...¤" \
    "¤GET_ELEMENT_BY_ID¤" \
    "GET_ELEMENT_BY_ID"

test_case "Mixed escaping" \
    "FETCH_¤URL_WITH_PARAM¤_AND_LOG" \
    "fetch_URL_WITH_PARAM_andLog"

test_case "Literal underscore inside ¤" \
    "¤THIS_IS_LITERAL¤" \
    "THIS_IS_LITERAL"

test_case "Escaped ¤ inside span" \
    "¤HELLO¤¤WORLD¤" \
    "HELLO¤WORLD"

test_case "Multiple _X in a row" \
    "GET_A_B_C_D" \
    "getABCD" \
    "GET¤ABCD¤"

test_case "Escaped _ outside literal" \
    "SOME__THING" \
    "some_thing"

test_case "Empty literal span" \
    "¤¤" \
    "¤"

test_case_stderr "Lone ¤ (unterminated span -> warning)" \
    "START_¤UNFINISHED" \
    "start_UNFINISHED" \
    'casectl: Found an unterminated literal span starting form the last ¤ in input. Did you forget too escape it?'

rm "$stderr_file"

echo
echo "== Results: $pass passed, $fail failed =="
[[ $fail -eq 0 ]] && exit 0 || exit 1
