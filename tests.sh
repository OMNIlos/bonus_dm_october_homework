#!/bin/sh
set -eu

check() {
    result=$(./bonus "$1" "$2" 3)
    case "$result" in
        "$3"*) ;;
        *) echo "Ошибка: $1 == $2"; exit 1 ;;
    esac
}

check '~(A|B)' '~A&~B' 'ЗАКОН'
check 'A&(B|C)' '(A&B)|(A&C)' 'ЗАКОН'
check '~~A' 'A' 'ЗАКОН'
check 'A\B' 'A' 'ОПЕЧАТКА'
check 'A^B' 'B^A' 'ЗАКОН'
check '(A\B)\C' 'A\(B\C)' 'ОПЕЧАТКА'

case "$(./bonus --slow 'A\B' 'A' 3)" in
    'ОПЕЧАТКА'*) ;;
    *) echo 'Ошибка полного перебора'; exit 1 ;;
esac

if ./bonus 'A)' 'A' 3 >/dev/null 2>&1; then
    echo 'Ошибка парсера'
    exit 1
fi

echo 'Тесты пройдены'
