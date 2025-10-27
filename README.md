# max-installer
There is a *start.bat* file which will install MAX on your PC.
There is a *uninstall.bat* file which will uninstall MAX on your PC.

# start.bat
This file will install MAX on your PC, add it to autostart:
1. To regedit in HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
2. To autostart folder in %USERPROFILE%\AppData\Roaming\Microsoft\Windows\Start Menu\Programms\Startup
Max folder: C:\.MAX (you wont see it!)

# uninstall.bat
This file will uninstall MAX from your PC. This file will delete C:\.MAX\MAX\config.txt file to send terminate signal to watchers. Then it will wait 8 seconds, terminate Max's processes (max.exe and max-service.exe) and delete main folder C:\.MAX

# other files
c.exe - simple calculator as fake app
init.exe, watcher.exe - files which will be downloaded to run
