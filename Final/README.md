Weston Fiala
EEP 524A - Final Project

* To run the program open the FinalProject.sln solution file
* I have set up special project properties that need to be used for the program to work. Please do not try to use the files without the project.
    * I use a library for generating the graphics, and its dll needs to be next to the executable, so after every build I copy it there.

* For Serial implementation, set SerialMandelbrot as the startup project
* For OpenCl implementation, set FinalProject as the startup project
    * To run openCL without issues, open host.cpp and change line 13 to the location that you unzipped this to.
    * BASE_DIRECTORY should point to the directory that this README is in.
    * If you don't change this directory, failures will happen.

* The for either project, running the program will produce a fractal that increases in order from 1.0 to 11.0.
    * this order can be changed at the top of host.cpp, or serialMandelbrot.cpp.
    * Be warned, the one for serialMandelbrot takes a very long time.

* The image produced will fill up the main display screen.

* The OpenCL version, as is, will run a local worksize optimization and then run the full design. 

* If you do not want the image to be displayed, set the value of true in kernelGen.runMandelbrot() to false.
* If you want to run the design to a different order, change ORDER.
* If you want to change the stepsize of the image. Change stepSize. Lower = slower, higher = faster.
* If you want to print out the timing results from the kernel run, uncomment the line that has Helper::printResults.

* If you are running on a machine that does not support OpenCL 2.0, change lines 7 & 8 in helperFunctions.h to be 120 instead of 200.

* If you have any problems please get in contact with me in any way. I hope this works flawlessly. I ran it on multiple different machines to test it, but your machine could be different.