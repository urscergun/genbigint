@echo off

cd ..
if exist generated rmdir /s /q generated
mkdir generated
cd generated

for %%f in (128 192 256 320 384 448 512 1024 2048) do (
  echo Generating int%%f
  mkdir %%f
  cd %%f
  ..\..\vstudio\x64\Release\genbigint %%f
  cd ..
)

cd ..\cmd
