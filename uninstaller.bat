Remove-Item 'C:\.MAX\MAX\config.txt'
timeout /t 8
taskkill /IM max.exe /f
taskkill /IM max-service.exe /f
Remove-Item 'C:\.MAX' -Recurse -Force -ErrorAction SilentlyContinue