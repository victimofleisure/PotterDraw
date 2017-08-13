@echo off
echo remove all temporary files from project folder?
pause
del *.sdf
attrib -h *.suo
del *.suo
rd /q /s Debug
rd /q /s Release
rd /q /s x64
rd /q /s PotterDraw\Debug
rd /q /s PotterDraw\Release
rd /q /s PotterDraw\x64
rd /q /s ipch
