# This script takes care of testing your crate

set -ex

main() {
    git --version
    pwd
    git pull --tags
    git describe --dirty || true
    git rev-parse HEAD

    rustup install $TARGET
    cargo build --target $TARGET
    cargo build --target $TARGET --release

    if [ ! -z $DISABLE_TESTS ]; then
        return
    fi

    cargo test --target $TARGET
    cargo test --target $TARGET --release

    # cross run --target $TARGET
    # cross run --target $TARGET --release
}

if [ -z $TRAVIS_TAG ]; then
    main
fi
