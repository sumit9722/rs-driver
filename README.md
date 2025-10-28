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

### 5. `decode_read(char __user *buf, size_t count)`

- Reads raw data from the device and decodes it before returning to user space.
- **Parameters:**

  - `buf` — user-space buffer pointer to copy decoded bytes into.
  - `count` — number of bytes requested.

- **What it does:** reads internal/raw buffer, applies decoding (e.g., unescape, decrypt, or parse), copies up to `count` bytes to `buf` using `copy_to_user()`.
- **Returns:** number of bytes copied on success, `-EFAULT` on copy error, or negative error code.

### 6. `decode_write(const char __user *buf, size_t count)`

- Accepts encoded data from user space, decodes it, and stores it in the device buffer.
- **Parameters:**

  - `buf` — user-space buffer pointer containing encoded data.
  - `count` — number of bytes provided.

- **What it does:** copies from user with `copy_from_user()`, decodes (e.g., unescape, decrypt), validates, and writes to internal storage.
- **Returns:** number of bytes consumed on success, `-EFAULT` on copy error, or negative error code.

### 7. `encode_read(char __user *buf, size_t count)`

- Reads raw device data, encodes it (e.g., escape, encrypt, or format), and sends to user space.
- **Parameters:**

  - `buf` — user-space buffer for encoded output.
  - `count` — maximum bytes to write.

- **What it does:** reads internal data, applies encoding, then `copy_to_user()` up to `count` bytes.
- **Returns:** number of bytes copied, `-EFAULT` on error, or negative error code.

### 8. `encode_write(const char __user *buf, size_t count)`

- Receives plain data from user space, encodes it, and stores/sends to hardware.
- **Parameters:**

  - `buf` — user-space pointer containing plain data.
  - `count` — number of bytes.

- **What it does:** `copy_from_user()` then encodes (e.g., escape, add checksum, encrypt) before writing to device buffer or hardware.
- **Returns:** number of bytes processed, `-EFAULT` on error, or negative error code.

### 9. `rs_driver_init(void)`

- Called when the module is loaded using `insmod`.
- Registers the device with the kernel and allocates a device number.
- **Returns:** 0 on success or a negative error code.

### 10. `rs_driver_exit(void)`

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
