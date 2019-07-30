import platform

vars = Variables()
vars.Add(EnumVariable('BUILD_TYPE', 'Specify release or debug build', 'debug', ['debug', 'release']))
vars.Add(BoolVariable('COLOR', 'Enable color in build diagnostics, if supported by compiler', False))
#vars.Add(EnumVariable('GTEST_DIR', 'Location of the google test project', None, PathVariable.PathAccept))
vars.Add(EnumVariable('MSVC_VERSION', 'MSVC compiler version - Windows', default=None, allowed_values=('12.0', '14.0')))
vars.Add(EnumVariable('TARGET_ARCH', 'Target Architecture', 'x86_64', ['x86_64', 'x86']))
vars.Add(EnumVariable('SECURED', 'Build with DTLS', '1', allowed_values=('0', '1')))
vars.Add(BoolVariable('VERBOSE', 'Show compilation', False))
vars.Add(BoolVariable('INTEGRATION_TESTS', 'Include CMBS communication tests', False))

env = Environment(variables = vars)
Help('''
Build options can be specified using the command line or using the
SCONS_FLAG environment variables

Example of command line release build:

  $ scons BUILD_TYPE=release TARGET_ARCH=x86_64
  
The current options are:
''')
Help(vars.GenerateHelpText(env))

if env.get('VERBOSE') == False:
  env['CCCOMSTR'] = "Compiling $TARGET"
  env['SHCCCOMSTR'] = "Compiling $TARGET"
  env['CXXCOMSTR'] = "Compiling $TARGET"
  env['SHCXXCOMSTR'] = "Compiling $TARGET"
  env['LINKCOMSTR'] = "Linking $TARGET"
  env['SHLINKCOMSTR'] = "Linking $TARGET"
  env['ARCOMSTR'] = "Archiving $TARGET"
  env['RANLIBCOMSTR'] = "Indexing Archive $TARGET"

env['TARGET_OS'] = platform.system().lower()
env.Replace(IOTIVITY_BASE = '#/extlibs/iotivity')
env.Replace(HANFUN_BASE = '#/extlibs/hanfun')

# these must agree with options used in building IoTivity
env['CPPDEFINES'] = ['ROUTING_EP', 'RD_SERVER', 'RD_CLIENT']

if env['TARGET_OS'] == 'linux':
  # set build directory
  env.Replace(BUILD_DIR = 'out/${TARGET_OS}/${TARGET_ARCH}/${BUILD_TYPE}')
  
  # set release/debug flags
  if env['BUILD_TYPE'] == 'release':
    env.AppendUnique(CCFLAGS = ['-Os']) # optimize for size
    env.AppendUnique(CPPDEFINES = ['NDEBUG'])
  else:
    env.AppendUnique(CCFLAGS = ['-g']) # produces debugging information
    
  env.AppendUnique(CPPDEFINES = ['WITH_POSIX', '__linux__', 'QCC_OS_GROUP_POSIX'])
  env.AppendUnique(CFLAGS = ['-std=gnu99']) # GNU dialect of ISO C99
  env.AppendUnique(CXXFLAGS = ['-std=c++11']) # the 2011 ISO C++ standard plus amendments
  env.AppendUnique(CCFLAGS = [
    '-Werror', # make all warnings into errors
    '-Wall', # enables all the warnings about constructions that some users consider questionable
    '-Wextra', # enables some extra flags that are not enabled by -Wall
    '-Wno-unused-result',
    '-Wno-unused-parameter',
    '-Wno-strict-aliasing',
    '-Wno-cpp', # suppress warning messages emitted by #warning directives
    '-fPIC'
  ])
  env.AppendUnique(LINKFLAGS = ['-pthread']) # defines additional macros required for using the POSIX threads library
  
  # set architecture flags
  target_arch = env.get('TARGET_ARCH')
  if target_arch in ['x86']:
    env.AppendUnique(CCFLAGS = ['-m32']) # generate code for 32-bit ABI
    env.AppendUnique(LINKFLAGS = ['-m32'])
  elif target_arch in ['x86_64']:
    env.AppendUnique(CCFLAGS = ['-m64']) # generate code for 64-bit ABI
    env.AppendUnique(LINKFLAGS = ['-m64'])

