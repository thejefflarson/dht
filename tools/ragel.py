#! /usr/bin/env python

"Ragel: '.rl' files are converted into .cc files using 'ragel'"

from waflib import Task
from waflib.TaskGen import extension, after_method

class ragel(Task.Task):
    color = 'BLUE'
    run_str = '${RAGEL} ${RAGELFLAGS} -o ${TGT[0].abspath()} ${SRC[0].abspath()}'
    ext_out = ['.c']


def configure(conf):
    conf.find_program('ragel', var="RAGEL")
    # TODO: fix -I
    conf.env.RAGELFLAGS = ['-C', '-T1', '-I..']


@extension('.rl')
def big_ragel(self, node):
    out = node.change_ext('.c')
    self.create_task('ragel', node, out)
    self.source.append(out)