@echo off

cd ..\generated

for /d %%f in (128 192 256 320 384 448 512 1024 2048) do (
  echo ++++++++++++++++++++++++++++++
  
  echo === Running test_int%%f ===
  %%f\x64\Release\test_int%%f.exe
  
  echo --- Running test_cint%%f ---
  %%f\x64\Release\test_cint%%f.exe
)

cd ..\cmd