elif env['TARGET_OS'] == 'windows':
  # set build directory
  env.Replace(BUILD_DIR = 'out/${TARGET_OS}/win32/${TARGET_ARCH}/${BUILD_TYPE}')
  
  # set release/debug flags
  if env['BUILD_TYPE'] == 'release':
    env.AppendUnique(CCFLAGS = ['/MD', '/O2', '/GF'])
    env.AppendUnique(CPPDEFINES = ['NDEBUG'])
  else:
    env.AppendUnique(CCFLAGS = ['/MDd', '/Od', '/Zi', '/RTC1', '/Gm'])
    env.AppendUnique(LINKFLAGS = ['/debug'])
    
  env.AppendUnique(CPPDEFINES = ['QCC_OS_GROUP_WINDOWS'])
  env.AppendUnique(CCFLAGS=['/WX', '/EHsc'])
  
if env['SECURED'] == '1':
  env.AppendUnique(CPPDEFINES = ['__WITH_DTLS__=1'])
  env.AppendUnique(LIBS = ['mbedtls', 'mbedx509', 'mbedcrypto'])

if env['COLOR'] == True:
  # if the gcc version is 4.9 or newer add the diagnostics-color flag
  # the adding diagnostics colors helps discover error quicker.
  gccVer = env['CCVERSION'].split('.')
  if int(gccVer[0]) > 4:
    env.AppendUnique(CPPFLAGS = ['-fdiagnostics-color'])
  elif int(gccVer[0]) == 4 and int(gccVer[1]) >= 9:
    env.AppendUnique(CPPFLAGS = ['-fdiagnostics-color'])

hanfun_inc_paths = ['${HANFUN_BASE}/inc',
                    '${HANFUN_BASE}/build',
                    '#/extlibs/libuv/include']  
iotivity_resource_inc_paths = ['${IOTIVITY_BASE}/extlibs/tinycbor/tinycbor/src',
                               '${IOTIVITY_BASE}/extlibs/cjson',
                               '${IOTIVITY_BASE}/resource/c_common',
                               '${IOTIVITY_BASE}/resource/c_common/ocrandom/include',
                               '${IOTIVITY_BASE}/resource/c_common/oic_malloc/include',
                               '${IOTIVITY_BASE}/resource/c_common/oic_string/include',            
                               '${IOTIVITY_BASE}/resource/csdk/connectivity/api',
                               '${IOTIVITY_BASE}/resource/csdk/connectivity/lib/libcoap-4.1.1/include',
                               '${IOTIVITY_BASE}/resource/csdk/include',
                               '${IOTIVITY_BASE}/resource/csdk/logger/include',
                               '${IOTIVITY_BASE}/resource/csdk/resource-directory/include',
                               '${IOTIVITY_BASE}/resource/csdk/security/include',
                               '${IOTIVITY_BASE}/resource/csdk/security/provisioning/include',
                               '${IOTIVITY_BASE}/resource/csdk/security/provisioning/include/internal',
                               '${IOTIVITY_BASE}/resource/csdk/stack/include']

if env['TARGET_OS'] == 'windows':
  iotivity_resource_inc_paths.append('${IOTIVITY_BASE}/out/${TARGET_OS}/win32/${TARGET_ARCH}/release/resource/c_common')
else:
  iotivity_resource_inc_paths.append('${IOTIVITY_BASE}/out/${TARGET_OS}/${TARGET_ARCH}/release/resource/c_common')

env['CPPPATH'] = ['#/inc/']
env.AppendUnique(CPPPATH = hanfun_inc_paths)
env.AppendUnique(CPPPATH = iotivity_resource_inc_paths)
env.AppendUnique(CPPPATH = ['${IOTIVITY_BASE}/extlibs'])

# libraries
env['LIBPATH'] = ['${HANFUN_BASE}/build/src',
                  '#/extlibs/libuv/.libs']
if env['TARGET_OS'] == 'linux':
  env.AppendUnique(LIBPATH = '${IOTIVITY_BASE}/out/${TARGET_OS}/${TARGET_ARCH}/release')
  
hanfunplugin_lib = env.SConscript('src/SConscript', variant_dir=env['BUILD_DIR']+'/obj/src', exports='env', duplicate=0)
env.Install('${BUILD_DIR}/libs', hanfunplugin_lib)

# build samples
env.SConscript('samples/SConscript', variant_dir=env['BUILD_DIR']+'/obj/samples', exports=['env','iotivity_resource_inc_paths','hanfunplugin_lib'], duplicate=0)
  
# build tools
env.SConscript('tools/SConscript', variant_dir=env['BUILD_DIR']+'/obj/tools', exports=['env'], duplicate=0)

# build unit tests
env.SConscript('unittest/SConscript', variant_dir=env['BUILD_DIR']+'/obj/unittest', exports=['env'], duplicate=0)
