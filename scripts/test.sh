#!/bin/bash

function foo {
    echo "FOO=$FOO"
    scripts/test2.sh
}

foo
FOO=x foo
foo
