@echo off

cd ..\generated

for /d %%f in (128 192 256 320 384 448 512 1024 2048) do (
  echo Building int%%f
  cd %%f
  msbuild test_int%%f.vcxproj -p:Configuration=Release
  msbuild test_cint%%f.vcxproj -p:Configuration=Release
  cd ..
)

cd ..\cmd
