#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DISK_SIZE 128
#define BLOCK_SIZE 1024
#define MAX_FILES 16
#define MAX_FILENAME 9

typedef struct
{
    char fileName[MAX_FILENAME];
    int fileSize;
} Inode;

typedef struct
{
    int freeBlocks[DISK_SIZE];
    Inode inodes[MAX_FILES];
} FileSystem;

// Function Definition

void initializeDisk(FileSystem *);
void copyFile(FileSystem *, const char *, const char *);
void createDirectory(FileSystem *, const char *);
void moveFile(FileSystem *, const char *, const char *);
void createFile(FileSystem *, const char *, int);
void removeDirectory(FileSystem *, const char *);
void deleteFile(FileSystem *, const char *);
void listAllFiles(FileSystem *);

void copyFile(FileSystem *fsys, const char *source, const char *desination)
{
    // Check if the source file exists
    int sourceInodeIndex = -1;
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strcmp(fsys->inodes[i].fileName, source) == 0)
        {
            sourceInodeIndex = i;
            break;
        }
    }

    if (sourceInodeIndex == -1)
    {
        printf("The source file '%s' does not exist\n", source);
        return;
    }

    int sourceFileSize = fsys->inodes[sourceInodeIndex].fileSize;

    // Calculate the number of blocks needed
    int BLOCKSNeeded = (sourceFileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Check available space
    int BLOCKSAvailable = 0;
    for (int i = 0; i < DISK_SIZE; i++)
    {
        if (fsys->freeBlocks[i] == 1)
        {
            BLOCKSAvailable++;
        }
    }

    if (BLOCKSAvailable < BLOCKSNeeded)
    {
        printf("Not enough space to copy '%s' to '%s'\n", source, desination);
        return;
    }

    // Find an empty slot for the destination file
    int InodeDestination = -1;
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strlen(fsys->inodes[i].fileName) == 0)
        {
            InodeDestination = i;
            break;
        }
    }

    if (InodeDestination == -1)
    {
        printf("Max file limit reached\n");
        return;
    }

    // Copy the file information to the destination
    strcpy(fsys->inodes[InodeDestination].fileName, desination);
    fsys->inodes[InodeDestination].fileSize = sourceFileSize;

    // Copy the blocks from source to destination
    int copied_blocks = 0;
    for (int i = 0; i < DISK_SIZE && copied_blocks < BLOCKSNeeded; i++)
    {
        if (fsys->freeBlocks[i] == 1)
        {
            fsys->freeBlocks[i] = 0;
            copied_blocks++;
        }
    }

    printf("File '%s' copied to '%s'\n", source, desination);
}

void createDirectory(FileSystem *fsys, const char *directory)
{

    // check if the directory already exists

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strcmp(fsys->inodes[i].fileName, directory) == 0)
        {
            printf("The directory '%s' already exists\n", directory);
            return;
        }
    }

    // find the last slash to extract the parent path
    const char *slash = strrchr(directory, '/');
    if (slash == NULL)
    {
        printf("Invalid directory path '%s'\n", directory);
        return;
    }

    // extract the parent path
    char MainPath[MAX_FILENAME];
    strncpy(MainPath, directory, slash - directory);
    MainPath[slash - directory] = '\0';

    // check if the parent directory exists
    int InodeIndex = -1;
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strcmp(fsys->inodes[i].fileName, MainPath) == 0)
        {
            InodeIndex = i;
            break;
        }
    }

    if (InodeIndex == -1)
    {
        printf("Parent directory '%s' don't exist.\n", MainPath);
        return;
    }

    // Find an empty slot for the new directory
    int NewInodeIndex = -1;
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strlen(fsys->inodes[i].fileName) == 0)
        {
            NewInodeIndex = i;
            break;
        }
    }

    if (NewInodeIndex == -1)
    {
        printf("Error directory limit\n");
        return;
    }

    // Create the directory
    strcpy(fsys->inodes[NewInodeIndex].fileName, directory);
    fsys->inodes[NewInodeIndex].fileSize = 0;

    printf("New directory '%s' created.\n", directory);
}

