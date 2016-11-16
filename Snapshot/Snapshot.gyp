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
{
  'includes' : [
    '../ion/common.gypi',
  ],

  'variables': {
    # Used by make_into_app.gypi. We can define this once per target, or
    # globally here.
    'target_app_location_param': '<(PRODUCT_DIR)/test',
  },

  'targets': [

    {
      'target_name': 'Snapshot_assets',
      'type': 'static_library',
      'includes': [
        '../ion/dev/zipasset_generator.gypi',
      ],
      'dependencies': [
        '<(ion_dir)/port/port.gyp:ionport',
      ],
      'sources': [
        'Snapshot_assets.iad',
      ],
    },  # target: Snapshot_assets

    {
      'target_name': 'Snapshot',
      'includes': [ 'demobase.gypi', ],
      'variables': {
        'demo_class_name': 'Snapshot'
      },
      'sources': [
        'Camera.cpp',
        'Camera.hpp',
        'FileManager.cpp',
        'FileManager.hpp',
        'FinalAction.hpp',
        'Hud.cpp',
        'Hud.hpp',
        'IonFwd.h',
        'KeyboardHandler.cpp',
        'KeyboardHandler.hpp',
        'Macros.h',
        'main.cpp',
        'resource.h',
        'Scene.cpp',
        'Scene.hpp',
        'SceneBase.cpp',
        'SceneBase.hpp',
        'stdafx.cpp',
        'stdafx.h',
        'targetver.h',
        'Window.cpp',
        'Window.hpp',
      ],
      'dependencies': [
        ':Snapshot_assets',
        '<(ion_dir)/external/freetype2.gyp:ionfreetype2',
        '<(ion_dir)/external/glfw.gyp:glfw',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          'AdditionalDependencies': [
            '%(AdditionalDependencies)',
            ],
        },
        'VCCLCompilerTool': {
          'OpenMP': 'true',
          'EnableEnhancedInstructionSet': '3', # AdvancedVectorExtensions
        }
      },
    },  # target: Snapshot
    {
      'variables': {
        'make_this_target_into_an_app_param': 'Snapshot',
        'apk_class_name_param': 'Snapshot',
      },
      'includes': [
        'demo_apk_variables.gypi',
      ],
    },

    {
      'target_name': 'SnapshotInstaller',
      'type': 'wix_installer',
      'dependencies': [
        ':Snapshot',
      ],
      'sources': [
        './InstallSnapshot.wxs',
      ],
    },  # target: SnapshotInstaller
  ],
}
