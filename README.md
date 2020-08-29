# ESP_Config

A utility program to create QtCreator.pro and .pro.user files for esp-idf projects.

I've been programming with Qt for many years and love the Qt IDE, QtCreator. Recently, I have been working on programming some esp32 hardware based projects. The IDE that seems to be prevalent among developers programming for ESP32 devices is Eclipse. I found that Eclipse was difficult to setup for these project and generally is lacking compared to QtCreator. Then, I fond this project, [Template esp-idf project for Qt Creator](https://github.com/ascii78/esp-template-qtcreator). The author of this project provides a template and instructions for setting up an esp-idf proect to use QtCreator. I was favorably impressed with what could be done but I figure that the process could be improved and made even more plug-and-play. 

The answer is this Qt program which upon given the Makefile for an esp-idf project, creates the project and project-user files. It automatically finds all the INCLUDEPATH entries and also creates a list of SOURCE files. 

All you need to do is to download this project and compile it. I have developed it on Qt version 5.11.3. Once you have the progrram compiled, to setup an esp-idf project do the following. 
1. Start ESP_Config.
2. Select menu entry File -> Makefile to locate the Makefile for your esp-idf project. 
3. Select menu entry File -> Save to write out the project's .pro file.
4. Before opening the project in QtCreator, you also need to create a /pro.user file with menu entry File -> Create user file. 
5. Open the .pro file created in QtCreator. 

# Setting up Build Options
In order to be able to build the program in Qt Creator, some changes need to be made to the project's Build settings.
From the left sidebar select *Projects* then *Build. 
![IMAGE]{BuildSettings.png "Build Settings")
1. Set the *QTDIR* environment variable to blank.
2. Edit the *PATH* environment variable to:
	a. Remove any reference to the QT path.
	b. Add the location of the C++ compiler, in my case, it is "/home/allen/.espressif/tools/xtensa-esp32-elf/esp-2020r2-8.2.0/xtensa-esp32-elf/bin"
3. If the first builds step is "qmake ...", delete it. Is should then look like the image, above. You nay optionally set "Make arguments" to "flash" if desired. 

Now you should be able to select "build: from the menu and compile (an optionally flas) the project. 

# Note:
I am currently using QtCreator version 4.6.0 since it is the last one that uses Clang which I found to be buggy.
