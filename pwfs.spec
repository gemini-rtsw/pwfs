# pwfs -- the Gemini Peripheral Wavefront Sensor IOC (unified P1/P2).
#
# Cross-compiles for vxWorks 5.4 / ppc604 using the GEM8.4 EPICS 3.13.4 tree
# and the Tornado 2.0.2 target headers, with ANL's Linux rebuild of the Wind
# River GNU tools. See MIGRATION-PLAN.md for what is specific to pwfs.
#
# ONE APPLICATION, TWO CRATES. Both PWFS crates boot this same tree and differ
# only in which startup script their boot parameters name:
#
#   pwfs1 crate 10.2.2.111  s = %{deploy}/bin/ppc604/startupMK_P1
#   pwfs2 crate 10.2.2.112  s = %{deploy}/bin/ppc604/startupMK_P2
#
# The two scripts differ only in probe identity -- prompt, record prefix, which
# Top.db and .pv are loaded, and detControl's "p1"/"p2" argument. The binaries
# are identical. The older per-probe applications (pwfs1/V4-11, pwfs2/V4-13)
# are frozen and not booted; their history is on the archive/* branches.
#
# %{deploy} is currently a symlink to the versioned directory of the day. The
# RPM replaces it with a real directory of the same name, so no boot parameter
# changes -- the same transition gmoscc and hrwfs made. rpm -q names what is
# installed and dnf downgrade is the rollback, which is what the symlink could
# never tell you.

%global _build_id_links none
%global __os_install_post %{nil}
%global debug_package %{nil}
%global _binaries_in_noarch_packages_terminate_build 0

# ---------------------------------------------------------------------------
# Support-library versions. These drive THREE things that must agree: the
# build-time -d flags, the runtime Requires, and the literal `ld <` paths in
# the generated startup scripts. Defining them once is what stops a pin bump
# from leaving the crate loading a different version than was tested -- for
# VxWorks that is a real hazard, because ld at boot means the copy on the file
# server IS the running code, not merely something linked against.
#
# All four were verified byte-identical to what the crates already load from
# the unversioned /gemini/epics3.13.4/<lib>/<lib> paths, so these are the same
# builds under managed names -- no new library packaging was needed for pwfs.
%global slalib_ver  V1-9-4
%global timelib_ver V1-8-6
%global astlib_ver  V1-4
%global cfitsio_ver V4-1

%global supdir  /gemini/epics3.13.4/support
%global deploy  /gemini/epics3.13.4/pwfs/pwfs

# Host half of APPLIC_IOCPATH. Only the path half reaches the startup scripts;
# this exists because CONFIG_APPLIC splits the value on a colon.
#
# STILL pisces-control, unlike hrwfs which was re-homed to mkotcsbootv2-lv1.
# Production's boot parameters and local.vws both name pisces-control
# (10.2.2.57), so this reproduces what operations runs today. Moving pwfs to
# the new boot server is a separate, deliberate change -- it means editing
# startup/local.vws (server name AND export path: pisces exports
# /export/gemini, mkotcsbootv2-lv1 exports /gemini) and adding 10.2.2.111 and
# 10.2.2.112 to the boot server's exports and rhosts.
%global iocpath_host pisces-control

# $GIT_HASH first: build_rpm.sh computes it on the HOST and passes it in.
%define git_hash %(if [ -n "$GIT_HASH" ]; then echo "$GIT_HASH"; else git rev-parse --short HEAD 2>/dev/null || echo nogit; fi)

%define name    pwfs
%define version 1.7
Name:           %{name}
Version:        %{version}
Release:        1.git%{git_hash}%{?dist}
Summary:        Gemini PWFS IOC software, unified P1/P2 (vxWorks 5.4 ppc604)
License:        Gemini Observatory (org-internal)
Source0:        %{name}-%{version}.tar.gz
AutoReqProv:    no
# The payload is ppc604 objects the host never executes; it serves them over
# NFS to two VME crates.
BuildArch:      noarch

BuildRequires:  gem-tornado20-linux = 2.0.2-1%{?dist}
BuildRequires:  gem-epics3134gem84 = 3.13.4-1%{?dist}
BuildRequires:  gem7-slalib-%{slalib_ver}-devel
BuildRequires:  gem7-timelib-%{timelib_ver}-devel
BuildRequires:  gem7-astlib-%{astlib_ver}-devel
BuildRequires:  gem7-cfitsio-%{cfitsio_ver}-devel
BuildRequires:  hrwfs-dhs-vxlibs
BuildRequires:  make, gcc, perl, tcsh

# Runtime deps are the OTHER /gemini trees the crates ld at boot, not anything
# this host executes -- AutoReqProv finds none of them, because the consumer is
# a vxWorks target reading them over NFS.
#
# Named WITHOUT an exact release, deliberately: the package name already
# encodes the library version, and rpm permits one release of a given name at
# a time, so pinning the release makes two consumers built against different
# REBUILDS of the same library version mutually uninstallable. That bit hrwfs.
Requires:       gem84-epics-runtime = 3.13.4
Requires:       gem7-slalib-%{slalib_ver}
Requires:       gem7-timelib-%{timelib_ver}
Requires:       gem7-astlib-%{astlib_ver}
Requires:       gem7-cfitsio-%{cfitsio_ver}
Requires:       hrwfs-dhs-vxlibs
Requires:       gem-vxworks-tornado20 >= 2.0.2

