# RS Driver Documentation

## rs_driver.c – Device Driver

### 1. `rs_open(struct inode *inode, struct file *file)`

- Called when the user-space app opens `/dev/rs_driver`.
- Sets up the device for communication.
- **Parameters:**

  - `inode`: Kernel structure representing the file on disk.
  - `file`: Structure representing the opened file instance.

- **Returns:** 0 on success.

### 2. `rs_release(struct inode *inode, struct file *file)`

- Called when the device file is closed.
- Cleans up any temporary state.
- **Returns:** 0.

### 3. `rs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)`

- Handles IOCTL commands for communication between user and kernel space.
- **Parameters:**

  - `file`: Pointer to the opened device.
  - `cmd`: IOCTL command code.
  - `arg`: Pointer to user-space data.

- **Commands:**

  - `RS_IOCTL_SET_DATA`: Copies data from user space to kernel.
  - `RS_IOCTL_GET_DATA`: Copies data from kernel to user.
  - `RS_IOCTL_RESET`: Resets the stored kernel data.

- **Returns:**

  - 0 on success.
  - `-EFAULT` if data copy fails.
  - `-EINVAL` if invalid command.

### 4. `rs_driver_init(void)`

- Called when the module is loaded using `insmod`.
- Registers the device with the kernel and allocates a device number.
- **Returns:** 0 on success or a negative error code.

### 5. `rs_driver_exit(void)`

- Called when the module is unloaded using `rmmod`.
- Unregisters the device and releases allocated resources.

---

## rs_app.c – User App

### 1. `main()`

- Entry point of the user-space program.
- Demonstrates how to communicate with the kernel driver using IOCTL calls.
- Steps:

  1. Opens `/dev/rs_driver` using `open()`.
  2. Sends data using `ioctl(fd, RS_IOCTL_SET_DATA, &data)`.
  3. Reads data back using `ioctl(fd, RS_IOCTL_GET_DATA, &data)`.
  4. Optionally resets data using `ioctl(fd, RS_IOCTL_RESET, NULL)`.
  5. Closes the device using `close(fd)`.

- **Returns:** 0 on success.

**Key Functions Used:**

- `open()`: Opens the driver device file.
- `ioctl()`: Sends commands and data to the driver.
- `close()`: Closes the driver file.

---

## rs_driver_ioctl.h – IOCTL Definitions

### 1. IOCTL Macros

Defines commands that the user-space program sends to the kernel module.

- `RS_IOCTL_MAGIC`: Unique identifier for this driver’s IOCTLs.
- `RS_IOCTL_SET_DATA`: Write operation from user to kernel.
- `RS_IOCTL_GET_DATA`: Read operation from kernel to user.
- `RS_IOCTL_RESET`: Command to reset driver’s internal data.

### 2. Data Structure

If a data structure is defined (like `struct rs_data`), it holds the values passed between user and kernel.

**Example:**

```c
#define RS_IOCTL_MAGIC 'r'
#define RS_IOCTL_SET_DATA _IOW(RS_IOCTL_MAGIC, 1, int)
#define RS_IOCTL_GET_DATA _IOR(RS_IOCTL_MAGIC, 2, int)
#define RS_IOCTL_RESET    _IO(RS_IOCTL_MAGIC, 3)
```

---

**Overall Flow:**

1. `rs_driver.c` implements the kernel module.
2. `rs_driver_ioctl.h` defines shared IOCTL commands.
3. `rs_app.c` is the user-space interface to send and receive data using those commands.
