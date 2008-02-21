#
# spec file for package SUNWcherokee
#
# Copyright (c) 2008 Sun Microsystems, Inc.
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#

%include Solaris.inc

Name:                    SUNWcherokee
Summary:                 cherokee - Fast, flexible, lightweight web server
Version:                 0.5.0
Source:                  http://www.cherokee-project.com/download/0.5/%{version}/cherokee-%{version}.tar.gz
SUNW_BaseDir:            %{_basedir}
BuildRoot:               %{_tmppath}/%{name}-%{version}-build

%include default-depend.inc
#BuildRequires: SUNWgnome-base-libs-share

%package root
Summary:                 cherokee - Fast, flexible, lightweight web server - platform dependent files, / filesystem
SUNW_BaseDir:            /
%include default-depend.inc

%package devel          
Summary:                 cherokee - Fast, flexible, lightweight web server - developer files
SUNW_BaseDir:            %{_basedir}
%include default-depend.inc
Requires: SUNWcherokee

%prep
%setup -q -n cherokee-%version

%build
#export PKG_CONFIG_PATH=%{_pkg_config_path}
#export MSGFMT="/usr/bin/msgfmt"
export CFLAGS="%optflags -I%{_includedir}"
export RPM_OPT_FLAGS="$CFLAGS"
export LDFLAGS="%_ldflags"

./configure --prefix=%{_prefix}                 \
            --enable-os-string="OpenSolaris"    \
            --enable-pthreads                   \
            --libexecdir=%{_libexecdir}         \
            --datadir=%{_datadir}               \
            --mandir=%{_mandir}                 \
            --infodir=%{_infodir}               \
            --sysconfdir=%{_sysconfdir}         \
            --localstatedir=%{_localstatedir}   \
            --with-wwwroot=%{_localstatedir}/cherokee
make -j$CPUS

%install
make install DESTDIR=$RPM_BUILD_ROOT

rm -f $RPM_BUILD_ROOT%{_libdir}/lib*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/lib*.la
rm -f $RPM_BUILD_ROOT%{_libdir}/cherokee/lib*.a
rm -f $RPM_BUILD_ROOT%{_libdir}/cherokee/lib*.la

mkdir -p ${RPM_BUILD_ROOT}/var/svc/manifest/network/
cp http-cherokee.xml ${RPM_BUILD_ROOT}/var/svc/manifest/network/http-cherokee.xml

%{?pkgbuild_postprocess: %pkgbuild_postprocess -v -c "%{version}:%{jds_version}:%{name}:$RPM_ARCH:%(date +%%Y-%%m-%%d):%{support_level}" $RPM_BUILD_ROOT}



%post -n SUNWcherokee-root

if [ -f /lib/svc/share/smf_include.sh ] ; then
    . /lib/svc/share/smf_include.sh
    smf_present
    if [ $? -eq 0 ]; then
	/usr/sbin/svccfg import /var/svc/manifest/network/http-cherokee.xml
    fi
fi

exit 0

%preun -n SUNWcherokee-root
if [  -f /lib/svc/share/smf_include.sh ] ; then
    . /lib/svc/share/smf_include.sh
    smf_present
    if [ $? -eq 0 ]; then
	if [ `svcs  -H -o STATE svc:/network/http:cherokee` != "disabled" ]; then
	    svcadm disable svc:/network/http:cherokee
	fi
    fi
fi


%postun -n SUNWcherokee-root

if [ -f /lib/svc/share/smf_include.sh ] ; then
    . /lib/svc/share/smf_include.sh
    smf_present
    if [ $? -eq 0 ] ; then
	/usr/sbin/svccfg export svc:/network/http > /dev/null 2>&1
	if [ $? -eq 0 ] ; then
	    /usr/sbin/svccfg delete -f svc:/network/http:cherokee
	fi
    fi
fi

exit 0




%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr (-, root, other)
%dir %attr (0755, root, bin) %{_bindir}
%{_bindir}/*
%dir %attr (0755, root, bin) %{_sbindir}
%{_sbindir}/cherokee
%dir %attr (0755, root, bin) %{_libdir}
%{_libdir}/lib*.so*
%{_libdir}/cherokee
%dir %attr (0755, root, sys) %{_datadir}
%dir %attr(0755, root, bin) %{_mandir}
%dir %attr(0755, root, bin) %{_mandir}/man1
%{_mandir}/man1/*
%{_datadir}/aclocal
%{_datadir}/cherokee
%{_datadir}/doc


%files root
%defattr (-, root, other)
%attr (0755, root, sys) %dir %{_sysconfdir}
%{_sysconfdir}/*
%dir %attr (0755, root, sys) %{_localstatedir}
%{_localstatedir}/cherokee/*
%defattr (0755, root, sys)
%attr(0444, root, sys)/var/svc/manifest/network/http-cherokee.xml

%files devel
%dir %attr (0755, root, bin) %dir %{_libdir}
%dir %attr (0755, root, other) %{_libdir}/pkgconfig
%{_libdir}/pkgconfig/*
%dir %attr (0755, root, bin) %{_includedir}
%{_includedir}/*


%changelog
* Wed Jan 25 2006 - rodrigo.fernandez-vizarra@sun.com
- Added SMF definition file install/removal

* Tue Jan 17 2006 - damien.carbery@sun.com
- Created.
