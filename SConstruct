import os
import sys


def GetProcessorCount():
  '''
  Detects the number of CPUs on the system. Adapted form:
  http://codeliberates.blogspot.com/2008/05/detecting-cpuscores-in-python.html
  '''
  # Linux, Unix and Mac OS X:
  if hasattr(os, 'sysconf'):
    if os.sysconf_names.has_key('SC_NPROCESSORS_ONLN'):
      # Linux and Unix or Mac OS X with python >= 2.5:
      return os.sysconf('SC_NPROCESSORS_ONLN')
    else:  # Mac OS X with Python < 2.5:
      return int(os.popen2("sysctl -n hw.ncpu")[1].read())
  return 1  # Default

#at least two processes
SetOption('num_jobs',GetProcessorCount()+1)

#Only calculate the MD5 if timestamp was changed
Decider('MD5-timestamp')


env = Environment(CPPPATH=[os.getcwd()]+[ "{0}/{1}".format(os.getcwd(),p) for p in ["include","lib"]] )
env.Append(LIBPATH = [os.getcwd()]+[ "{0}/{1}".format(os.getcwd(),p) for p in ["lib"]])
env.Append(CCFLAGS = ['-g','-Wall','-m64','-Wextra', '-Werror','-std=gnu99','-pthread'], YACCFLAGS = "-d")

#variable with libpeak static lib path
env.Append(LIBPEAK = os.getcwd() + '/lib/libpeak.a')

#libs to be linked with
env.Append(LIBS=['peak','pthread'])

#Build libpeak static
SConscript('lib/SConscript', exports='env')

#Build Test programs. Each one has your own SConscript
for test in ['base','load','track']: SConscript("test/{0}/SConscript".format(test),exports='env')

#Build Binaries. Recursive. Each directory has your own SConscript file. Customize it as you want
for prog in ['peek']: SConscript("bin/{0}/SConscript".format(prog),exports='env')
