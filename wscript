#!/usr/bin/env python


def options(opt):
    opt.load('compiler_c')


def configure(conf):
    conf.load('compiler_c')
    conf.env.append_unique('CFLAGS', ['-std=c99', '-Wall', '-Wextra', '-Werror'])
    conf.env.append_value('INCLUDES', ['include'])


def build(bld):
    sources = bld.path.ant_glob(['src/*.c'])
    bld.shlib(
        features='c cshlib',
        source=sources,
        target='dht'
    )

    bld.stlib(
        features='c cstlib',
        source=sources,
        target='dht'
    )
