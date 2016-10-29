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

//Input varyings
varying vec3 vNormal_WS;
varying vec3 vFragPos_WS;

uniform vec3 uCamPos_WS;
uniform vec4 uBaseColor;

//Light
struct Light 
{
   vec3 Dir_WS;
   vec3 Ambient;
   vec3 Diffuse;
   vec3 Specular;
   float SpecularIntensity;
};

uniform Light uLight;

void main()
{     
   vec4 v4PrimaryColor = uBaseColor;

    // Ambient
    vec3 v3AmbientLight = uLight.Ambient;

    vec3 v3Normal = normalize(vNormal_WS);

    // Diffuse
    float fLambert = max(dot(uLight.Dir_WS, v3Normal), 0.0);

    vec3 v3DiffuseLight = uLight.Diffuse * vec3(fLambert);

    // Specular, using Blinn-Phong and 
    vec3 v3ViewDir = normalize(uCamPos_WS - vFragPos_WS);
    vec3 v3HalfVec = normalize(uLight.Dir_WS + v3ViewDir);  
    vec3 v3SpecularColor = uLight.Specular * vec3(clamp(pow(max(dot(v3Normal, v3HalfVec), 0.0), uLight.SpecularIntensity), 0.0, 1.0));

    v4PrimaryColor.rgb = (v3AmbientLight + v3DiffuseLight) * v4PrimaryColor.rgb + v3SpecularColor;

   gl_FragColor = v4PrimaryColor;
   gl_FragColor = vec4((v3AmbientLight + v3DiffuseLight) * v4PrimaryColor.rgb, 1.0);
}