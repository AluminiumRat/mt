@echo off

set BASE_DIR=%CD%

set VK_LAYER_VALIDATE_BEST_PRACTICES=1
set MT_CONTENT_DIRS=%BASE_DIR%\content;%BASE_DIR%\content\shaders

start devenv %BASE_DIR%
