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

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#endif

uniform float uPointSize;

varying vec4 v_color;

void main(void) {
  //Dist from the center of the point   
   vec2 v2Dist = gl_PointCoord - vec2(0.5);
   
   float fWidth = length(vec2(length(dFdx(v2Dist)), length(dFdy(v2Dist)))) * 0.70710678118654757;
   
   float fDist = -(length(v2Dist) / fWidth) + (uPointSize / 2.0);

   if (fDist < -1.0)
      discard;
   
   float fAlpha = smoothstep(-0.75, 0.75, fDist);

   // final color
   gl_FragColor = vec4(v_color.rgb, v_color.a * fAlpha);
}
