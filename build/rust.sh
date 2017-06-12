#!/bin/sh

set -ev

cargo build

mv target/debug/fac debug-fac

rm -rf target Cargo.lock
