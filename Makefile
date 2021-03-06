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

LDCONF = $(DESTDIR)/etc/ld.so.conf.d/nc_server-$(DEB_HOST_GNU_TYPE).conf

PKGCONFIG := $(DESTDIR)/usr/lib/$(DEB_HOST_GNU_TYPE)/pkgconfig/nc_server.pc
SCONSPKGCONFIG := $(DESTDIR)$(PREFIX)/$(ARCHLIBDIR)/pkgconfig/nc_server.pc

# Where to find pkg-configs of other software
PKG_CONFIG_PATH := /usr/lib/$(DEB_HOST_GNU_TYPE)/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig

SCONS = $(SCONSPATH) BUILDS=$(BUILDS) REPO_TAG=$(REPO_TAG) \
  ARCHLIBDIR=$(ARCHLIBDIR) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)

.PHONY : build clean scons_install $(LDCONF)

$(info DESTDIR=$(DESTDIR))
$(info DEB_BUILD_GNU_TYPE=$(DEB_BUILD_GNU_TYPE))
$(info DEB_HOST_GNU_TYPE=$(DEB_HOST_GNU_TYPE))
$(info PKG_CONFIG_PATH=$(PKG_CONFIG_PATH))

build:
	$(SCONS) PREFIX=$(PREFIX) --config=force -j 4

$(LDCONF):
	mkdir -p $(@D); \
	echo "/opt/nidas/lib/$(DEB_HOST_GNU_TYPE)" > $@

scons_install:
	$(SCONS) PREFIX=$(DESTDIR)$(PREFIX) -j 4 install

$(SCONSPKGCONFIG): scons_install

$(PKGCONFIG): $(SCONSPKGCONFIG)
	mkdir -p $(@D); \
	sed -i -e "s,$(DESTDIR),," $<; \
	cp $< $@

install: scons_install $(LDCONF) $(PKGCONFIG)
	cp scripts/nc_ping $(DESTDIR)$(PREFIX)/bin

clean:
	$(SCONS) -c

