@ECHO Off

if not exist *.cue (
    echo "CUE/ISO missing, please build first."
) else (
    @REM Finding first cue file and running it on yabause
    FOR %%F IN (*.cue) DO (
        "..\..\emulators\yabause\yabause.exe" -a -i %%F
        exit /b
    )
)
