@echo off
echo Installing OpenGD77 Serial driver...
"%~dp0wdi-simple" --vid 0x01FC9 --pid 0x0094 --type 3 --name "OpenGD77" --dest "%~dp0OpenGD77-serial-driver-files"
echo.
pause