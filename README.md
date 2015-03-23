Simulator intended to give teachers a simpler way to teach buffer overflow vulnerabilities. Instead of having to learn the complexities of x86 instruction sets and stack implementations, students can learn a simple language with a sad BO vulnerability, and can develop working exploits in minutes.

The project was written in QT4, so you have to install that to work on the project. The code is not very maintainable, probably riddled with bugs (I seem to remember it barely working when I built it). I'd like to get it less buggy, more maintainable, more capable and less reliant on heavy libraries, which may require a different language and a web interface (that might be quite nice and might make it easier to use educationally). But for now I'm starting by getting the code available.

writeup.pdf is a basic writeup of the system intended for any user (especially a student) that may use the system. I believe I used latex to produce said pdf originally, and will have to find the source / translate it into markdown or something. It would eventually (in the event of making this a web application)

src contains the source code of the project, including the QT project file.

The system on which I originally built the bosim binary that is in the root of the project was a 64 bit intel processor running a 32 bit linux of some semi-ubuntu flavor, probably Linux Mint 10 at that time. bosim is the actual binary compiled in release form on my system. ./bosim should execute the demonstration simply and nicely (if you have the dependencies, which I have no idea how to install anymore). I'm not sure what exactly it takes to run the binary on a fresh ubuntu install or anything like that. So you will probably have to fire up QT if you want to do anything with this code. I hope to decrease that barrier to entry at some point.

