# Radeon ProRender Maya Plugin

Development requires Maya 2016 or later.

Dependencies
============

External dependencies must be included in the repository's ThirdParty directory, with the exception of the Maya SDK, see next section. Please check the README in the ThirdParty directory to see  how to acquire the required libraries.

Please contact a Radeon ProRender AMD developer to find out how to get access to this repository.

Environment Variables
=====================

The following environment variables are assumed defined, you will probably need to add these (Maya defines none).
If you are not building the installer you do not need all versions.

	MAYA_x64_2015
	MAYA_SDK_2016
	MAYA_x64_2016
	MAYA_SDK_2016_5
	MAYA_x64_2016_5
	MAYA_SDK_2017
	MAYA_x64_2017
	MAYA_SDK_2018
	MAYA_x64_2018

eg:
	set MAYA_x64_2015=C:\Program Files\Autodesk\Maya2015
	set MAYA_SDK_2016=E:\SDK\Autodesk\Maya\2016.3\devkitBase
	set MAYA_x64_2016=C:\Program Files\Autodesk\Maya2016
	set MAYA_SDK_2016_5=E:\SDK\Autodesk\Maya\2016.5\devkitBase
	set MAYA_x64_2016_5=C:\Program Files\Autodesk\Maya2016.5
	set MAYA_SDK_2017=E:\SDK\Autodesk\Maya\2017\devkitBase
	set MAYA_x64_2017=C:\Program Files\Autodesk\Maya2017
	set MAYA_SDK_2018=:\Program Files\Autodesk\Maya2018
	set MAYA_x64_2018=C:\Program Files\Autodesk\Maya2018

You may require a reboot of the machine to get the environment variables to be recognized by Developer Studio.

Running/Debugging the Build on Windows
======================================

In order to run/debug with FR Renderer Maya needs to find the plugin build products and other associated files.

Set a system environment variable showing location of frMaya repository folder
	SET FR_MAYA_PLUGIN_DEV_PATH=<frMaya root folder>
eg:
	set FR_MAYA_PLUGIN_DEV_PATH=E:\Dev\AMD\frMaya

Copy the frMaya.module file to:
	%COMMONPROGRAMFILES%\Autodesk Shared\Modules\maya\2016
	%COMMONPROGRAMFILES%\Autodesk Shared\Modules\maya\2016.5
	%COMMONPROGRAMFILES%\Autodesk Shared\Modules\maya\2017
	etc

Note that it is named intentionally named differently to installer RadeonProRender.module files to avoid conflicts.

Useful Info
===========

Most Mel commands for Maya, can be found here:
	C:\Program Files\Autodesk\Maya2016\scripts\startup\defaultRuntimeCommands.mel
Shelfs can be easily created from the Shelf editor within Maya application.

In order to setup a UI with attribute (for example into render settings):
-create attribute in Mel script with correct hierarchy setup (for Example check out: createFireREnderGlobalsTab.mel file)
-declare attribute in RadeonProRenderGlobals
-write initialization code
-write attribute data in FireRender.xml (not sure if this is actually needed)
-Read attribute's value in FireRenderUtils->readFromCurrentScene(bool)

Shelfs
======
Copy the shelf_Radeon-ProRender.mel file into "...\Documents\maya\2016\prefs\shelves"

Linux
=====

Setup for RPR Maya: https://docs.google.com/document/d/1OZvYWZ5IZn2o4uUcKNBsGC9KxBMPHYdys_exYM-vieg/edit

Setup for developer environment:

First follow setup for Maya and RPR plugin above.

Install following packages (sudo yum install):
cmake
devtoolset-6
ncurses-devl
libX11, libX11-devel, qt-x11, libXt, libXt-devel
mesa-libGL-devel
mesa-libGLU-devel
OpenImageIO-devel
glew-devel
pciutils-devel
libdrm-devel
makeself

Please install Maya 2016, 2016 ext 2 (2016.5 - it will keep 2016), 2017.
After that download 2016 dev kit from:
https://github.com/autodesk-adn/Maya-devkit
Copy content of Linux sub-dir to 2016 installed directory.

After installing these, open the shell and go to:
cd fireRenderLinux
./cmake_clean.sh
(check there are no errors in the output)
And after that use:
./make_dist_debug.sh or ./make_dist.sh or ./make_installer.sh

OSX
===

Please check the Readme.txt in the FireRender.Maya.OSX directory.

Versioning
==========

The version number of the plugin should be increased when releasing a new version of the plugin.  This is done by
changing the PLUGIN_VERSION define in the FireRender.Maya.Src\common.h file.

