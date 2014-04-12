#!/usr/bin/env python


def options(opt):
    opt.load('compiler_c')


def configure(conf):
    conf.load('compiler_c')
    conf.env.append_unique('CFLAGS', ['-std=c99', '-Wall', '-Wextra', '-Werror'])


def build(bld):
    sources = bld.path.ant_glob(['src/*.c'])
    includes = bld.path.ant_glob('includes/*.c')
    bld.shlib(
        features='c cshlib',
        source=sources,
        includes=includes,
        target='dht'
    )

    bld.stlib(
        features='c cstlib',
        source=sources,
        includes=includes,
        target='dht'
    )
