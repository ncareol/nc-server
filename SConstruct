# -*- python -*-
#
# SConstruct for building nc_server RPC server program, shared library for
# clients, and related utilities.

# The server requires: nidas, netcdf, then svc, procs, and xdr generated from .x
# interface definition.
#
# The client library only requires: clnt and xdr.
#
# The nc_check utility is strictly a netcdf utility, it only needs netcdf.
#
# The other utilities, nc_close, nc_sync, and nc_shutdown, are strictly
# wrappers which call the client library.
#
# So after creating one default environment, derive from it 4 environments
# according to the various requirements above: srv_env, lib_env,
# client_env, nc_env.

import eol_scons

env = Environment(tools=['default', 'gitinfo', 'symlink', 'rpcgen'])

conf = Configure(env)
if conf.CheckCHeader('sys/capability.h'):
    conf.env.Append(CPPDEFINES = ['HAS_CAPABILITY_H'])
if conf.CheckLib('cap'):
    conf.env.AppendUnique(LIBS = 'cap')
env = conf.Finish()

opts = eol_scons.GlobalVariables('config.py')
opts.AddVariables(PathVariable('PREFIX','installation path',
                               '/opt/nc_server', PathVariable.PathAccept))

opts.Add('REPO_TAG',
         'git tag of the source, in the form "vX.Y", when '
         'building outside of a git repository')
opts.Add('BUILDS',
         'A host architecture to build for: host, armbe, armel or armhf.',
         'host')
opts.Add('ARCHLIBDIR', 
         'Where to install nc_server libraries relative to $PREFIX')
opts.Add('PKG_CONFIG_PATH', 
         'Path to pkg-config files, if you need other than the system default')
opts.Update(env)

# Propagate path to the process environment for running pkg-config
if env.has_key('PKG_CONFIG_PATH'):
    env['ENV']['PKG_CONFIG_PATH'] = env['PKG_CONFIG_PATH']

BUILDS = Split(env['BUILDS'])

if 'host' in BUILDS:
    # Must wait to load sharedlibrary until REPO_TAG is set in all situations
    env = env.Clone(tools=['sharedlibrary'])
elif 'armel' in BUILDS:
    # Must wait to load sharedlibrary until REPO_TAG is set in all situations
    env = env.Clone(tools=['armelcross', 'sharedlibrary'])
elif 'armhf' in BUILDS:
    # Must wait to load sharedlibrary until REPO_TAG is set in all situations
    env = env.Clone(tools=['armhfcross', 'sharedlibrary'])

# Default settings for all builds.  Need to turn off deprecated warnings
# until exception specifications are removed from nidas code.
env['CCFLAGS'] = ['-g', '-Wall', '-O2']
env['CXXFLAGS'] = ['-Weffc++', '-Wno-deprecated']

env.GitInfo("version.h", "#")

# Clone the netcdf environment before adding the RPC/XDR settings.
nc_env = env.Clone()
nc_env.Require('netcdf')

# -L: generated code sends rpc server errors to syslog
env['RPCGENSERVICEFLAGS'] = ['-L']

# Tool which adds build settings for RPC/XDR.
def rpc(env):
    # As of Fedora 28, glibc does not include the deprecated Sun RPC
    # interface because it does not support IPv6:
    #
    # https://fedoraproject.org/wiki/Changes/SunRPCRemoval
    #
    # So use the tirpc package config if available, otherwise fall back to
    # the legacy rpc built into glibc.
    try:
        env.ParseConfig('pkg-config --cflags --libs libtirpc')
        print("Using libtirpc.")
        env['PCREQUIRES'] = "libtirpc"
    except OSError:
        print("Using legacy rpc.")
        pass

# The rest of the environments to setup, server, lirbrary, and clients,
# will need RPC/XDR.
env.Tool(rpc)

# Generate the rpcgen products from the base environment.
clnt = env.RPCGenClient('nc_server_rpc.x')
header = env.RPCGenHeader('nc_server_rpc.x')
svc = env.RPCGenService('nc_server_rpc.x')
xdr = env.RPCGenXDR('nc_server_rpc.x')
env.Depends(xdr, header)

# Build the client library
lib_env = env.Clone()
lib_env.Append(CCFLAGS = ['-Wno-unused', '-Wno-strict-aliasing'])
libobjs = lib_env.SharedObject([xdr, clnt])
lib = lib_env.SharedLibrary3('nc_server_rpc', libobjs)

# Define a tool to build against the nc_server client library.
def nc_server_client(env):
    env.AppendUnique(LIBPATH='.')
    env.Append(LIBS='nc_server_rpc')

# clients need the client library
clnt_env = env.Clone()
clnt_env.Tool(nc_server_client)

# Server needs xdr from the client library, so derive the server
# environment from the client env, then add netcdf, and get nidas using
# pkg-config.  This might be able to use the nidas tool instead, but that
# has not been tried yet.
srv_env = clnt_env.Clone()
srv_env.ParseConfig('pkg-config --cflags --libs nidas')
srv_env.Require(['netcdf'])

srcs = ["nc_server.cc", "nc_server_rpc_procs.cc", svc]

print("ARCHLIBDIR=%s" % env['ARCHLIBDIR'])

nc_server = srv_env.Program('nc_server', srcs)

nc_close = clnt_env.Program('nc_close','nc_close.cc')

nc_sync = clnt_env.Program('nc_sync','nc_sync.cc')

nc_shutdown = clnt_env.Program('nc_shutdown','nc_shutdown.cc')

nc_check = nc_env.Program('nc_check','nc_check.c')

libtgt = env.SharedLibrary3Install('$PREFIX',lib)
env.Install('$PREFIX/bin',
            [nc_server, nc_close, nc_sync, nc_shutdown, nc_check])
env.Install('$PREFIX/include', 'nc_server_rpc.h')
env.Install('$PREFIX/bin', ['scripts/nc_ping', 'scripts/nc_server.check'])
env.Alias('install', ['$PREFIX'])
env.Alias('install', libtgt)

# Create nc_server.pc, replacing @token@
env.Command('nc_server.pc', '#nc_server.pc.in',
            "sed -e 's,@PREFIX@,$PREFIX,' -e 's,@ARCHLIBDIR@,$ARCHLIBDIR,'"
            " -e 's,@REPO_TAG@,$REPO_TAG,' "
            " -e 's,@REQUIRES@,$PCREQUIRES,' "
            "< $SOURCE > $TARGET")

Alias('install',
      env.Install('$PREFIX/$ARCHLIBDIR/pkgconfig', 'nc_server.pc'))

env.SetHelp()
