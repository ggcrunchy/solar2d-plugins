SET project=%~dp0
SET parent=%project%..\DocStuff
SET dir=%project%\plugin

robocopy %parent% %dir% "doc.css"
robocopy %project% %dir% "config.ld"

lua %parent%\LDoc\ldoc.lua %dir%\StableArray.lua

del %dir%\doc.css %dir%\config.ld

set project=
set parent=
set dir=

pause
