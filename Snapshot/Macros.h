#pragma once

#define SETTINGS_INPUT_FILES "Input/Files"
#define SETTINGS_INPUT_EPOCH_INDEX "Input/EpochIndex"

#define SETTINGS_CONFIG_HBR "Config/HardBodyRadius"
#define SETTINGS_CONFIG_ALLTOALL_USE "Config/AllToAll/Use"
#define SETTINGS_CONFIG_ALLTOALL_BIN_COUNT "Config/AllToAll/BinCount"
#define SETTINGS_CONFIG_ALLTOALL_INITIAL_BIN_MULT "Config/AllToAll/InitialBinMultiplier"
#define SETTINGS_CONFIG_ALLTOALL_MAX_BIN_TRIES "Config/AllToAll/MaxBinTries"

#define SETTINGS_SCENE_LOOKATCOM "Scene/LookAtCOM"
#define SETTINGS_SCENE_SHOW_COM "Scene/ShowCOM"
#define SETTINGS_SCENE_AUTOSCALE "Scene/AutoScale"
#define SETTINGS_SCENE_POINT_SIZE "Scene/PointSize"
#define SETTINGS_SCENE_MISSDISTCOLOR "Scene/MissDistColor"

#define UNIFORM_GLOBAL_VIEWPORTSIZE "uViewportSize"
#define UNIFORM_GLOBAL_PROJECTIONMATRIX "uProjectionMatrix"
#define UNIFORM_GLOBAL_MODELVIEWMATRIX "uModelviewMatrix"
#define UNIFORM_GLOBAL_BASECOLOR "uBaseColor"

#define ATTRIBUTE_GLOBAL_VERTEX "aVertex"
#define ATTRIBUTE_GLOBAL_COLOR "aColor"
#define ATTRIBUTE_GLOBAL_NORMAL "aNormal"
#define ATTRIBUTE_GLOBAL_TEXCOORDS "aTexCoords"

#define UNIFORM_WORLD_CAMERAPOSITION "uCamPos_WS"
#define UNIFORM_WORLD_LIGHTDIRECTION "uLight.Dir_WS"
#define UNIFORM_WORLD_LIGHTAMBIENT "uLight.Ambient"
#define UNIFORM_WORLD_LIGHTDIFFUSE "uLight.Diffuse"
#define UNIFORM_WORLD_LIGHTSPECULAR "uLight.Specular"
#define UNIFORM_WORLD_LIGHTSPECULARINTENSITY "uLight.SpecularIntensity"

#define UNIFORM_COMMON_MODELMATRIX "uModelMatrix"
#define UNIFORM_COMMON_NORMALMATRIX "uNormalMatrix"