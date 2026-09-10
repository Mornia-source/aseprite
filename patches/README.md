# Submodule patches

`src/psd` is a git submodule pointing at `https://github.com/aseprite/psd.git`,
which we cannot push to. Our fix lives as a local-only commit there, so the SHA
recorded by the parent repository does not exist on any remote: a fresh clone,
or `git submodule update`, will not be able to check it out and the change is
silently lost.

Until the submodule is forked (see docs/MODDING_NOTES.md, risk C1), every commit
we make inside `src/psd` is also exported here so it can be recovered:

    cd src/psd
    git am ../../patches/psd-0001-luni-unicode-layer-name.patch

## Proper fix

1. Fork `aseprite/psd` to the same account that owns this fork.
2. `cd src/psd && git remote add fork <url> && git push fork main`
3. Point `.gitmodules` at the fork and run `git submodule sync`.

Then these patches become redundant.
