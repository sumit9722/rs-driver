#ifndef __RS_DRIVER_IOCTL_H
#define __RS_DRIVER_IOCTL_H

#include <linux/ioctl.h>

#define RS_IOCTL_MAGIC 'r'

#define RS_SET_PARAMS _IOW(RS_IOCTL_MAGIC, 0, struct rs_params)

struct rs_params {
	int symsize;  
	int gfpoly;   
	int fcr;      
	int prim;     
	int nroots;   
};

#endif