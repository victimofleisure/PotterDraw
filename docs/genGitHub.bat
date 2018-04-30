@echo off
copy C:\Chris\MyProjects\PotterDraw\web\*.* .
copy downloadGitHub.html download.html
copy galleryGitHub.html gallery.html
C:\Chris\MyProjects\tbl2web\release\tbl2web "..\PotterDraw ToDo.txt" issues.html issues.txt "PotterDraw Issues"
if errorlevel 1 goto err
C:\Chris\tools\navgen templateGitHub.html .
if errorlevel 1 goto err
C:\Chris\MyProjects\FixSelfUrl\Release\FixSelfUrl *.html
if errorlevel 1 goto err
md Help
del /s Help\*.htm
C:\Chris\MyProjects\doc2web\release\doc2web /nospaces C:\Chris\MyProjects\PotterDraw\Help\help.txt Help Contents.htm C:\Chris\MyProjects\PotterDraw\info\PotterDrawHelp.htm "PotterDraw Help"
if errorlevel 1 goto err
cd Help
md images
del /y images\*.*
copy C:\Chris\MyProjects\PotterDraw\Help\images\*.* images
if errorlevel 1 goto err
copy ..\helptopic.css content.css
C:\Chris\tools\navgen C:\Chris\MyProjects\PotterDraw\Help\template.txt .
copy ..\helpheader.txt x
copy x + Contents.htm
echo ^<body^>^<html^> >>x
del Contents.htm
ren x Contents.htm
md printable
cd printable
move C:\Chris\MyProjects\PotterDraw\info\PotterDrawHelp.htm .
ren PotterDrawHelp.htm prnhelp.htm
echo y | C:\Chris\tools\fsr prnhelp.htm "../../../images/" "https://victimofleisure.github.io/PotterDraw/Help/images/"
echo y | C:\Chris\tools\fsr prnhelp.htm "../../images/" "https://victimofleisure.github.io/PotterDraw/Help/images/"
echo y | C:\Chris\tools\fsr prnhelp.htm "../images/" "https://victimofleisure.github.io/PotterDraw/Help/images/"
ren prnhelp.htm PotterDrawHelp.htm
cd ..
cd ..
ren issues.html issues.htm
echo y | C:\Chris\tools\fsr issues.htm "<div id=body>" "<div id=widebody>"
ren issues.htm issues.html
ren links.html links.htm
echo y | C:\Chris\tools\fsr links.htm "\"http://chordease.sourceforge.net/\"" "\"https://victimofleisure.github.io/ChordEase/\""
echo y | C:\Chris\tools\fsr links.htm "\"http://ffrend.sourceforge.net/\"" "\"https://victimofleisure.github.io/FFRend/\""
echo y | C:\Chris\tools\fsr links.htm "\"http://fractice.sourceforge.net/\"" "\"https://victimofleisure.github.io/Fractice/\""
echo y | C:\Chris\tools\fsr links.htm "\"http://mixere.sourceforge.net/\"" "\"https://victimofleisure.github.io/Mixere/\""
echo y | C:\Chris\tools\fsr links.htm "\"https://polymeter.sourceforge.io/\"" "\"https://victimofleisure.github.io/Polymeter/\""
echo y | C:\Chris\tools\fsr links.htm "\"http://potterdraw.sourceforge.net/\"" "\"https://victimofleisure.github.io/PotterDraw/\""
echo y | C:\Chris\tools\fsr links.htm "\"http://sourceforge.net/projects/triplight/\"" "\"https://github.com/victimofleisure/TripLight/\""
echo y | C:\Chris\tools\fsr links.htm "\"http://waveshop.sourceforge.net/\"" "\"https://victimofleisure.github.io/WaveShop/\""
echo y | C:\Chris\tools\fsr links.htm "\"http://whorld.sourceforge.net/\"" "\"https://victimofleisure.github.io/Whorld/\""
echo y | C:\Chris\tools\fsr links.htm "\"http://whorld.org/\"" "\"https://victimofleisure.github.io/Whorld/\""
ren links.htm links.html
goto exit
:err
pause Error!
:exit
