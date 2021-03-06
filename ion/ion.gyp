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
# Declares demos, all_public_libraries and all_tests aggregate targets.
#
# The targets in this file are here just to serve as groupings, so that "all of
# Ion" can be built by pointing gyp to this file. Do NOT depend on the targets
# in this file to build your Ion-dependent thing, point to the individual Ion
# library targets you need.
{
  'includes': [
    'common.gypi',
  ],

  'targets': [
    {
      'target_name': 'Snapshot',
      'type': 'none',
      'conditions': [
        ['OS != "qnx"', {
          'dependencies' : [
            '../Snapshot/Snapshot.gyp:*',
          ],
        }],
      ],  # conditions
    },    
  ],
}
