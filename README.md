# Azalea-unikernel

Azalea OS consists of a simple kernel for fast processing of applications and FWK for providing full capability of Linux. For simple kernels, we developed LWK and Unikernel, and this repository is a prototype of Unikernel, named Azalea-Unikernel, for the Intel KNL based manycore system. Azalea-LWK is in https://github.com/oslab-swrc/Azalea, and detailed information of Azalea OS is also described.

We have released this prototype under an open-source license to enable collaboration with parties outside. Most parts of Azalea OS are available to you under MIT while some parts are also available to you under the GNU General Public License (GPL) version 2. Please feel free to explore the source code and the documentation provided here

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

## What is Azalea-unikernel

Azalea-unikernel was developed by applying unikernel characteristics to Azalea-LWK. The Unikernel is only made up of the necessary libraries for execution, and the kernel and application use the same address space, so there is no additional cost for context switching. Azalea-unikernl improves processing performance, reduces the size of image by configuring image as library required for application to run, and boots fast by reflecting the advantages of Unikernel in multi-kernel OS.

### Architecture and Components

Azalea-unikernel consists of libOS which is responsible for kernel functions, run-time libraries, and application programs. A server can run multiple Azalea-unikernels with the number of cores and memory. Linux is install in the part of the server, and acts as a driver that loads each kernel or supports communication between the other nodes.

The testbed manycore systems consists of multiple Intel Xeon-phi for Azalea-unikernel, Xeon for FWK, and I/O devices such as storages and networks. The Xeon-phi consists of 7210p Kight Landing(KNL) processors and the Xeon has E5-2697v2 processors. Centos 7 is installed in FWK and driver.

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