void moveFile(FileSystem *fsys, const char *source, const char *desination)
{
    int sourceInodeIndex = -1;
    int InodeDestination = -1;

    // finding the source and destinaiton file
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strcmp(fsys->inodes[i].fileName, source) == 0)
        {
            sourceInodeIndex = i;
        }
        if (strcmp(fsys->inodes[i].fileName, desination) == 0)
        {
            InodeDestination = i;
        }
    }

    if (sourceInodeIndex == -1)
    {
        printf("Source file '%s' don't exist.\n", source);
        return;
    }

    if (InodeDestination != -1)
    {
        printf("Destination file '%s' already exists.\n", desination);

        // free blocks
        int dst_file_size = fsys->inodes[InodeDestination].fileSize;
        int dst_blocks_to_free = dst_file_size / BLOCK_SIZE + (dst_file_size % BLOCK_SIZE != 0);
        for (int i = 0; i < DISK_SIZE && dst_blocks_to_free > 0; i++)
        {
            if (fsys->freeBlocks[i] == 0)
            {
                fsys->freeBlocks[i] = 1;
                dst_blocks_to_free--;
            }
        }

        // clear destination file innode
        memset(&fsys->inodes[InodeDestination], 0, sizeof(Inode));
    }

    // rename the source file to the destination name
    strcpy(fsys->inodes[sourceInodeIndex].fileName, desination);

    // free blocks of the source file
    int sourceFileSize = fsys->inodes[sourceInodeIndex].fileSize;
    int FreeSourceBlock = sourceFileSize / BLOCK_SIZE + (sourceFileSize % BLOCK_SIZE != 0);
    for (int i = 0; i < DISK_SIZE && FreeSourceBlock > 0; i++)
    {
        if (fsys->freeBlocks[i] == 0)
        {
            fsys->freeBlocks[i] = 1;
            FreeSourceBlock--;
        }
    }

    printf("File '%s' moved to '%s'\n", source, desination);
}

void createFile(FileSystem *fsys, const char *filename, int size)
{
    // calculating the number of blocks needed for the file
    int BLOCKSNeeded = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // checking space
    int BLOCKSAvailable = 0;
    for (int i = 0; i < DISK_SIZE; i++)
    {
        if (fsys->freeBlocks[i] == 1)
        {
            BLOCKSAvailable++;
        }
    }

    if (BLOCKSAvailable < BLOCKSNeeded)
    {
        printf("Not enough space\n");
        return;
    }

    // finding an empty slot for the file
    int InodeIndex = -1;
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strlen(fsys->inodes[i].fileName) == 0)
        {
            InodeIndex = i;
            break;
        }
    }

    if (InodeIndex == -1)
    {
        printf("Max file limit reached\n");
        return;
    }

    // CREATE the File
    strcpy(fsys->inodes[InodeIndex].fileName, filename);
    fsys->inodes[InodeIndex].fileSize = size;

    // block allocaion for the file
    int blocks_allocated = 0;

    for (int i = 0; i < DISK_SIZE && blocks_allocated < BLOCKSNeeded; i++)
    {
        if (fsys->freeBlocks[i] == 1)
        {
            fsys->freeBlocks[i] = 0;
            blocks_allocated++;
        }
    }

    printf("File '%s' created with size %d bytes\n", filename, size);
}

void removeDirectory(FileSystem *fsys, const char *directory)
{
    // finding the innode
    int DirectoryIndex = -1;
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strcmp(fsys->inodes[i].fileName, directory) == 0)
        {
            DirectoryIndex = i;
            break;
        }
    }

    //  check directory and print error
    if (DirectoryIndex == -1)
    {
        printf("Directory '%s' don't exist.\n", directory);
        return;
    }

    // If the directory is not empty, print an error message and return.
    if (fsys->inodes[DirectoryIndex].fileSize > 0)
    {
        printf("'%s' is non empty.\n", directory);
        return;
    }

    // remove the directory inode.
    memset(&fsys->inodes[DirectoryIndex], 0, sizeof(Inode));

    // recusrion
    for (int i = 0; i < MAX_FILES; i++)
    {
        const Inode *inode = &fsys->inodes[i];

        if (strlen(inode->fileName) > 0 && strstr(inode->fileName, directory) == inode->fileName)
        {
            removeDirectory(fsys, inode->fileName);
        }
    }
}

void deleteFile(FileSystem *fsys, const char *filename)
{
    int InodeIndex = -1;

    // Finding the files whiche needs to be deleted .
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (strcmp(fsys->inodes[i].fileName, filename) == 0)
        {
            InodeIndex = i;
            break;
        }
    }

    // chekcing if file exists
    if (InodeIndex == -1)
    {
        printf("File '%s' don't exisy\n", filename);
        return;
    }

    // clear the inode and free values
    int file_size = fsys->inodes[InodeIndex].fileSize;
    int blocks_to_free = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for (int i = 0; i < DISK_SIZE && blocks_to_free > 0; i++)
    {
        if (fsys->freeBlocks[i] == 0)
        {
            fsys->freeBlocks[i] = 1;
            blocks_to_free--;
        }
    }

    memset(&fsys->inodes[InodeIndex], 0, sizeof(Inode)); // clears the innode by flling the block of memeory with darknessss

    printf("File '%s' deleted successfully\n", filename);
}

