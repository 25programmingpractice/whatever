Unicode true
!include "MUI2.nsh"

SetCompressor /SOLID lzma
RequestExecutionLevel admin

!define VERSION "1.0.0"

!define MUI_ICON "assets\material-symbols-music-cast-rounded.ico"
!define MUI_UNICON "assets\material-symbols-music-cast-rounded.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "assets\material-symbols-music-cast-rounded.bmp"
!define MUI_HEADERIMAGE_BITMAP_STRETCH "AspectFitHeight"
!define MUI_WELCOMEFINISHPAGE_BITMAP "assets\material-symbols-music-cast-rounded.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP_STRETCH "NoStretchNoCropNoAlign"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "assets\material-symbols-music-cast-rounded.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP_STRETCH "NoStretchNoCropNoAlign"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE

!define MUI_LICENSEPAGE_CHECKBOX
!define MUI_LICENSEPAGE_CHECKBOX_TEXT "我已阅读并同意许可条款"
!define MUI_LICENSEPAGE_BUTTON "下一步(N) >"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN $INSTDIR\whatever.exe
!define MUI_FINISHPAGE_RUN_TEXT "运行 Whatever 播放器"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME ""
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME_TEXT "创建桌面快捷方式"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION desktopshortcut
!define MUI_FINISHPAGE_LINK "访问 Whatever GitHub 仓库"
!define MUI_FINISHPAGE_LINK_LOCATION https://github.com/25programmingpractice/whatever
!define MUI_FINISHPAGE_LINK_COLOR 1879e7
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH
!insertmacro MUI_LANGUAGE "SimpChinese"

Function desktopshortcut
    CreateShortcut "$DESKTOP\Whatever.lnk" "$INSTDIR\whatever.exe"
FunctionEnd

OutFile "Whatever_setup_win64.exe"
InstallDir "$APPDATA\Whatever"
Name "Whatever 播放器"
Caption "Whatever 安装程序"
BrandingText "Whatever (c) 2025 Lin Junming, Zhang Junming, Yi Zhihang"

VIAddVersionKey /LANG=${LANG_SimpChinese} "ProductName" "Whatever"
VIAddVersionKey /LANG=${LANG_SimpChinese} "CompanyName" "Lin Junming, Zhang Junming, Yi Zhihang"
VIAddVersionKey /LANG=${LANG_SimpChinese} "LegalCopyright" "(c) 2025 Lin Junming, Zhang Junming, Yi Zhihang"
VIAddVersionKey /LANG=${LANG_SimpChinese} "FileDescription" "Whatever 安装程序"
VIAddVersionKey /LANG=${LANG_SimpChinese} "FileVersion" "${VERSION}.0"
VIAddVersionKey /LANG=${LANG_SimpChinese} "ProductVersion" "${VERSION}.0"
VIProductVersion "${VERSION}.0"

Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    File /r ".\build\build\*.*"

    CreateShortcut "$SMPROGRAMS\Whatever.lnk" "$INSTDIR\Whatever.exe"

    WriteUninstaller "$INSTDIR\uninstall.exe"

    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever" "DisplayName" "Whatever"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever" "DisplayVersion" "${VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever" "Publisher" "Lin Junming, Zhang Junming, Yi Zhihang"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever" "DisplayIcon" "$INSTDIR\Whatever.exe"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever" "NoRepair" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever" "NoModify" 1
SectionEnd

Section "Uninstall"
    RMDir /r "$INSTDIR"
    Delete "$DESKTOP\Whatever.lnk"
    Delete "$SMPROGRAMS\Whatever.lnk"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Whatever"
SectionEnd