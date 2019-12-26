#
# Makefile for nc-server Debian packages
# The primary task is to invoke scons to do the build and
# install to the $DESTDIR.

SCONSPATH = scons
BUILDS ?= "host"
REPO_TAG ?= v1.1
PREFIX=/opt/nc_server

# Where we want them in the package
ARCHLIBDIR = lib/$(DEB_HOST_GNU_TYPE)

PKGCONFIG := $(DESTDIR)/usr/lib/$(DEB_HOST_GNU_TYPE)/pkgconfig/nc_server.pc
SCONSPKGCONFIG := $(DESTDIR)$(PREFIX)/$(ARCHLIBDIR)/pkgconfig/nc_server.pc

# Where to find pkg-configs of other software
PKG_CONFIG_PATH := /usr/lib/$(DEB_HOST_GNU_TYPE)/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig

SCONS = $(SCONSPATH) $(NIDASPATH) BUILDS=$(BUILDS) REPO_TAG=$(REPO_TAG) \
  ARCHLIBDIR=$(ARCHLIBDIR) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)

NIDASPATH = NIDAS_PATH=/opt/nidas/armhf

.PHONY : build clean scons_install

$(info DESTDIR=$(DESTDIR))
$(info DEB_BUILD_GNU_TYPE=$(DEB_BUILD_GNU_TYPE))
$(info DEB_HOST_GNU_TYPE=$(DEB_HOST_GNU_TYPE))
$(info PKG_CONFIG_PATH=$(PKG_CONFIG_PATH))

build:
	$(SCONS) PREFIX=$(PREFIX) --config=force -j 4

scons_install:
	$(SCONS) PREFIX=$(DESTDIR)$(PREFIX) -j 4 install

$(SCONSPKGCONFIG): scons_install

$(PKGCONFIG): $(SCONSPKGCONFIG)
	mkdir -p $(@D); \
	sed -i -e "s,$(DESTDIR),," $<; \
	cp $< $@

install: scons_install $(PKGCONFIG)

clean:
	$(SCONS) -c

