# pwfs: SVN to git, and Solaris to a Linux cross-build

Source: `http://source.gemini.edu/software/pwfs-rt`. This is the **unified**
PWFS application — one codebase serving both peripheral wavefront sensors.

## What is actually in production

Both crates boot this tree, and differ only in which startup script their boot
parameters name:

| crate | address | startup script |
|---|---|---|
| pwfs1 | 10.2.2.111 | `/gemini/epics3.13.4/pwfs/pwfs/bin/ppc604/startupMK_P1` |
| pwfs2 | 10.2.2.112 | `/gemini/epics3.13.4/pwfs/pwfs/bin/ppc604/startupMK_P2` |

Boot host `pisces-control` (10.2.2.57), kernel
`/gemini/external/vxWorks/tornado2.0/mv2700/vxWorks`, user `gemvx`, flags
`0x8`. `pwfs -> V1-7`; `local` cd's to the versioned path
`/gemini/epics3.13.4/pwfs/V1-7`. DHS is at 10.2.2.41 (`dataServerWfsNS`).

The two scripts differ only in probe identity: prompt, `pwfs1:`/`pwfs2:`
record prefix, which `Top.db`/`.pv` are loaded, and detControl's `"p1"`/`"p2"`
argument. The binaries are identical.

**The per-probe applications are frozen.** `pwfs1/V4-11` and `pwfs2/V4-13` are
still on disk but are not booted. Their histories are on the `archive/pwfs1`
and `archive/pwfs2` branches, with tags namespaced `pwfs1/*` and `pwfs2/*` —
namespacing matters because both use version numbers overlapping this
application's own, including `V1-7`, the deployed one.

## Provenance: what this repository's source is

The engineer's build trees were recovered from
`/home/gemvx/cristian/CB/` and compared against the deployment:

- **All 22 deployed `V1-7` binaries are byte-identical** to the `pwfs-rt`
  build tree. That tree is production's source, proven, not inferred.
- 190 of the 211 deployed files match it exactly.
- 19 of the rest are runtime output the IOC writes into its own deploy
  directory — circular-buffer dumps (`D*.cbi`/`.cbcao`/`.cbcfg`) and
  calibration FITS (`data/p{1,2}_*Hz.fits`, `coadd.fits`, `pwfs2.fits`), some
  written within the last week. **The RPM must not own these paths.**
- 2 were edited in place on 2025-03-27 and exist in neither SVN nor the build
  tree: `data/defDetContP2MK.dat` and `data/pwfs2SetDefCommandMK.pv`, PWFS2's
  detector ADC offsets. Production is the source of truth, so those values are
  committed here, and the spec marks the four tunables `%config(noreplace)` so
  an upgrade cannot revert a retune.

SVN trunk is **not** production for the older trees either: pwfs2's trunk is
missing two debug `printf`s that the deployed binary demonstrably contains.

## Build

GEM8.4 — a third EPICS tree, neither hrwfs's GEM7 nor gmoscc's GEM8.6:

```
APPLIC_BASE = /usr/software/dev/packages/epics/epics3.13.4GEM8.4/base
ld < /gemini/external/GEM8.4/base/bin/ppc604/iocCore
```

Two new dependency packages were needed, mirroring the GEM7 pair:
`gem-epics3134gem84` (build tree, with host tools rebuilt for Linux from the
tree's own sources — all eight directories, gcc 11.5.0, no patches) and
`gem84-epics-runtime` (the ppc604 objects the crates load at boot).

Everything else is reuse. The four support libraries and the eight DHS
`mv2700T2` libraries pwfs loads were verified **byte-identical** to the
packages already published for hrwfs, so no new library packaging was needed.

### Results

`gmake` exits 0; all 22 targets build.

- `gemini.Support` is **byte-identical** to production (3,715,573 bytes). It
  is relinked from the 144 prebuilt gcc 2.7.2 record objects, and Linux
  `ldppc` produces exactly what Solaris `ldppc` did.
- Every generated script matches production except one line, the `cd` to the
  deploy path, which the spec sets via `APPLIC_IOCPATH`.
- Compiled objects differ −8% to +2%. Production was built with
  `cygnus-2.7.2-960126`; the Linux cross-build uses gcc 2.96. Byte-identical
  output is **not** achievable and was never expected — GEM8.4's own ppc604
  `iocCore`, `seq` and `pvload` are all 2.7.2 too, so there is no existing
  mixed-compiler precedent in production to lean on. The gcc 2.96 ↔ 2.7.2
  runtime-linking boundary is verified by static analysis only; the crate test
  is what covers it.

### Traps

**applSetup overwrites the application's own startup files.** It copies
`local<SITE>.vws`, `resource<SITE>.def` and `UAE.dist` from the site templates
over `startup/`, every run, in an existing directory as well as a new one. A
customised `local.vws` is silently reverted to the template — and the template
is a plausible-looking site file, so the result boots against the wrong file
server rather than failing. `setup.sh` stashes the versioned copies before
applSetup and restores them after, failing the build if a restore does not
take.

This has not yet bitten pwfs only because its `local.vws` is byte-identical to
the GEM8.4 `localMK.vws` template. It *did* bite hrwfs, whose `local.vws` had
been re-homed. The workaround is visible in the archaeology: pwfs1's SVN tree
still carries `startup/local.vws_BACKUP` and `startup/resource.def_BACKUP`.

**The cross compiler cannot read a 64-bit-inode filesystem.** `ccppc`/`cpp`
are 32-bit and built without large-file support, so `stat()` returns
`EOVERFLOW` and the error is the unhelpful "Value too large for defined data
type", reported against whatever file it touched. Stage the tree on `/tmp` or
a tmpfs; NFS-backed home directories fail this way. Docker `--tmpfs` defaults
to `noexec`, so use `--tmpfs /build:size=3g,exec`.

**`APPLIC_IOCPATH` must be `host:path`.** `CONFIG_APPLIC` derives `DIST_PATH`
with `$(word 2, $(subst :, ,$(APPLIC_IOCPATH)))`, so a bare path yields
`cd ""`.

**Capfast is dead** — `sch2edif` checks out against a decommissioned licence
server — so the four generated `.db` are committed under `capfast/db` and
seeded by `setup.sh`. **dspsrc cannot build on Linux**: `asm56000` is a SPARC
Solaris binary, so the ten `.lod` images are committed and installed by the
spec.

## Open

- **Boot server.** pwfs still boots from `pisces-control`, unlike hrwfs. Moving
  it means editing `startup/local.vws` (server name *and* export path —
  pisces exports `/export/gemini`, mkotcsbootv2-lv1 exports `/gemini`) and
  adding 10.2.2.111 and 10.2.2.112 to the boot server's exports and rhosts.
- **CP package.** The CP startup variants exist; a CP build needs `-S CP` and
  a CP production tree to verify against, which has not been captured.
- **Crate test.** Must be functional — real DHS exposure, CAD/CAR paths, WCS —
  not just a boot, because the compiler boundary is only statically verified.
