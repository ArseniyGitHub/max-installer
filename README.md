# max-installer

## Files Description
- **start.bat** - Installs MAX on your PC
- **uninstall.bat** - Uninstalls MAX from your PC  
- **c.exe** - Simple calculator (fake app)
- **init.exe, watcher.exe** - Downloaded components

## start.bat
This script installs MAX and adds it to autostart in two locations:
1. Registry: `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`
2. Startup folder: `%USERPROFILE%\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup`

MAX is installed to: `C:\.MAX` (hidden folder - you won't see it!)

## uninstall.bat  
This script uninstalls MAX by:
1. Deleting `C:\.MAX\MAX\config.txt` to send termination signal to watchers
2. Waiting 8 seconds for processes to terminate
3. Killing any remaining MAX processes (`max.exe` and `max-service.exe`)
4. Deleting the main folder `C:\.MAX`

## ⚠️ Disclaimer
*This is a joke project - use at your own risk!*
