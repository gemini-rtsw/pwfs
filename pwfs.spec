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
# RPM replaces it with a real directory of the same name, so the startup script
# path in the boot parameters does not change -- the same transition gmoscc and
# hrwfs made. rpm -q names what is installed and dnf downgrade is the rollback,
# which is what the symlink could never tell you.
#
# The boot SERVER does change: both crates move from pisces-control
# (10.2.2.57) to mkotcsbootv2-lv1 (10.2.2.145), as hrwfs did -- `host name`
# and `host inet` in each crate's boot parameters, and startup/local.vws here.

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
# Read from tools/linux-build/build.conf, the single source shared with
# setup.sh and the Makefile, so a local build and this one cannot disagree.
# rpmbuild runs from the repository root (build_rpm.sh does `cd /work`), the
# same assumption the git_hash macro already makes.
%define buildconf() %(. tools/linux-build/build.conf 2>/dev/null && echo $%1)
%global slalib_ver  %{buildconf SLALIB_VER}
%global timelib_ver %{buildconf TIMELIB_VER}
%global astlib_ver  %{buildconf ASTLIB_VER}
%global cfitsio_ver %{buildconf CFITSIO_VER}
%if "%{slalib_ver}" == ""
%{error:tools/linux-build/build.conf not readable -- rpmbuild must run from the repository root}
%endif

# Exact builds the package is compiled against -- the same pins hrwfs uses.
%global slalib_nvr  1.9.4-1.git4a156f2%{?dist}
%global timelib_nvr 1.8.6-1.git63b2b74%{?dist}
%global astlib_nvr  1.4-1.gitdcad7c0%{?dist}
%global cfitsio_nvr 4.1-1.git819bc30%{?dist}

%global supdir  /gemini/epics3.13.4/support
%global deploy  %{buildconf DEPLOY}


# $GIT_HASH first: build_rpm.sh computes it on the HOST and passes it in.
%define git_hash %(if [ -n "$GIT_HASH" ]; then echo "$GIT_HASH"; else git rev-parse --short HEAD 2>/dev/null || echo nogit; fi)

%define name    pwfs
%define version 1.7
Name:           %{name}
Version:        %{version}
Release:        2.git%{git_hash}%{?dist}
Summary:        Gemini PWFS IOC software, unified P1/P2 (vxWorks 5.4 ppc604)
License:        Gemini Observatory (org-internal)
Source0:        %{name}-%{version}.tar.gz
AutoReqProv:    no
# The payload is ppc604 objects the host never executes; it serves them over
# NFS to two VME crates.
BuildArch:      noarch

BuildRequires:  gem-tornado20-linux = 2.0.2-1%{?dist}
BuildRequires:  gem-epics3134gem84 = 3.13.4-1%{?dist}
BuildRequires:  gem7-slalib-%{slalib_ver}-devel = %{slalib_nvr}
BuildRequires:  gem7-timelib-%{timelib_ver}-devel = %{timelib_nvr}
BuildRequires:  gem7-astlib-%{astlib_ver}-devel = %{astlib_nvr}
BuildRequires:  gem7-cfitsio-%{cfitsio_ver}-devel = %{cfitsio_nvr}
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
# Exactly what a developer runs. All build logic -- setup, the deploy path,
# the library versions -- lives in the repository (Makefile, setup.sh,
# build.conf), so this is the same command, producing the same files, as a
# local `make` in the same container.
#
# APPLIC_SITE selects which site's #if (MK)/(CP) blocks compile in; the
# default comes from build.conf.
make %{?site:APPLIC_SITE=%{site}}

# The same checks a developer can run by hand after a local build.
./tools/linux-build/check-build.sh

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
* Fri Oct 02 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 1.7-2
- Build in the gemini-rtsw-ci pipeline. All build logic in the repository
  (build.conf, Makefile, setup.sh); %build is plain make, identical to a local
  build. Re-homed to mkotcsbootv2-lv1.
* Wed Sep 16 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 1.7-1
- Initial RPM packaging via the Linux cross-build. Source is the unified pwfs
  application deployed as V1-7, verified byte-identical to production across
  all 22 loadable objects.
