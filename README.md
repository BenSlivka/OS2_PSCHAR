# OS2_PSCHAR
A character-mode full-screen/visual Process Status tool for OS/2 v1.1 ca. 1988.

The initial version was very sluggish.
I had written a CPU profiling tool earlier (ca. 1987), which worked by increasing the real-time clock interrupt rate to 1KHz from 60Hz, and storing the current CS:IP in a fixed memory buffer.
Using this tool, I determined that the C run-time routine sprintf() -- which takes a formatting string and arguments that are integers, strings, etc., and generates a new string -- was a big performance bottleneck.

So I wrote FMT.ASM -- a lean-and-mean subset of sprintf()! Didn't do floating point, and a few other obscure data types/formats.
Used all the assembly language tricks I had learned in the 1970s-1980s on the Control Data Corporation 6400 at Northwestern University's Vogelback Computing Center. The COMPASS assembly language was very sophisticated, with not only MACROs but also MICROs (an in-line text replacement mechanism).

From the vantage point of 2/9/2021 -- nearly 30 years later -- this code seems hopelessly complex and yet trivial.
As we write massive web apps that run in gigabytes (even terabytes) of RAM.
While OS/2 1.1 would run well in 4Mb of RAM.
--bens@microsoft.com (1985-1999)
