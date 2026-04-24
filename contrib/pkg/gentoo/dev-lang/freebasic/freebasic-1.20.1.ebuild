# Copyright 2026 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

DESCRIPTION="FreeBASIC compiler"
HOMEPAGE="https://www.freebasic.net/"
SRC_URI="freebasic-${PV}.tar.xz"

LICENSE="GPL-2+ LGPL-2.1+"
SLOT="0"
KEYWORDS="~amd64 ~arm ~arm64 ~riscv"

RDEPEND="
	sys-devel/gcc
	sys-devel/binutils
	sys-libs/ncurses:=
	sys-libs/gpm
	dev-libs/libffi:=
	media-libs/alsa-lib
	media-libs/libpulse
	x11-libs/libX11
	x11-libs/libXext
	x11-libs/libXpm
	x11-libs/libXrandr
	x11-libs/libXrender
	virtual/opengl
	virtual/glu
"
DEPEND="${RDEPEND}"
BDEPEND="
	virtual/pkgconfig
	app-text/dos2unix
	net-misc/rsync
"

src_compile() {
	emake bootstrap-minimal
	emake all FBC=bootstrap/fbc
}

src_install() {
	emake install DESTDIR="${D}" prefix=/usr
}
