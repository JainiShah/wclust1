# Microsoft Developer Studio Project File - Name="WClust" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WClust - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WClust.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WClust.mak" CFG="WClust - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WClust - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WClust - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WClust - Win32 Release"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /d "NDEBUG"
# ADD RSC /l 0x405 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "WClust - Win32 Debug"

# PROP BASE Use_MFC 5
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x405 /d "_DEBUG"
# ADD RSC /l 0x405 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "WClust - Win32 Release"
# Name "WClust - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\cdxCSizingDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\ChooseClDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Cluster.cpp
# End Source File
# Begin Source File

SOURCE=.\ColorComboBox.cpp
# End Source File
# Begin Source File

SOURCE=.\DialList.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgAddWCP.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgFileInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgPositionReg.cpp
# End Source File
# Begin Source File

SOURCE=.\DlgWcpNotFound.cpp
# End Source File
# Begin Source File

SOURCE=.\EditDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\EditSpikes.cpp
# End Source File
# Begin Source File

SOURCE=.\EEG.cpp
# End Source File
# Begin Source File

SOURCE=.\EEGStack.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\MyObj.cpp
# End Source File
# Begin Source File

SOURCE=.\SelectTT.cpp
# End Source File
# Begin Source File

SOURCE=.\SetDial.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\Sync.cpp
# End Source File
# Begin Source File

SOURCE=.\SyncStack.cpp
# End Source File
# Begin Source File

SOURCE=.\TimeEv.cpp
# End Source File
# Begin Source File

SOURCE=.\Vertex.cpp
# End Source File
# Begin Source File

SOURCE=.\WClust.cpp
# End Source File
# Begin Source File

SOURCE=.\WClust.rc
# End Source File
# Begin Source File

SOURCE=.\WClustDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\WClustView.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\cdxCSizingDialog.h
# End Source File
# Begin Source File

SOURCE=.\ChooseClDlg.h
# End Source File
# Begin Source File

SOURCE=.\Cluster.h
# End Source File
# Begin Source File

SOURCE=.\ClusteringInfoDlg.h
# End Source File
# Begin Source File

SOURCE=.\ColorComboBox.h
# End Source File
# Begin Source File

SOURCE=.\DialList.h
# End Source File
# Begin Source File

SOURCE=.\DlgAddWCP.h
# End Source File
# Begin Source File

SOURCE=.\DlgFileInfo.h
# End Source File
# Begin Source File

SOURCE=.\DlgPositionReg.h
# End Source File
# Begin Source File

SOURCE=.\DlgWcpNotFound.h
# End Source File
# Begin Source File

SOURCE=.\EditDlg.h
# End Source File
# Begin Source File

SOURCE=.\EditSpikes.h
# End Source File
# Begin Source File

SOURCE=.\EEG.h
# End Source File
# Begin Source File

SOURCE=.\EEGStack.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\MyObj.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SelectTT.h
# End Source File
# Begin Source File

SOURCE=.\SetDial.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\Sync.h
# End Source File
# Begin Source File

SOURCE=.\SyncStack.h
# End Source File
# Begin Source File

SOURCE=.\TimeEv.h
# End Source File
# Begin Source File

SOURCE=.\Vertex.h
# End Source File
# Begin Source File

SOURCE=.\WClust.h
# End Source File
# Begin Source File

SOURCE=.\WClustDoc.h
# End Source File
# Begin Source File

SOURCE=.\WClustView.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\00000.bmp
# End Source File
# Begin Source File

SOURCE=.\res\0000FF.bmp
# End Source File
# Begin Source File

SOURCE=.\res\009090.bmp
# End Source File
# Begin Source File

SOURCE=.\res\0090F0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\00F090.bmp
# End Source File
# Begin Source File

SOURCE=.\res\00F0F0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\00ff0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\00FF00.bmp
# End Source File
# Begin Source File

SOURCE=.\res\900090.bmp
# End Source File
# Begin Source File

SOURCE=.\res\9000F0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\909000.bmp
# End Source File
# Begin Source File

SOURCE=.\res\909090.bmp
# End Source File
# Begin Source File

SOURCE=.\res\9090F0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\90F000.bmp
# End Source File
# Begin Source File

SOURCE=.\res\90F090.bmp
# End Source File
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\CCCCCC.bmp
# End Source File
# Begin Source File

SOURCE=.\res\F00090.bmp
# End Source File
# Begin Source File

SOURCE=.\res\F000F0.bmp
# End Source File
# Begin Source File

SOURCE=.\res\F09000.bmp
# End Source File
# Begin Source File

SOURCE=.\res\F09090.bmp
# End Source File
# Begin Source File

SOURCE=.\res\F0F000.bmp
# End Source File
# Begin Source File

SOURCE=.\res\ff0000.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\WClust.ico
# End Source File
# Begin Source File

SOURCE=.\res\WClust.rc2
# End Source File
# Begin Source File

SOURCE=.\res\WClustDoc.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