%description
EPICS 3.13.4 (GEM8.4) control software for the Gemini Peripheral Wavefront
Sensors, cross-compiled for the ppc604 vxWorks target. One application serves
both probes: PWFS1 and PWFS2 boot the same binaries and differ only in their
startup script. Installs the deployable IOC tree under %{deploy}: loadable
objects, generated startup scripts, databases, the DSP images and the detector
parameter files.

%package devel
Summary:        Build environment for pwfs development images
Requires:       gem-tornado20-linux, gem-epics3134gem84
Requires:       gem7-slalib-%{slalib_ver}-devel, gem7-timelib-%{timelib_ver}-devel
Requires:       gem7-astlib-%{astlib_ver}-devel, gem7-cfitsio-%{cfitsio_ver}-devel
Requires:       hrwfs-dhs-vxlibs
Requires:       make, gcc, perl, tcsh
%description devel
Pulls the pinned pwfs build dependencies into a dev container.

%prep
%setup -q

%build
. /etc/profile.d/gem84.sh

# Bootstrap is shared with interactive use, so a developer build and this one
# run identical steps.
APPLIC_SITE=%{?site}%{!?site:MK} ./tools/linux-build/setup.sh

# Re-home APPLIC_IOCPATH to the deploy path BEFORE building. macTest
# substitutes $(iocpath) into every generated script, so without this the
# startup scripts cd into the rpmbuild directory and the crate boots into
# nothing. It must be host:path -- CONFIG_APPLIC derives DIST_PATH with
#   $(word 2, $(subst :, ,$(APPLIC_IOCPATH)))
# so a bare path yields an empty cd "".
sed -i 's|^APPLIC_IOCPATH *=.*|APPLIC_IOCPATH = %{iocpath_host}:%{deploy}|' config/CONFIG.Defs
grep -q "^APPLIC_IOCPATH = %{iocpath_host}:%{deploy}$" config/CONFIG.Defs || {
    echo "ERROR: APPLIC_IOCPATH rewrite did not take" >&2; exit 1; }

make

# Re-home the support-library load paths. The sources name the unversioned
# shared trees (/gemini/epics3.13.4/slalib/slalib/...) which record nothing;
# the packages install under %%{supdir}/<lib>/<VER>.
sed -i -e 's|@SLALIB_VER@|%{slalib_ver}|g' -e 's|@TIMELIB_VER@|%{timelib_ver}|g' \
       -e 's|@ASTLIB_VER@|%{astlib_ver}|g' -e 's|@CFITSIO_VER@|%{cfitsio_ver}|g' \
       bin/ppc604/startup* bin/ppc604/local

# Guards. Each of these has failed at least once during the hrwfs port, and
# every one of them is silent at build time and fatal at boot.
if grep -l '@[A-Z_]*_VER@' bin/ppc604/* 2>/dev/null | grep -q .; then
    echo "ERROR: unsubstituted @..._VER@ remains in a generated script" >&2; exit 1
fi
for v in %{slalib_ver} %{timelib_ver} %{astlib_ver} %{cfitsio_ver}; do
    grep -q "$v" bin/ppc604/startupMK_P1 || {
        echo "ERROR: startupMK_P1 does not name $v" >&2; exit 1; }
done
for f in startupMK_P1 startupMK_P2 local; do
    grep -q 'cd "%{deploy}"' bin/ppc604/$f || {
        echo "ERROR: $f does not cd into %{deploy}" >&2; exit 1; }
done
if grep -rl '/root/rpmbuild' bin 2>/dev/null | grep -q .; then
    echo "ERROR: build path leaked into the payload" >&2; exit 1
fi

%install
# Mirror the historical rdist payload (startup/UAE.dist): bin/<arch>, include,
# dbd, data, plus the DSP images and RELEASE.NOTES.
rm -rf $RPM_BUILD_ROOT
D=$RPM_BUILD_ROOT%{deploy}
mkdir -p $D/bin
cp -a bin/ppc604 $D/bin/
rm -f $D/bin/ppc604/Distfile
# The DSP images cannot be rebuilt on Linux -- asm56000 is a SPARC Solaris
# binary -- so they are committed and installed from the source tree.
mkdir -p $D/bin/asm56000
cp -a dspsrc/*.lod $D/bin/asm56000/
cp -a include dbd data RELEASE.NOTES $D/
cp -a IMP_Startup.pwfs1 IMP_Startup.pwfs2 $D/ 2>/dev/null || :

%files
%defattr(-,root,root,-)
%{deploy}
# Files operations edits in place, or the IOC rewrites at runtime. Marked
# noreplace so an upgrade cannot silently undo a retune: PWFS2's detector ADC
# offsets were changed directly in the deployed tree on 2025-03-27 and existed
# in neither SVN nor the engineer's build tree. Those values are committed
# here (production is the source of truth), and this stops the next upgrade
# from reverting them the way a plain %%files entry would.
%config(noreplace) %{deploy}/data/defDetContP1MK.dat
%config(noreplace) %{deploy}/data/defDetContP2MK.dat
%config(noreplace) %{deploy}/data/pwfs1SetDefCommandMK.pv
%config(noreplace) %{deploy}/data/pwfs2SetDefCommandMK.pv

%files devel

%changelog
* Wed Sep 16 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 1.7-1
- Initial RPM packaging via the Linux cross-build. Source is the unified pwfs
  application deployed as V1-7, verified byte-identical to production across
  all 22 loadable objects.
