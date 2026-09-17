#!/bin/bash
# Per-checkout bootstrap for building pwfs on Linux. Run once after cloning;
# then plain `gmake` builds forever after.
#
#   ./tools/linux-build/setup.sh && gmake
#
# Everything here exists because applSetup.pl stamps the checkout's ABSOLUTE
# path into the generated config, so it cannot be baked into an image.
set -e

TOP="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$TOP"

# Source the GEM8.4 profile BY NAME rather than relying on the login shell.
# gem-epics3134gem7 and gem-epics3134gem84 both export EPICS and prepend to
# PATH from /etc/profile.d; if both are ever installed, whichever sorts later
# wins. Naming the file makes this deterministic.
if [ -f /etc/profile.d/gem84.sh ]; then . /etc/profile.d/gem84.sh
else . "$TOP/tools/linux-build/gem-env.sh"; fi

# Scrub generated state. applSetup PRESERVES APPLIC_INSTALL from an existing
# config/CONFIG.Defs, so a checkout bootstrapped elsewhere would keep the stale
# path -- failing loudly if absent, or silently baking a wrong path into the
# generated startup scripts if it happens to exist.
#
# include/ is on this list because in the unified pwfs tree it is entirely
# generated: every header in it is a copy of one in src/. That is NOT true of
# the older pwfs2 tree, where detControl.h exists only in include/ -- so this
# line is correct here and would lose a file there.
rm -rf config bin lib include dbd data Distfile .applTop resource.def javalib \
       APPLIC_INSTALL.txt pwfsBuild.log
find . -type d -name 'O.*' -prune -exec rm -rf {} + 2>/dev/null || true

echo "APPLIC_TOP = $TOP" > .applTop

# Capfast is dead: sch2edif fails a FlexLM check against a decommissioned
# licence server, so the .sch -> .edf -> .db chain cannot run anywhere. The
# generated databases are committed in capfast/db; seed them into the build
# directory and make them NEWER than the .sch files, or make regenerates.
mkdir -p capfast/O.$HOST_ARCH
cp capfast/db/*.db capfast/O.$HOST_ARCH/
touch capfast/O.$HOST_ARCH/*.db

# pwfsInstall's applSetup arguments, minus the site autodetect (its
# `[ "..." -ne "" ]` is a numeric test on a string and misbehaves) and minus
# the trailing $1, which would set APPLIC_IOCPATH. The spec sets that to the
# deploy path; a developer build leaves it empty and cd's into the checkout.
SITE="${APPLIC_SITE:-MK}"
cp -f IMP_Startup$SITE.pwfs1 IMP_Startup.pwfs1
cp -f IMP_Startup$SITE.pwfs2 IMP_Startup.pwfs2

# Invoke through perl explicitly: applSetup.pl's shebang is the Solaris path
# /usr/software/dev/solaris/bin/perl. A symlink there works, but not if
# /usr/software is a bind mount -- the mount hides anything the image created
# underneath it.
APPLSETUP="$EPICS_BASE/bin/$HOST_ARCH/applSetup.pl"
[ -f "$APPLSETUP" ] || { echo "ERROR: $APPLSETUP not found -- build the host tools first" >&2; exit 1; }
# -d names the PACKAGED support-library paths, not the old shared
# /gemini/epics3.13.4/<lib>/<lib> symlinks that pwfsInstall used. Those still
# serve the other GEM IOCs and must not be what this build resolves against.
# These four were verified byte-identical to what the crates already load.
SUP=/gemini/epics3.13.4/support
# applSetup OVERWRITES the application's own startup files from the site
# templates. In an existing startup directory it copies, unconditionally:
#
#   templates/uae/startup/local<SITE>.vws  -> startup/local.vws
#   templates/uae/startup/resource<SITE>.def -> startup/resource.def
#   templates/uae/startup/UAE.dist         -> startup/UAE.dist
#
# (GEM7 applSetup.pl lines 571 and 592; GEM8.4 is the same when APPLIC_SITE is
# set, which it always is here.) So a customised local.vws is silently
# reverted to the template on every build -- and because the template is a
# plausible-looking site file, the result boots and mounts from the WRONG file
# server rather than failing. That is exactly how this bit hrwfs: its
# local.vws had been re-homed to mkotcsbootv2-lv1 and every RPM shipped the
# template's pisces-control instead.
#
# Evidence this has bitten people before: pwfs1's SVN tree still carries
# startup/local.vws_BACKUP and startup/resource.def_BACKUP.
#
# So: stash whatever the repository actually versions, and put it back after.
# Only files that existed BEFORE are restored -- if a file legitimately comes
# from the template, it is not in the stash and is left alone.
STASH=$(mktemp -d)
for f in local.vws resource.def UAE.dist; do
    [ -f "startup/$f" ] && cp -p "startup/$f" "$STASH/$f"
done

perl "$APPLSETUP" -T ppc604 -I adl -I capfast -I src -I startup \
             -I docs -I dspsrc -I par -I db \
             -d $SUP/astlib/V1-4 \
             -d $SUP/slalib/V1-9-4 \
             -d $SUP/timelib/V1-8-6 \
             -d $SUP/cfitsio/V4-1 \
             -d /gemini/dhs/dhs -S "$SITE"

# Put back the versioned startup files applSetup just overwrote (see above).
for f in local.vws resource.def UAE.dist; do
    if [ -f "$STASH/$f" ]; then
        if ! cmp -s "$STASH/$f" "startup/$f"; then
            echo "  restoring startup/$f (applSetup replaced it with the site template)"
            cp -p "$STASH/$f" "startup/$f"
        fi
    fi
done

# Verify BEFORE discarding the stash: shipping the template's file server
# instead of ours is a silent, bootable, wrong result, so a failed restore
# must stop the build rather than be discovered at a crate.
for f in local.vws resource.def UAE.dist; do
    if [ -f "$STASH/$f" ] && ! cmp -s "$STASH/$f" "startup/$f"; then
        echo "ERROR: startup/$f was not restored after applSetup" >&2
        rm -rf "$STASH"; exit 1
    fi
done
rm -rf "$STASH"


# dspsrc needs Motorola's asm56000/dsplnk/cldlod/srec, which exist only as
# SPARC Solaris binaries -- it cannot build on Linux at all. Its ten generated
# .lod files are committed in dspsrc/ and installed by the spec instead.
sed -i '/^DIRS += dspsrc *$/d' Makefile.Dirs

# Match production's debug format: gcc 2.7.2 emitted stabs, gcc 2.96 defaults
# to DWARF, and every deployed GEM object carries .stab/.stabstr. Runtime is
# unaffected either way (vxWorks ld ignores debug sections) but the Tornado 2.0
# debugger reads stabs. DEBUG_CFLAGS is in the CFLAGS chain and assigned
# nowhere, so it is a free hook.
echo "DEBUG_CFLAGS = -gstabs" >> config/CONFIG.Defs

# applSetup failing leaves no config/, and the top-level Makefile uses
# `-include $(APPLIC_TOP)/config/CONFIG` -- so gmake would silently fall
# through to its first target and tar the source tree instead of building
# anything, exiting 0. Fail here instead.
for f in config/CONFIG config/CONFIG.Defs config/RULES.Dirs; do
    [ -f "$f" ] || { echo "ERROR: applSetup.pl did not produce $f" >&2; exit 1; }
done

echo
echo "Setup complete for site $SITE:"
grep '^APPLIC_' config/CONFIG.Defs | sed 's/^/  /'
echo "  run 'gmake' to build."
