# **RS Driver Documentation**

This document provides an overview of the Reed-Solomon (RS) Linux kernel driver and its associated user-space test application.

## **1\. rs\_driver.c – The Linux Kernel Driver**

This component is a Linux kernel module that provides Reed-Solomon encoding and decoding services by creating two dedicated virtual character devices.

### **Device Files and Job Flow**

The driver maps standard file operations (read, write, ioctl) to RS jobs through two device files located in the /dev directory:

* **/dev/encoded** (Minor 0): Used for **Encoding**. The job flow is: Write raw data $\\rightarrow$ Read protected block (data \+ parity).  
* **/dev/decoded** (Minor 1): Used for **Decoding**. The job flow is: Write protected block $\\rightarrow$ Read corrected original data.

### **Key Kernel Functions**

The driver relies on a custom set of file operation handlers.

#### **1.1. Configuration: rs\_ioctl**

This is the central control function used to configure the Reed-Solomon code parameters for the kernel-side core.

* **Function Signature:** rs\_ioctl(struct file \*file, unsigned int cmd, unsigned long arg)  
* **Purpose:** Receives RS code settings from the user application and initializes the kernel's Reed-Solomon control structure (rs\_control). A kernel mutex lock (rs\_lock) is used to ensure safe, thread-protected initialization.  
* **Supported Command:** **RS\_SET\_PARAMS**. This command takes a user-space structure (struct rs\_params) and uses copy\_from\_user() to transfer the settings into the kernel.  
* **Returns:**  
  * **0:** Success.  
  * **\-EFAULT:** Failed to copy settings from user space.  
  * **\-ENOTTY:** Command ID is incorrect.

#### **1.2. Encoding Functions**

Encoding jobs are handled by the file operations registered to the /dev/encoded device.

* **rs\_encode\_write (Input Data):**  
  1. Allocates an internal **driver buffer** large enough for the data *plus* parity symbols.  
  2. Copies the raw data from the user application into the driver buffer.  
  3. Calls the kernel function **encode\_rs8()** to generate parity symbols.  
  4. Stores the entire protected block (data \+ parity) in the internal buffer.  
* **rs\_encode\_read (Output Protected Block):**  
  1. Copies the full contents of the internal driver buffer (data \+ parity) back to the user application.

#### **1.3. Decoding Functions**

Decoding jobs are handled by the file operations registered to the /dev/decoded device.

* **rs\_decode\_write (Input Protected Block):**  
  1. Copies the potentially damaged protected block from the user application into the internal driver buffer.  
  2. Calls the kernel function **decode\_rs8()** to perform error detection and correction.  
  3. Logs the number of errors corrected.  
* **rs\_decode\_read (Output Corrected Message):**  
  1. Copies **only the original message data** (excluding the parity symbols) from the corrected internal buffer back to the user application.

#### **1.4. Module Lifecycle Functions**

These functions manage the driver's registration and cleanup in the kernel.

* **startup\_func (Module Load \- insmod):** Registers the character device, initializes the cdev structures for both minor devices, and uses device\_create to create the two /dev/encoded and /dev/decoded device files.  
* **cleanup\_func (Module Unload \- rmmod):** Reverses the startup process by destroying the device files and class, deleting the cdev structures, unregistering the device region, and freeing any memory used by the RS core and the global driver buffer.

## **2\. rs\_app.c – The Test Application**

This user-space C program demonstrates the full end-to-end usage of the kernel driver.

### **Main Program Flow**

The main() function executes the entire encode-decode cycle in sequence:

1. **Set Up:** Defines the Reed-Solomon settings (e.g., $\\text{nroots} \= 32$) in a local struct rs\_params.  
2. **Open Doors:** Uses **open()** to acquire file descriptors (encode\_fd, decode\_fd) for /dev/encoded and /dev/decoded.  
3. **Send Config:** Uses **ioctl()** to send the struct rs\_params settings to the kernel via the RS\_SET\_PARAMS command.  
4. **Encode Data:** Uses **write()** on encode\_fd to send the original message.  
5. **Get Block:** Uses **read()** on encode\_fd to retrieve the full encoded block (data \+ parity).  
6. **Decode Data:** Uses **write()** on decode\_fd to submit the full block for correction.  
7. **Get Message:** Uses **read()** on decode\_fd to retrieve the final, corrected original message.  
8. **Check:** Uses strcmp to compare the final message against the original data to verify success.

### **Helper Functions**

* **print\_buffer\_hex:** A utility function used to display the raw bytes of any buffer in **hexadecimal** format, particularly useful for inspecting the calculated parity bits in the encoded block.

## **3\. rs\_driver\_ioctl.h – Shared Settings**

This header file defines the constants and data structures required for communication between the user application and the kernel driver.

### **IOCTL Macros**

The file defines the unique command used for driver configuration:

* **\#define RS\_IOCTL\_MAGIC**: 'r'  
  * *Description:* The magic number identifying IOCTL commands for this driver.  
* **\#define RS\_SET\_PARAMS**: \\\_IOW(RS\_IOCTL\_MAGIC, 0, struct rs\_params)  
  * *Description:* The command to Write In the RS configuration structure from user space to the kernel.

### **struct rs\_params**

This structure defines the Reed-Solomon code parameters:  
```c 
struct rs_params {  
	int symsize;  // Symbol size in bits (e.g., 8\)  
	int gfpoly;   // Generator polynomial for the Galois field (e.g., 0x11d)  
	int fcr;      // First consecutive root of the generator polynomial  
	int prim;     // Primitive element  
	int nroots;   // Number of roots (equal to the number of parity symbols)  
}; 
```
