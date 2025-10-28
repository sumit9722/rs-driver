# RS Driver Documentation

## rs_driver.c – The Kernel Driver

This driver is a Linux kernel component that creates two separate virtual devices for handling encoding and decoding jobs.

### Device Files and Jobs

The driver works by using normal file commands (read and write) on two special files it creates in the /dev folder:

/dev/encoded (Minor 0): Used for Encoding. You write your raw data, and then you read back the full block (data + parity/protection).

/dev/decoded (Minor 1): Used for Decoding. You write the full protected block, and then you read back the corrected original data.

Key Kernel Functions

The driver uses unique read and write functions for the encode and decode devices, since it skips the standard generic open or release setup steps.

1. Configuration: rs_ioctl(struct file \*file, unsigned int cmd, unsigned long arg)

This is the only special command handler used to set up the driver's configuration.

Inputs (Parameters):

file: The device you opened (/dev/encoded or /dev/decoded).

cmd: The specific command code.

arg: The settings data (struct rs_params) coming from your application.

Supported Command:

RS_SET_PARAMS: This takes the settings from your app and initializes the kernel's Reed-Solomon core (rs_decoder). It uses a kernel lock (rs_lock) to make sure setup is safe.

What it returns:

0 if everything worked.

-EFAULT if it couldn't copy the settings correctly.

-ENOTTY if the command's unique ID is wrong.

2. Encoding Jobs

The encoding process uses two main functions:

rs_encode_write (Takes data to protect): Copies your data, runs the encode_rs8() function to create the parity bits, and saves the whole thing (data + parity) in the driver's buffer.

rs_encode_read (Sends back the protected block): Copies the entire data + parity buffer back to your application.

3. Decoding Jobs

The decoding process also uses two main functions:

rs_decode_write (Takes the potentially damaged protected block): Copies the block, runs the decode_rs8() function to fix errors, and prints how many errors it fixed.

rs_decode_read (Sends the fixed message back): Copies only the original message (without the parity bits) from the corrected buffer back to your application.

4. Module Start and Stop

These functions handle the driver lifecycle:

startup_func (Module Load - insmod): Runs when you load the module. It registers the device, sets up the internal structure (cdev_init), and creates the /dev/encoded and /dev/decoded files.

cleanup_func (Module Unload - rmmod): Runs when you unload the module. It removes the device files, cleans up the class, unregisters the devices, and frees up all the memory used by the RS core.

rs_app.c – The Test Application

This application shows you step-by-step how to set up the driver and run a full encode-decode test.

How the main() Program Works

The main function handles the entire process in sequential steps:

Set Up: Defines the standard Reed-Solomon settings (like nroots = 32).

Open Doors: Opens two file "doors": one (encode_fd) for the /dev/encoded device and one (decode_fd) for /dev/decoded.

Send Config: Uses the ioctl() command to send the settings to the kernel driver.

Encode Data: Uses write() on the encode door to send the original message.

Get Block: Uses read() on the encode door to grab the full protected block (message + parity).

Decode Data: Uses write() on the decode door to send the full block for error correction.

Get Message: Uses read() on the decode door to get the final, corrected message.

Check: Compares the final message to the original one to confirm success.

Key C Library Functions Used:

open(): Opens the device files.

ioctl(): Sends the configuration to the driver.

write(): Sends data in both the encode and decode steps.

read(): Gets the encoded block and the final corrected message.

close(): Shuts the device file doors.

Helper Function

The rs_app.c includes one helpful function:

print_buffer_hex: A handy tool that displays the raw data bytes in hexadecimal, which is useful for seeing the parity bits in the encoded block.

rs_driver_ioctl.h – Shared Settings

This header file holds the crucial settings and commands that both the kernel and your application must agree on.

IOCTL Macros

This is the single command used to configure the RS logic:

#define RS_IOCTL_MAGIC 'r'
#define RS_SET_PARAMS \_IOW(RS_IOCTL_MAGIC, 0, struct rs_params)

The \_IOW part just means this command is used to Write data In (settings) from your app to the kernel.