void listAllFiles(FileSystem *fsys)
{
    printf("\nHere's the list of all files and directories:\n");

    // Loop through all the inodes
    for (int i = 0; i < MAX_FILES; i++)
    {
        const Inode *inode = &fsys->inodes[i];

        // Check if it's a directory
        if (inode->fileSize == 0 && strlen(inode->fileName) > 0)
        {
            printf("Directory: %s\n", inode->fileName);
        }
        // Check if it's a file
        else if (inode->fileSize > 0 && inode->fileSize < BLOCK_SIZE)
        {
            // Check if the file name contains only characters from 'A' to 'z' and includes '.'
            int validFileName = 1;
            for (int j = 0; j < strlen(inode->fileName); j++)
            {
                if (!(inode->fileName[j] >= 'A' && inode->fileName[j] <= 'z') && inode->fileName[j] != '.')
                {
                    validFileName = 0;
                    break;
                }
            }

            // Print the file name if it's valid
            if (validFileName)
            {
                printf("%s %d bytes.\n", inode->fileName, inode->fileSize);
            }
        }
    }

    printf("\n");
}


void initializeDisk(FileSystem *myfs)
{
    // initialize the disk with the const integer of disk size
    for (int i = 0; i < DISK_SIZE; i++)
    {
        myfs->freeBlocks[i] = 1;
    }

    strcpy(myfs->inodes[0].fileName, "/");
    myfs->inodes[0].fileSize = 0;
}

int main(int argc, char *argv[])
{
    FileSystem myfs;

    // checking if the disk is available
    FILE *file = fopen("myfs", "rb");
    if (file != NULL)
    {
        fread(&myfs, sizeof(FileSystem), 1, file);
        fclose(file);
    }
    else
    {
        // initialize the disk
        for (int i = 0; i < DISK_SIZE; i++)
        {
            myfs.freeBlocks[i] = 1;
        }

        strcpy(myfs.inodes[0].fileName, "/");
        myfs.inodes[0].fileSize = 0;
    }

    if (argc != 2)
    {
        printf("Command File : %s\n", argv[0]);
        return 1;
    }

    // execution instuctions from a file
    FILE *commandFile = fopen(argv[1], "r");
    if (commandFile == NULL)
    {
        printf("Error in the file.\n");
        return 1;
    }

    char command[256];

    while (fgets(command, sizeof(command), commandFile) != NULL)
    {
        command[strcspn(command, "\n")] = '\0';

        // execute instruction
        // Split the command into its components.

        char cmd[256], arg1[256], arg2[256];
        int num_args = sscanf(command, "%s %s %s", cmd, arg1, arg2);

        // Switch on the command name.
        switch (cmd[0])
        {
        case 'C':
            if (strcmp(cmd, "CR") == 0)
            {
                createFile(&myfs, arg1, atoi(arg2));
            }
            else if (strcmp(cmd, "CP") == 0)
            {
                copyFile(&myfs, arg1, arg2);
            }
            else if (strcmp(cmd, "CD") == 0)
            {
                createDirectory(&myfs, arg1);
            }
            else
            {
                printf("Unknown command: %s\n", cmd);
            }
            break;
        case 'D':
            if (strcmp(cmd, "DL") == 0)
            {
                deleteFile(&myfs, arg1);
            }
            else if (strcmp(cmd, "DD") == 0)
            {
                removeDirectory(&myfs, arg1);
            }
            else
            {
                printf("Unknown command: %s\n", cmd);
            }
            break;
        case 'L':
            if (strcmp(cmd, "LL") == 0)
            {
                listAllFiles(&myfs);
            }
            else
            {
                printf("Unknown command: %s\n", cmd);
            }
            break;
        case 'M':
            if (strcmp(cmd, "MV") == 0)
            {
                moveFile(&myfs, arg1, arg2);
            }
            else
            {
                printf("Unknown command: %s\n", cmd);
            }
            break;
        default:
            printf("Unknown command: %s\n", cmd);
        }
    }

    // save the state of disk
    FILE *output = fopen("myfs", "wb");
    if (output != NULL)
    {
        fwrite(&myfs, sizeof(FileSystem), 1, output);
        fclose(output);
    }
    else
    {
        printf("Failed To save.\n");
    }

    // Close fiel
    fclose(commandFile);

    return 0;
}