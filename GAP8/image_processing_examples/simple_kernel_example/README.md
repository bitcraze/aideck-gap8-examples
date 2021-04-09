# Image processing examples

This example takes an image, applies a kernel and writes the image over your JTAG cable (using openOCD file semi-hosting) to your computer.
You can choose between two kernels:

- demosaicking 
- inverting

for both you can choose to either execute the kernel sequentially on the fabric controller or parallelized on the cluster. Check out the defines in the Makefile to configure your kernel! But be careful, it only supports to choose one kernel at a time.
## Performance
To keep this example simple, we are just saving the time before and after the kernel runs - if you want more precise performance measurements check out  https://greenwaves-technologies.com/manuals/BUILD/PMSIS_API/html/group__Perf.html .
Here some pitfalls that we improved in the demosaicking kernel:
First computing a grayscale image from a RGB camera took around 2.33s (all measurements here are at a frequency of 50MHz on the fabric controller and cluster). That's awfully long, so what were we doing wrong?

- `output[idx] = 0.33*red + 0.33*green + 0.33*blue;` was not a good idea - we do not have a floating point unit (FPU) on GAP8, so float multiplications are really slow. 
- `output[idx] = red/3 + green/3 + blue/3;` This got us to 303.3ms - but we can still improve, right?
- `output[idx] = (red + green + blue)/3;` We only need one division! This gives us 199.2ms
- we also have 8 cores on the cluster! Paralellizing it brought us down to 33.3ms.

With those 3 steps we could already improve by a factor of 70. There are still other possible improvements, like moving the check `grayscale == 1` out of the for-loop for avoiding wrong branch predictions or loop-unrolling. 

One last hint: be careful with printf - this is a very long and complicated function and should never be inside your performance measurement.

This example was developed and tested with the gap-sdk 3.8.1.