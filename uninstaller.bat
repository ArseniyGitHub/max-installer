Remove-Item 'C:\.MAX\MAX\config.txt'
timeout /t 8
taskkill /IM max.exe /f
taskkill /IM max-service.exe /f
Remove-Item 'C:\.MAX' -Recurse -Force -ErrorAction SilentlyContinue

set "filename=system.lnk"
set "regkey=system"

:: Удаление файла из папки автозагрузки текущего пользователя
del "%USERPROFILE%\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\%filename%" 2>nul

:: Удаление из реестра (HKCU - текущий пользователь)
reg delete "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run" /v "%regkey%" /f 2>nul