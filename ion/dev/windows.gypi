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
# This files gets included only for windows targets.

{
  'includes': [
    '../common.gypi'
  ],
  'variables' : {
    'variables': {
      'windows_path_dirs_x86': [
        '<(msvc_dir)/vc/bin',
        '<(msvc_dir)/common7/IDE',
        '<(msvc_dir)/redist/x86/Microsoft.VC<(msvc_version).CRT',
        '<(msvc_redist_dir)/Debug_NonRedist/x86/Microsoft.VC<(msvc_version).DebugCRT',
        '<(msvc_redist_dir)/x86/Microsoft.VC<(msvc_version).CRT',
        '<(platformsdk_dir)/bin/x86',
      ],
      'windows_path_dirs_x64': [
        '<(msvc_dir)/vc/bin/amd64',
        '<(msvc_dir)/common7/IDE',
        '<(msvc_dir)/redist/x64/Microsoft.VC<(msvc_version).CRT',
        # Yes, the x64 paths include the x86 paths. This is because some tools
        # might be built with the default configuration (x86) and still need to
        # run under an x64 build.
        '<(msvc_redist_dir)/Debug_NonRedist/x86/Microsoft.VC<(msvc_version).DebugCRT',
        '<(msvc_redist_dir)/x86/Microsoft.VC<(msvc_version).CRT',
        '<(msvc_redist_dir)/Debug_NonRedist/x64/Microsoft.VC<(msvc_version).DebugCRT',
        '<(msvc_redist_dir)/x64/Microsoft.VC<(msvc_version).CRT',
        '<(platformsdk_dir)/bin/x64',
      ],
    },
    # The include dirs are the same for both architectures.
    'include_dirs_common': [
      '<(msvc_dir)/vc/include',
      '<(platformsdk_dir)/include/um',
      '<(platformsdk_dir)/include/shared',
      '<(platformsdk_dir)/include/winrt',
    ],

    'library_dirs_x86': [
      '<(msvc_dir)/vc/lib',
      '<(platformsdk_dir)/Lib/winv6.3/um/x86',
    ],
    'library_dirs_x64': [
      '<(msvc_dir)/vc/lib/amd64',
      '<(platformsdk_dir)/Lib/winv6.3/um/x64',
    ],

    'windows_path_dirs_x86%': '<(windows_path_dirs_x86)',
    'windows_path_dirs_x64%': '<(windows_path_dirs_x64)',
    'msvc_dir': '<!(<(python) -c "import os; print os.getenv(\'VSINSTALLDIR\') or os.path.realpath(\'<(third_party_dir)/msvc/files\')")',
    'platformsdk_dir': '<!(<(python) -c "import os; print os.getenv(\'WindowsSdkDir\') or os.path.realpath(\'<(third_party_dir)/windows_sdk_8_1/files\')")',
    'msvc_redist_dir': '<(msvc_dir)/vc/redist',
    'msvc_version': '<!(<(python) -c "import os; print os.getenv(\'VisualStudioVersion\').replace(\'.\', \'\') or \'120\'")',
  },
  'target_defaults': {
    'conditions': [
      # Ninja on windows uses environment files, which must exist before
      # anything else is built. See documentation in
      # generate_ninja_environment.gyp
      ['GENERATOR == "ninja"', {
        'dependencies': [
          '<(ion_dir)/dev/generate_ninja_environment.gyp:generate_ninja_environment',
        ],
      }],
    ],  # conditions
  },  # target_defaults
}
