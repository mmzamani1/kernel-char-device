#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>      // Required for file_operations structure
#include <linux/uaccess.h> // Required for copy_to_user and copy_from_user functions
#include <linux/jiffies.h>

MODULE_LICENSE("mmzamani");

#define DEVICE_NAME "myQueue" // Define the name of the device
#define BUFFER_SIZE 100         // Define the maximum size of the buffer


#define QUEUE_SIZE 10
#define TIMEOUT 10 * HZ // 10 seconds in jiffies

static char queue[QUEUE_SIZE];
static int front = 0;
static int rear = 0;
static int count = 0; // Number of elements in the queue


static char message[BUFFER_SIZE] = {0}; // Buffer to store the message
static int major_number; // Variable to store the major number for the device

static int blocking_state; // 1 for blocking and 0 for non_blocking

module_param(blocking_state, int, 0644);

// Wait queues for blocking behavior
static DECLARE_WAIT_QUEUE_HEAD(read_queue);
static DECLARE_WAIT_QUEUE_HEAD(write_queue);

// Function prototypes for the device operations
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);

// Struct to define the file operations for the character device
static struct file_operations fops = {
    .open = dev_open,       // Pointer to the open function
    .read = dev_read,       // Pointer to the read function
    .write = dev_write,     // Pointer to the write function
    .release = dev_release, // Pointer to the release function
};


// Module initialization function
static int __init myQueue_init(void)
{
    memset(queue, 0, sizeof(queue)); // Initialize the queue
    // Register the character device and get the major number
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0)
    {
        printk(KERN_INFO "myQueue module load failed\n");
        return major_number; // Return the error code
    }
    printk(KERN_INFO "The module has been loaded with major number = %d\n", major_number);
    printk(KERN_INFO "The module has been loaded with blockin state = %d\n", blocking_state);
    return 0;
}

// Module cleanup function
static void __exit myQueue_exit(void)
{
    unregister_chrdev(major_number, DEVICE_NAME); // Unregister the character device
    printk(KERN_INFO "myQueue module has been unloaded\n");
}

// Function called when the device is opened
static int dev_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "DEVICE_NAME = myQueue has been opened\n");
    return 0;
}

// Function called when the device is released (closed)
static int dev_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "DEVICE_NAME = myQueue has been closed\n");
    return 0;
}

// Function called to read from the device
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    char ch;
    int errors = 0;
    int message_len = count;
    
    if (*offset >= QUEUE_SIZE) { 
        return 0;
    } 
   
    if (count == 0){
        if (blocking_state == 1){// blocking
            printk(KERN_INFO "Blocking state, waiting for data...\n");

            if (wait_event_interruptible_timeout(read_queue, count > 0, TIMEOUT)) {
                ch = queue[front];
                front = (front + 1) % QUEUE_SIZE;
                count--;
                errors = copy_to_user(buffer, queue, QUEUE_SIZE);
                *offset += message_len;
                
            }
            else {
                printk(KERN_INFO "Queue is empty, read operation timed out\n");
                return errors == 0 ? message_len : -EFAULT;
            }
        }
        else if (blocking_state == 0){// non_blocking
            printk(KERN_INFO "Queue is empty, read operation failed\n");
            return errors == 0 ? message_len : -EFAULT;
        }
    }
    else {
        // Read character from the queue
        ch = queue[front];
        front = (front + 1) % QUEUE_SIZE;
        count--;
        errors = copy_to_user(buffer, queue, QUEUE_SIZE);
        *offset += message_len;
    }
    
    return errors == 0 ? message_len : -EFAULT;
}

// Function called to write to the device
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)
{
    char ch;
    int errors = 0;

    if (*offset >= QUEUE_SIZE) { 
        return 0;
    } 

    errors = copy_from_user(message, buffer, len);
    printk(KERN_INFO "message: %s\n", message);

    if (count == QUEUE_SIZE){
        if (blocking_state == 1){// blocking
            printk(KERN_INFO "Blocking state, waiting for space...\n");

            if (wait_event_interruptible_timeout(read_queue, count < QUEUE_SIZE, TIMEOUT)) {
                errors = copy_from_user(&ch, buffer, len);
                queue[rear] = ch;
                rear = (rear + 1) % QUEUE_SIZE;
                count++;
                *offset += len; 
            }
            else {
                printk(KERN_INFO "Queue is full, write operation timed out\n");
                return errors == 0 ? len : -EFAULT;
            }
        }
        else if (blocking_state == 0){// non_blocking
            printk(KERN_INFO "Queue is full, write operation failed\n");
            return errors == 0 ? len : -EFAULT;
        }
    }
    else{
        // Write character to the queue
        errors = copy_from_user(&ch, buffer, len);
        queue[rear] = ch;
        rear = (rear + 1) % QUEUE_SIZE;
        count++;
        *offset += len; 
    }

    return errors == 0 ? len : -EFAULT;
}


// Macros to register the module initialization and exit functions.
module_init(myQueue_init);
module_exit(myQueue_exit);