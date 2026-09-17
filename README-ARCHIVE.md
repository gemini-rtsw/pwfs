# pwfs2 -- frozen history

This branch is the complete history of the **pwfs2** wavefront sensor
application, migrated from Subversion (`pwfs2-rt`, source.gemini.edu).

**It is not built, deployed, or run.** Gemini North's PWFS crates both boot the
*unified* `pwfs` application on this repository's `main` branch:

    pwfs1 crate 10.2.2.111 -> /gemini/epics3.13.4/pwfs/pwfs/bin/ppc604/startupMK_P1
    pwfs2 crate 10.2.2.112 -> /gemini/epics3.13.4/pwfs/pwfs/bin/ppc604/startupMK_P2

one binary set with two startup scripts differing only in probe identity.

The last pwfs2 release, `V4-13`, remains on disk at
`/gemini/epics3.13.4/pwfs2/V4-13` as a fallback, so this history is kept
rather than discarded -- if that deployment is ever reinstated, this is the
source for it.

## Relationship to `main`

None, deliberately. `pwfs-rt` was imported into Subversion as its own CVS
module, not copied from `pwfs2-rt`, so the two histories share no commit.
That is why this is an unrelated-history branch rather than an ancestor of
`main`.

## Tags

This branch's tags are namespaced `pwfs2/<tag>` -- `pwfs2/V1-0`, `pwfs2/V4-9`
and so on. Both archived applications use version numbers that overlap the
unified application's own (`V1-0`..`V1-7`), including `V1-7`, the
version currently deployed. Flat tags would collide on exactly the tag that
matters most.
