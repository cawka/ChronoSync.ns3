# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION='0.0.1'
APPNAME='libChronoSync'

from waflib import Build, Logs, Configure

def options(opt):
    opt.add_option('--debug',action='store_true',default=False,dest='debug',help='''debugging mode''')
    opt.add_option('--log4cxx', action='store_true',default=False,dest='log4cxx',help='''Compile with log4cxx/native NS3 logging support''')

    opt.load('compiler_c compiler_cxx boost gnu_dirs ns3 protoc')

REQUIRED_NS3_MODULES = ['ndnSIM', 'core', 'network', 'internet', 'point-to-point']

def configure(conf):
    conf.load("compiler_cxx gnu_dirs boost ns3")

    conf.define('NS3_MODULE', 1)
    if conf.options.debug:
        conf.define ('_DEBUG', 1)
        conf.add_supported_cxxflags (cxxflags = ['-O0',
                                                 '-Wall',
                                                 '-Wno-unused-variable',
                                                 '-g3',
                                                 '-Wno-unused-private-field', # only clang supports
                                                 '-fcolor-diagnostics',       # only clang supports
                                                 '-Qunused-arguments'         # only clang supports
                                                 ])
    else:
        conf.add_supported_cxxflags (cxxflags = ['-O3', '-g'])


    if not conf.check_cfg(package='openssl', args=['--cflags', '--libs'], uselib_store='SSL', mandatory=False):
      libcrypto = conf.check_cc(lib='crypto',
                                header_name='openssl/crypto.h',
                                define_name='HAVE_SSL',
                                uselib_store='SSL')
    if not conf.get_define ("HAVE_SSL"):
        conf.fatal ("Cannot find SSL libraries")

    conf.check_ns3_modules(REQUIRED_NS3_MODULES, mandatory = True)

    conf.check_boost(lib='system iostreams test')
    conf.define ('NS3_LOG_ENABLE', 1)

    conf.load('protoc')

def build (bld):
    libsync = bld.shlib (
        target = "ChronoSync.ns3",
        features=['cxx', 'cxxshlib'],
        source =  bld.path.ant_glob (['ns3/*.cc', 
                                      'src/*.cc', 
                                      'src/*.proto']),
        use = 'BOOST BOOST_IOSTREAMS SSL PROTOBUF ' + ' '.join (['ns3_'+dep for dep in REQUIRED_NS3_MODULES]).upper (),
        includes = ['src', 'ns3'],
        )

    # Unit tests
    unittests = bld.program (
        target="unit-tests",
        source = bld.path.ant_glob(['test-ns3/*.cc']),
        features=['cxx', 'cxxprogram'],
        use = 'BOOST_TEST ChronoSync.ns3',
        includes = ['src', 'ns3'],
        install_path = None
        )

    headers = bld.path.ant_glob(['src/*.h', 
                                 'ns3/*.h']) + bld.path.get_bld ().ant_glob (['src/*.h'])

    bld.install_files ("%s/ChronoSync.ns3" % bld.env['INCLUDEDIR'], headers)

    bld (
        source='ChronoSync.ns3.pc.in',
        install_path = '${LIBDIR}/pkgconfig',
        PREFIX       = bld.env['PREFIX'],
        INCLUDEDIR   = "%s/ChronoSync.ns3" % bld.env['INCLUDEDIR'],
        VERSION      = VERSION,
        )


@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by compiler and add them to env.CXXFLAGS variable
    """
    self.start_msg('Checking allowed flags for c++ compiler')

    supportedFlags = []
    for flag in cxxflags:
        if self.check_cxx (cxxflags=[flag], mandatory=False):
            supportedFlags += [flag]

    self.end_msg (' '.join (supportedFlags))
    self.env.CXXFLAGS += supportedFlags
