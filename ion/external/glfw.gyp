#
# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS-IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file only gets included on Windows and Linux.

{
  'includes' : [
    '../common.gypi',
    'external_common.gypi',
  ],
  'variables' : {
    'glfw_rel_dir' : '../../third_party/glfw',
  },
  'targets' : [
    {
      'target_name' : 'glfw',
      'type': 'static_library',
      'dependencies': [
        'external.gyp:graphics',
      ],
      'sources' : [
        '<(glfw_rel_dir)/src/glfw_config.h',
        '<(glfw_rel_dir)/include/GLFW/glfw3.h',
        '<(glfw_rel_dir)/include/GLFW/glfw3native.h',
        '<(glfw_rel_dir)/src/context.c',
        '<(glfw_rel_dir)/src/init.c',
        '<(glfw_rel_dir)/src/input.c',
        '<(glfw_rel_dir)/src/monitor.c',
        '<(glfw_rel_dir)/src/vulkan.c',
        '<(glfw_rel_dir)/src/window.c',
      ],
      'include_dirs' : [
        'glfw',
        '<(glfw_rel_dir)/include',
        '<(glfw_rel_dir)/src',
      ],
      'all_dependent_settings' : {
        'include_dirs' : [
          'glfw',
          '<(glfw_rel_dir)/include',
          '<(glfw_rel_dir)/src',
        ],
      },  # all_dependent_settings
      'defines!': [
        # glfw copiously prints out every event when _DEBUG is defined, so
        # undefine it.
        '_DEBUG',
      ],
      'conditions': [
        ['OS in ["linux", "mac"]', {
          'cflags': [
            '-Wno-int-to-pointer-cast',
            '-Wno-pointer-to-int-cast',
          ],
          'all_dependent_settings': {
            'cflags_cc': [
              '-Wno-mismatched-tags',
            ],
          },
        }],
        ['OS in ["mac", "ios"]', {
          'xcode_settings': {
            'OTHER_CFLAGS': [
              '-Wno-conversion',
             ],
          },
        }],  # mac or ios
        ['OS=="mac"', {
          'include_dirs': [
            '/usr/X11/include',
          ],
          'sources': [
            '<(glfw_rel_dir)/src/cocoa_platform.h',
            '<(glfw_rel_dir)/src/cocoa_joystick.h',
            '<(glfw_rel_dir)/src/posix_tls.h',
            '<(glfw_rel_dir)/src/nsgl_context.h',
            '<(glfw_rel_dir)/src/cocoa_init.m',
            '<(glfw_rel_dir)/src/cocoa_joystick.m',
            '<(glfw_rel_dir)/src/cocoa_monitor.m',
            '<(glfw_rel_dir)/src/cocoa_window.m',
            '<(glfw_rel_dir)/src/cocoa_time.c',
            '<(glfw_rel_dir)/src/posix_tls.c',
            '<(glfw_rel_dir)/src/nsgl_context.m',
          ],
          'link_settings': {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/AGL.framework',
              '$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
              '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
              '/usr/X11/lib/libX11.dylib',
              '/usr/X11/lib/libGL.dylib',
              '/usr/X11/lib/libXext.dylib',
              '/usr/X11/lib/libXxf86vm.dylib',
              '/usr/X11/lib/libXrandr.dylib',
            ],
          },
          'defines': [
            # This prevents glfw from using its hacky font definitions on OSX.
            '_GLFW_COCOA',
            '_GLFW_USE_MENUBAR',
            '_GLFW_USE_CHDIR',
            '_GLFW_USE_RETINA',
            '__CYGWIN__',
            'HAVE_DLFCN_H',
            'HAVE_ERRNO_H',
            'HAVE_FCNTL_H',
            'HAVE_GETTIMEOFDAY',
            'HAVE_GL_GLU_H',
            'HAVE_GL_GLX_H',
            'HAVE_GL_GL_H',
            'HAVE_INTTYPES_H',
            'HAVE_LIMITS_H',
            'HAVE_MEMORY_H',
            'HAVE_STDINT_H',
            'HAVE_STDLIB_H',
            'HAVE_STRINGS_H',
            'HAVE_STRING_H',
            'HAVE_SYS_IOCTL_H',
            'HAVE_SYS_PARAM_H',
            'HAVE_SYS_STAT_H',
            'HAVE_SYS_TIME_H',
            'HAVE_SYS_TYPES_H',
            'HAVE_UNISTD_H',
            'HAVE_VFPRINTF',
            'HAVE_VPRINTF',
            'HAVE_X11_EXTENSIONS_XF86VMODE_H',
            'HAVE_X11_EXTENSIONS_XINPUT_H',
            'HAVE_X11_EXTENSIONS_XI_H',
            'HAVE_X11_EXTENSIONS_XRANDR_H',
            'STDC_HEADERS',
            'TIME_WITH_SYS_TIME',
          ],
        }],
        ['OS=="linux"', {
          'defines' : [
            'HAVE_DLFCN_H',
            'HAVE_ERRNO_H',
            'HAVE_FCNTL_H',
            'HAVE_GETTIMEOFDAY',
            'HAVE_GL_GLU_H',
            'HAVE_GL_GLX_H',
            'HAVE_GL_GL_H',
            'HAVE_INTTYPES_H',
            'HAVE_LIBXI',
            'HAVE_LIMITS_H'
            'HAVE_MEMORY_H',
            'HAVE_STDINT_H',
            'HAVE_STDINT_H',
            'HAVE_STDLIB_H',
            'HAVE_STRINGS_H',
            'HAVE_STRING_H',
            'HAVE_SYS_IOCTL_H',
            'HAVE_SYS_PARAM_H',
            'HAVE_SYS_STAT_H',
            'HAVE_SYS_TIME_H',
            'HAVE_SYS_TYPES_H',
            'HAVE_UNISTD_H',
            'HAVE_VFPRINTF',
            'HAVE_VPRINTF',
            'STDC_HEADERS',
          ],
          'link_settings': {
            'libraries': [
              '-lrt',  # For clock_gettime.
            ],
          },  # link_settings
        }],
        ['OS=="windows"', {
          'sources': [
            '<(glfw_rel_dir)/src/win32_platform.h',
            '<(glfw_rel_dir)/src/win32_joystick.h',
            '<(glfw_rel_dir)/src/wgl_context.h',
            '<(glfw_rel_dir)/src/egl_context.h',
            '<(glfw_rel_dir)/src/win32_init.c',
            '<(glfw_rel_dir)/src/win32_joystick.c',
            '<(glfw_rel_dir)/src/win32_monitor.c',
            '<(glfw_rel_dir)/src/win32_time.c',
            '<(glfw_rel_dir)/src/win32_tls.c',
            '<(glfw_rel_dir)/src/win32_window.c',
            '<(glfw_rel_dir)/src/wgl_context.c',
            '<(glfw_rel_dir)/src/egl_context.c',
          ],
          'link_settings': {
            'libraries': [
              '-lWinmm',  # For time stuff.
              '-lAdvapi32',  # For registry stuff.
            ],
          },  # link_settings
          'defines': [
            '_GLFW_WIN32',
            '_GLFW_USE_HYBRID_HPG',
          ],
          'defines!': [
            'NOGDI',
          ],
        }],
      ],
    },
  ],
}
