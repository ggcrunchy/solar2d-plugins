@echo off
set PATH=%PATH%;"%USERPROFILE%\AppData\Local\Android\Sdk\ndk-bundle"

ndk-build APP_OPTIM="release"
