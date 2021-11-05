# Azalea OS

Azalea OS is an operating system (OS) research project. This repository contains the prototype of Azalea OS for the Intel  X86 based manycore system. Azalea OS consists of unikernel for a simple kernel for fast processing of applications and Full-Kernel for providing full capability of Linux. 

We have released this prototype under an open-source license to enable collaboration with parties outside. It is composed of two packages: Azalea, Newlib
(LICENSE) Most parts of Azalea's codes are under GPL 3.0-or-later license, and some codes follow the BSD 3-clause, GPL-3.0-only, MIT.
(LICENSE.newlib) Each file in Newlib may have its own copyright/license that is embedded in the code. Unless otherwise noted in the code, the specified license will apply to the content of the newlib directory (include/newlib/)

## Overview

The documentation in this repository is structured as follows:

* What is Azalea-unikernel
  * Architecture and Components
* Getting Started
  * Install Operating System
  * Modify Linux Environments
  * Cloning the Source Repositories
  * Building
* Booting and Running Azalea-unikernel 
* Developer Information

## What is Azalea

In this section, we first discuss the idea behind Azalea OS and then describe what you can do with our prototype today.

As manycore systems have evolved recently, it has been required to provide scalability and stable performance of applications in manycore environments. General-purpose operating systems provide a rich set of functionality to support a broad set of application, whereas it does not show scalable performance in manycore systems because of OS noise. Therefore, manycore systems provide only the features that are absolutely required, in a way to provide the targeted applications the scalable performance in proportion to the number of cores.

Azalea OS is our approach to bridge the gap between these two extremes. We combine unikernels with a full kernel and run them side by side using space partitioning. Unikernel provides scalability and parallel performance in manycoreos by minimizing kernel functionality in manycore systems. Full-kernel handles compatibility with existing Linux APIs (I/O, systemcalls, etc.) that a unikernel cannot handle.

Since the core and memory resources are sufficient in manycore systems, time sharing, which is used to share resources, is no longer appropriate. Therefore, space sharing is defined to allocate resources to be used by applications. The application use the allocated resources without any kernel noise. By using space sharing techniques, kernel interference and waste of resource is minimized. Space sharing consists of two techniques as followed.
*One core-One thread - A thread is assigned to the core to ensure that it is executed without interrupt until terminated.
*One core-One memory - Specifies the memory area available for each core. When a thread that is running on a specific CPU requests memory, application use only the memory in that specified area. This can eliminate memory contention between cores.

We are aware that this document may not convey all of Azalea OS's design in sufficient depth and may fall short of providing a guide to understanding the prototype. We welcome any feedback about the quality of this description, so that we can improve it. Your can use the github mechanisms to post feedback.

Azalea-unikernel was developed by applying unikernel characteristics to Azalea-LWK. The Unikernel is only made up of the necessary libraries for execution, and the kernel and application use the same address space, so there is no additional cost for context switching. Azalea-unikernl improves processing performance, reduces the size of image by configuring image as library required for application to run, and boots fast by reflecting the advantages of Unikernel in multi-kernel OS.

### Architecture and Components

Azalea-unikernel consists of libOS which is responsible for kernel functions, run-time libraries, and application programs. A server can run multiple Azalea-unikernels with the number of cores and memory. Linux is install in the part of the server, and acts as a driver that loads each kernel or supports communication between the other nodes.

The testbed manycore systems consists of multiple Intel Xeon-phi for Azalea-unikernel, Xeon for FWK, and I/O devices such as storages and networks. The Xeon-phi consists of 7210p Kight Landing(KNL) processors and the Xeon has E5-2697v2 processors. Centos 7 is installed in FWK and driver.

### I/O Offloading

In the Azalea OS architecture, the filesystem would exist in the FWK of Xeon. When the I/O function such as creat, open, read, write, and close is required at the application in LWK, it is delivered to the FWK and its result is returned to the LWK.

## Getting Started

### Install Operating System

We recommend that you install CentOS for Linux as a driver.

### Modify Linux Environments

It sets up the CPU and memory areas to be used by Linux as a driver for Azaleal-unikernel. By default, Linux uses 5 cores and 1G of memory, Azalea-unikernel uses the rest of the resources. The setting method is as follows.

* Edit /etc/default/grub by using gedit or vi
* Find the line starting with GRUB_CMDLINE_LINUX_DEFAULT="maxcpus=2 mem=999M apic=debug memmap=24K$0x96000"
* Save the file and close the editor.
* Finally, start a terminal and run: sudo update-grub (grub2-mkconfig -o /boot/grub2/grub.cfg) to update GRUB's configuration file (you probably need to enter your password).
* On the next reboot, the kernel should be started with the boot parameter. To permanently remove it, simply remove the parameter from GRUB_CMDLINE_LINUX_DEFAULT and run sudo update-grub again. To verify your changes, you can see exactly what parameters your kernel booted with by executing cat /proc/cmdline.

### Cloning the Source Repositories

Clone the main repository of Azalea-unikernel
<pre>
  $ git clone https://github.com/oslab-swrc/Azalea-unikernel.git
</pre>

To run the legacy application, the libc header files of newlib should be stored in the ```include/api``` directory

<pre>
  $ wget ftp://sourceware.org/pub/newlib/newlib-3.0.0.tar.gz
  $ tar xvfz newlib-3.0.0.tar.gz
  $ cp -R newlib-3.0.0/newlib/libc/include/* ./Azalea/include/api/
  $ cd Azalea/include/api
  $ patch -p1 < azalea-newlib.patch
</pre>

### Building

Now, that you have prepared your build environment, you can build Azalea-unikernel. Execute makefile in the root directory. 
It will make a single bootable image including kernel, library, and application.

<pre>
  $ make
</pre>

## Booting and Running Azalea-unikernel 

The generated image can be loaded into the memory and executed via the driver. The code related to loading and booting is in the sideloader folder and also can be easily executed through a script in devel-script folder.

<pre>
  $ cd devel-script
  $ ./start.sh -i <# of index> -c <# of core> -m <# of memory>
</pre>

Index is used to make core and memory configuration easier, and core and memory settings are not applied when index value is set. Basically, one index has 5 cores and 1GB of memory. Indexes start at 1.

## Limitations

TBD

## Developer Information

There is no actual developer guide for Azalea-unikernel yet. We struggle with that task ourselves!
If you want to get a grasp of the Azalea-unikernel source, reread the section on the Architecture and Components of Azalea-unikernel and follow our pointers to the source code. 
Please feel free to contact us if you have questions or recommendations.
