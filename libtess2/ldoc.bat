SET project=%~dp0
SET parent=%project%..\DocStuff

robocopy %parent% %project% "doc.css"

lua %parent%\LDoc\ldoc.lua libtess2.lua

del %project%\doc.css

set project=
set parent=

pause
