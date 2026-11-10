#!/bin/sh
# Compare output and exit status against the platform's cat and grep.
set -u
base=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/tests.XXXXXX") || exit 1
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
suite=${1:-all}
passed=0
failed=0
printf 'Alpha one\n\n\nbeta\tTWO\nend\n' > "$tmp/a"
printf 'beta\nAlpha\nwithout newline' > "$tmp/b"
printf '\n\nlast\n' > "$tmp/c"
printf 'tail' > "$tmp/partial"
printf '\nnext\n' > "$tmp/continuation"
printf 'tab\tcontrol\001\177\r\n' > "$tmp/control"
: > "$tmp/empty"

compare() {
  utility=$1
  shift
  if [ "$utility" = cat ] && [ "${cat_reference:-busybox}" = busybox ]; then
    busybox cat "$@" > "$tmp/expected" 2> "$tmp/reference-error"
  else
    "$utility" "$@" > "$tmp/expected" 2> "$tmp/reference-error"
  fi
  expected_status=$?
  "$base/$utility/$utility" "$@" > "$tmp/actual" 2> "$tmp/actual-error"
  actual_status=$?
  if [ "$expected_status" -eq "$actual_status" ] && cmp -s "$tmp/expected" "$tmp/actual"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    printf 'FAIL: %s' "$utility" >&2
    printf ' <%s>' "$@" >&2
    printf ' (status %s vs %s)\n' "$expected_status" "$actual_status" >&2
    diff -u "$tmp/expected" "$tmp/actual" >&2 || :
  fi
}

if [ "$suite" = all ] || [ "$suite" = cat ]; then
  compare cat "$tmp/a"
  for flag in -b -e -n -s -t -v -E -T; do
    cat_reference=busybox
    case "$flag" in -s|-E|-T) cat_reference=gnu ;; esac
    if [ "$cat_reference" = busybox ]; then
      compare cat "$flag" "$tmp/a" "$tmp/b" "$tmp/c"
      compare cat "$flag" "$tmp/partial" "$tmp/continuation"
    else
      compare cat "$flag" "$tmp/a" "$tmp/c"
    fi
    compare cat "$flag" "$tmp/control"
    compare cat "$flag" "$tmp/empty"
  done
  for flag in -bn -ns -et -en -tn -vn -eb -tb -b -s; do
    cat_reference=busybox
    case "$flag" in *s*) cat_reference=gnu ;; esac
    compare cat "$flag" "$tmp/a" "$tmp/c"
    case "$flag" in -en|-tn|-vn|-eb|-tb) compare cat "$flag" "$tmp/b" ;; esac
  done
  cat_reference=busybox
  compare cat "$tmp/missing" "$tmp/a"
  compare cat -- "$tmp/a"
  # Long GNU names are required even when the reference is BusyBox.
  for pair in 'number-nonblank b' 'number n' 'squeeze-blank s'; do
    set -- $pair
    cat "-$2" "$tmp/a" > "$tmp/expected"
    "$base/cat/cat" "--$1" "$tmp/a" > "$tmp/actual"
    if cmp -s "$tmp/expected" "$tmp/actual"; then passed=$((passed + 1)); else
      failed=$((failed + 1)); printf 'FAIL: cat --%s\n' "$1" >&2
    fi
  done
fi

if [ "$suite" = all ] || [ "$suite" = grep ]; then
  for pattern in 'Alpha' 'beta' '^$' '^Alpha' 'end$' '[ab][el]' 'absent' ''; do
    compare grep "$pattern" "$tmp/a"
    compare grep "$pattern" "$tmp/a" "$tmp/b" "$tmp/empty"
  done
  for flag in -i -v -c -l -n; do
    for pattern in 'alpha' '^$' 'absent' ''; do
      compare grep "$flag" "$pattern" "$tmp/a"
      compare grep "$flag" "$pattern" "$tmp/a" "$tmp/b" "$tmp/empty"
    done
  done
  compare grep -e Alpha "$tmp/a" "$tmp/b"
  compare grep -e Alpha -e beta "$tmp/a" "$tmp/b"
  compare grep -e '' "$tmp/a"
  compare grep '[invalid' "$tmp/a"
  compare grep Alpha "$tmp/missing" "$tmp/a"
  compare grep Alpha "$tmp/empty"
  compare grep -- '-pattern' "$tmp/a"
fi
printf 'Integration: %s passed, %s failed\n' "$passed" "$failed"
[ "$failed" -eq 0 ]
