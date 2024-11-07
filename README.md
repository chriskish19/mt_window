<h1> Multithreaded win32 window example</h1>
<p>Small program that demonstrates how you might go about running multiple windows on seperate threads each with its own win32 message pump. Painful but possible. There is no error handling provided as this is a simple example program!</p>

<h1>Prerequisites</h1>
<li>Git</li>
<li>Cmake atleast version 3.18</li>
<li>Some type of c++ compiler system (clang, MSVC, GNU on windows aka MinGW-w64)</li>
<li>c++ 20</li>
<li>Not required. But makes life easier. an IDE (codeblocks, Visual Studio ect... which includes a c++ compiler system) 
<p></p>

<h1>How to:</h1>
<p>In your terminal...</p>

```powershell
# make a directory for cloning the project into
PS C:\Users\chris> mkdir win32_window

# navigate to the directory you just made
PS C:\Users\chris> cd win32_window

# attempt to clone the project from github
PS C:\Users\chris\win32_window> git clone "https://github.com/chriskish19/mt_window.git"

# navigate to the cmake directory where the CMake script is located
PS C:\Users\chris\win32_window> cd cmake

# make a directory to generate build files into
PS C:\Users\chris\win32_window\cmake> mkdir build

# invoke cmake to generate the build files
PS C:\Users\chris\win32_window\cmake> cmake -S . -B ./build
```

<h1>Notes</h1>
<p>If you are using CMake to generate a visual studio solution it will generate 3 projects named ALL_BUILD, mt_window, and ZERO_CHECK. Pressing the green button at the top will not properly launch the program it will only build it. You need to right click on mt_window project and select set as the startup project. Then press the green button again and it will launch the program. Once the program is launched the menu bar is where you can launch more windows by pressing New Window then clicking Create New Window.</p>
<p>If you are using CMake within an IDE as an Open Folder solution I have provided a CMake settings json file for build configs! Best way to build projects.</p>
