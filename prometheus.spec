
Vendor:       Mathias Kettner
Copyright:    Copyright 2002 by Mathias Kettner and Michael Dragovic
Name:         prometheus
Version:      1.2.1
Release:      4
Group:        Games
Provides:     prometheus
Source:       %{name}-%{version}.tar.gz
BuildRoot:    /tmp/prombuild
Summary:      PROMETHEUS Play-By-Email-Game from Mathias Kettner
AutoReqProv:  On

%description
PROMETHEUS Play-By-Email-Game from Mathias Kettner

Concept, Design, Programming, Rulebook and Gameplay by Mathias Kettner <mk@quara.de>
Graphics by Michael Dragovic <dragovic@gmx.net>
English Translation by Günther Brenner <GBrenner@bas.de>

%prep
%setup

%build
make

%install
make DESTDIR=$RPM_BUILD_ROOT install

%files
%attr(-,root,root) /usr/share/prometheus
%attr(-,root,root) /usr/bin/prometheus-*

%changelog
