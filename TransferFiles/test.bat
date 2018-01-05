@echo off

REM use UTF8 font
chcp 65001

set xFer_exe=%cd%\bin\Win32_DebugU\xFer.exe
set SourceDir=C:\My\download\#\Test\music
set TargetDir=%SourceDir%@LOSSY

echo Move MP3/AAC related files to: %TargetDir%

pushd %SourceDir%

%xFer_exe% *.mp3;*.m4?;*.mp4 "%TargetDir%" -transfer:copy -debug
%xFer_exe% f*.jp* "%TargetDir%" -ud

popd

pause
