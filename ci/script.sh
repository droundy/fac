# This script takes care of testing your crate

set -ex

main() {
    cross build --target $TARGET
    cross build --target $TARGET --release

    if [ ! -z $DISABLE_TESTS ]; then
        return
    fi

    cross test --target $TARGET
    cross test --target $TARGET --release

    # cross run --target $TARGET
    # cross run --target $TARGET --release
}

if [ -z $TRAVIS_TAG ]; then
    main
fi
