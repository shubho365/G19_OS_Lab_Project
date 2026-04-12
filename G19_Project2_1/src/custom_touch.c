#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <utime.h>

int main(int number_of_arguments, char *arguments[])
{
    // Checking the number of arguments
    if (number_of_arguments != 2)
    {
        perror("Command should in the form 'touch <file_name>'");
        return 1;
    }

    // creating a file if it does not exist
    char *file_name = arguments[1];
    int file_descriptor = open(file_name, O_CREAT | O_WRONLY, 0644);

    if (file_descriptor < 0)
    {
        perror("Error while creating the file");
        return 1;
    }

    close(file_descriptor);

    // updating timestamps
    if (utime(file_name, NULL))
    {
        perror("Error while updating timestamps");
        return 1;
    }

    return 0;
}
