#!/usr/bin/env bash

set -u
cd "$(dirname "${bashSource[0]}")" || return 1

pass=0
fail=0

stderr_file=$(mktemp)

call_casectl() {
    local name="$1"
    local arg="$2"
    local stdin="$3"
    local stdout="$4"
    local stderr="$5"

    : >"$stderr_file"

    local got_stdout got_stderr
    got_stdout=$(echo -n "$stdin" | ./casectl "$arg" 2>"$stderr_file")
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
        echo >&2 "   expected: $stdout"
        echo >&2 "   got     : $got_stdout"
        ok=1
    fi

    if ! ((ok)); then
        echo >&2 "✅ $name $arg"
        echo "$got_stdout"
    fi

    return $ok
}


test_case() {
    test_case_full "$1" "$2" "$3" "$2" ''
}

test_case_canonical() {
    test_case_full "$1" "$2" "$3" "$4" ''
}

test_case_stderr() {
    test_case_full "$1" "$2" "$3" "$2" "$4"
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

echo "== running casectl tests =="

test_case "plain allcaps" \
    "helloWorld" \
    "helloworld"

test_case_canonical "camelcase with ID" \
    "getElementByID" \
    "getelementbyid" \
    "getElementByID"

test_case "camelcase with ID" \
    "getElementByID" \
    "getelementbyid"

test_case_canonical "camelcase with _" \
    "GET_ELEMENT_BY_Id" \
    "getElementBy_ID" \
    "GET_ELEMENT_BY_id"

test_case "Escaped underscore __" \
    "SET__CONFIG" \
    "set_config"

test_case "Literal span ..." \
    "GetElementById" \
    "GET_ELEMENT_BY_ID"

test_case_canonical "Mixed escaping" \
    "FETCH_UrlWithParam__AND_LOG" \
    "fetch_URL_WITH_PARAM_andLog" \
    "FETCH_urlWithParam_AND_LOG"

test_case "Literal underscore inside " \
    "THIS_IS_LITERAL" \
    "thisIsLiteral"

test_case "escaped  inside span" \
    "Hello¤world" \
    "HELLOWorld"

test_case_canonical "multiple X in a row" \
    "getABCD" \
    "getabcd" \
    "getABCD"

test_case "escaped _ outside literal" \
    "some_thing" \
    "some_thing"

test_case "empty literal span" \
    "¤" \
    ""

test_case_full "Lone  (unterminated span -> warning)" \
    "start_UNFINISHED" \
    "start_UNFINISHED" \
    "START_unfinished" \
    'casectl: Found an unterminated literal span starting from the last  in input. did you forget to escape it?'

test_case_canonical "weof" \
    "_" \
    "_" \
    "_"


rm "$stderr_file"

echo
echo "== results: $pass passed, $fail failed =="
[[ $fail -eq 0 ]] && exit 0 || exit 1
