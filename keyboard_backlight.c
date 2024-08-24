
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <linux/input.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <time.h>

#define IDLE_TIME 15 // Time in seconds
#define KEYBOARD_NAME "AT Translated Set 2 keyboard" // Replace with your keyboard's name

int backlight_on = 0;
int backlight_fd;
int event_fd;
time_t last_event_time;

// Function to initialize the keyboard backlight file descriptor
int init_backlight_fd(const char *backlight_path) {
    int fd = open(backlight_path, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open backlight device");
        exit(EXIT_FAILURE);
    }
    return fd;
}

// Function to find the event device for the keyboard
int find_keyboard_event_device(const char *keyboard_name) {
    DIR *dir;
    struct dirent *entry;
    char dev_path[256];
    char device_name[256];
    struct libevdev *dev = NULL;
    int fd;

    if ((dir = opendir("/dev/input")) == NULL) {
        perror("Failed to open /dev/input directory");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            snprintf(dev_path, sizeof(dev_path), "/dev/input/%s", entry->d_name);
            fd = open(dev_path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                perror("Failed to open event device");
                continue;
            }

            if (libevdev_new_from_fd(fd, &dev) < 0) {
                close(fd);
                continue;
            }

            if (libevdev_get_name(dev) != NULL) {
                strncpy(device_name, libevdev_get_name(dev), sizeof(device_name) - 1);
                device_name[sizeof(device_name) - 1] = '\0';

                if (strcmp(device_name, keyboard_name) == 0) {
                    libevdev_free(dev);
                    closedir(dir);
                    return fd; // Found the correct device
                }
            }

            libevdev_free(dev);
            close(fd);
        }
    }

    closedir(dir);
    return -1; // Keyboard device not found
}

// Function to turn on the keyboard backlight
void turn_on_backlight() {
    if (backlight_on == 0) {
        if (write(backlight_fd, "1", 1) != 1) {
            perror("Failed to turn on backlight");
        }
        backlight_on = 1;
		printf("Kyeboard backlight 1");
    }
}

// Function to turn off the keyboard backlight
void turn_off_backlight() {
    if (backlight_on == 1) {
        if (write(backlight_fd, "0", 1) != 1) {
            perror("Failed to turn off backlight");
        }
        backlight_on = 0;
		printf("Kyeboard backlight 0");
    }
}

int main(int argc, char *argv[]) {
    struct libevdev *dev = NULL;
    int rc;
	printf("hhello world!");

    // Initialize the file descriptors
    const char *backlight_path = "/sys/class/leds/asus::kbd_backlight/brightness";
    const char *keyboard_name = KEYBOARD_NAME;

    backlight_fd = init_backlight_fd(backlight_path);
    event_fd = find_keyboard_event_device(keyboard_name);

    if (event_fd < 0) {
        fprintf(stderr, "Keyboard event device not found\n");
        return EXIT_FAILURE;
    }

    rc = libevdev_new_from_fd(event_fd, &dev);
    if (rc < 0) {
        fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
        return EXIT_FAILURE;
    }

    last_event_time = time(NULL);

    while (1) {
        struct input_event ev;
        rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);

        if (rc == 0) {
            if (ev.type == EV_KEY && ev.value == 1) { // Key pressed
                turn_on_backlight();
                last_event_time = time(NULL);
            }
        }

        // Check idle time
        if (backlight_on == 1) {
            time_t current_time = time(NULL);
            if (difftime(current_time, last_event_time) >= IDLE_TIME) {
                turn_off_backlight();
            }
        }

        usleep(100000); // Sleep for 100ms
    }

    // Clean up
    libevdev_free(dev);
    close(event_fd);
    close(backlight_fd);

    return 0;
}

