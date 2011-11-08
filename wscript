import Options
from os import unlink, symlink, popen

srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')

  #conf.check_cfg(package='poppler', args='--cflags --libs', mandatory=True)
  conf.check_cfg(package='poppler-glib', args='--cflags --libs', mandatory=True)
  conf.check_cfg(package='gdk-3.0', args='--cflags --libs', mandatory=True)

def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'poppler'
  obj.source = 'NodePopplerDocument.cc NodePopplerPage.cc'
  obj.uselib = ['GDK-3.0', 'POPPLER-GLIB']
