/**
Copyright 2016 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/
attribute vec3 aVertex;
attribute vec3 aNormal;

uniform mat4 uProjectionMatrix;
uniform mat4 uModelviewMatrix;
uniform mat4 uModelMatrix;
uniform mat3 uNormalMatrix;

//The following values frame depends on whether bump mapping is enabled
varying vec3 vNormal_WS;
varying vec3 vFragPos_WS;

void main()
{
   //Calc position and pass tex coord
   gl_Position = uProjectionMatrix * uModelviewMatrix * vec4(aVertex, 1.0f);

   vFragPos_WS = vec3(uModelMatrix * vec4(aVertex, 1.0f));
   vNormal_WS = normalize(uNormalMatrix * aNormal); 
}