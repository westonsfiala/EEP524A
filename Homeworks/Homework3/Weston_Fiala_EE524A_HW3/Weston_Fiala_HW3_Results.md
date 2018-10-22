Discuss differences observed between the 4 cases above. What do you observe?
▪ Try this on both GPU and CPU OpenCL devices
▪ Do the OpenCL kernel timing results agree with Code Builder KDF Best
Configuration execution median values?

After running the naive and opt1 kernels on both my GPU and my CPU, I found that the time it took to run each kernel was roughly equivalent between devices, but the standard deviation for the CPU runs was much lower. The Naive implementation ran slower across every run compared to the opt1 kernel.

One thing that I did notice is that the measurements of run duration were much higher when using the Windows performance counters. One thing that surprised me is just how big of a difference was seen between measuring from when NDKernel finishes and when clFinish is done using the performance counters. This big discrepincy really drives home the point that there is other stuff going on under the hood when we do these calls. I believe that these performance counters are useful in that they tell you how long the code actually took from the host side, but the openCL measurements give you a better result when trying to analyze a kernel. The overhead from getting all the information about completion to the CPU masks performance of the actual kernel. 

The results from the Code Builder sessions are about spot on. The CPU ones ran a bit slower that I expected, but it was roughly the same. I could tweek some of the local range sizes to improve it some more I believe.