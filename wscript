#!/usr/bin/env python


def options(opt):
    opt.load('compiler_c')
    opt.load('ragel', tooldir = 'tools')

def configure(conf):
    conf.load('compiler_c')
    conf.env.append_unique('CFLAGS', ['-std=c99', '-Wall', '-Wextra', '-Werror', '-g'])
    conf.env.append_value('INCLUDES', ['include'])
    conf.load('ragel', tooldir = 'tools')


def build(bld):
    sources = bld.path.ant_glob(['src/dht_bucket.c', 'src/dht_node.c', 'src/dht_protocol.rl'])

    bld.shlib(
        features='c cshlib',
        source=sources,
        includes=['src', 'includes'],
        target='dht'
    )

    bld.stlib(
        features='c cstlib',
        source=sources,
        includes=['src', 'includes'],
        target='dht'
    )

    bld.program(
        features='c',
        source='test/test_bucket.c',
        includes=['src', 'includes'],
        use='dht',
        target='test_bucket',
        install_path=None
    )
