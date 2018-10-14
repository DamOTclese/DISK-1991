
/* **********************************************************************
   * A floppy disk finder. Much better than any one elses.              *
   *                                                                    *
   * What was needed was a very simple disk program that would keep     *
   * track of the files and disks someone has. No menus, screens,       *
   * colors, configuration, fancy output, or anything else. If there is *
   * a command-line entry, a list of the disks that have the file(s)    *
   * is printed and then the program is exited.                         *
   *                                                                    *
   * It will assign a label if there is none to a disk without one. The *
   * next empty label number is selected and you may not want this do   *
   * you are asked if you want to continue.                             *
   *                                                                    *
   * The label program that comes with DOS is shelled out to to create  *
   * the label.                                                         *
   *                                                                    *
   ********************************************************************** */

/* **********************************************************************
   * How about any macros.                                              *
   *                                                                    *
   ********************************************************************** */

#define skipspace(s)    while(isspace(*s))  ++(s)

/* **********************************************************************
   * Include some header files.                                         *
   *                                                                    *
   ********************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <bios.h>
#include <conio.h>
#include <ctype.h>
#include <dos.h>
#include <dir.h>
#include <string.h>
#include <io.h>
#include <time.h>

/* **********************************************************************
   * Define various constants                                           *
   *                                                                    *
   ********************************************************************** */

#define Total_Disks     2000
#define TRUE            1
#define FALSE           0
#define BOOL            unsigned char

/* **********************************************************************
   * disk_defined is an array of bytes which indicate whether or not a  *
   * disk has been defined. Its entries are 0 if not, else 1.           *
   *                                                                    *
   ********************************************************************** */

    static char disk_defined[Total_Disks];

/* **********************************************************************
   * The remaining entries in the data file look like this.             *
   *                                                                    *
   ********************************************************************** */

    static struct Disk_Entry {
        char valid;                     /* 1 if entry is used, else 0   */
        char file_name[13];             /* File name                    */
        short label;                    /* Label converted to number    */
    } disk;                             /* Define a single item         */

/* **********************************************************************
   * Define the data needed for this module.                            *
   *                                                                    *
   ********************************************************************** */

    static FILE *disk_file;
    static long table_start;
    static unsigned char current_drive;

/* **********************************************************************
   * See if there is a keyboard interrupt.                              *
   *                                                                    *
   ********************************************************************** */

static BOOL check_keyboard_interrupt(void)
{
    if (kbhit() != 0) {
        if (getch() == 27) {
            return(TRUE);
        }
    }

    return(FALSE);
}

/* **********************************************************************
   * Set the string offered to upper case.                              *
   *                                                                    *
   ********************************************************************** */

static void ucase(char *string)
{
    while (*string) {
        *string = (char)toupper(*string);
        string++;
    }
}

/* **********************************************************************
   * Set the string offered to lower case.                              *
   *                                                                    *
   ********************************************************************** */

static void lcase(char *string)
{
    while (*string) {
        *string = (char)tolower(*string);
        string++;
    }
}

/* **********************************************************************
   * Seek to the start of the disk entry table.                         *
   *                                                                    *
   ********************************************************************** */

static void seek_to_table(void)
{
    if (fseek(disk_file, table_start, SEEK_SET) != 0) {
        (void)printf("I was unable to reposistion the disk file pointer!\n");
        (void)fcloseall();
        exit(10);
    }
}

/* **********************************************************************
   * This routine is not mine.                                          *
   *                                                                    *
   * FROM: Orv Stoll                                                    *
   *                                                                    *
   * SUBJECT: wildcard routine                                          *
   *                                                                    *
   * The match() routine below does wildcard matches with a superset    *
   * of MSDOS wildcard elements. The "*" replaces any number of         *
   * characters including no characters. The "?" replaces exactly one   *
   * character and there must be a character in that position. The      *
   * "[0-9]" construct means that that character location must have a   *
   * range from the value of the first character (0) to the value of    *
   * the second character (9). In this case an ascii digit. [A-M] would *
   * be an upper case A through M.                                      *
   *                                                                    *
   *     * Origin: Port-a-Opus (Opus 0:103/517)                         *
   *                                                                    *
   ********************************************************************** */

static int match(char *fn, char *tn)
{
    int fr, lo, hi;

    while ((*tn != 0) && (*fn != 0)) {
        switch (*tn) {
            case '*':
                tn++;

                if (*tn == 0)
                    return(-1);

		while ((*fn != 0) && ((fr = match(fn, tn)) == 0))
		    fn++;

                if (*fn==0)
                    return(0);
                else
                    return(fr);

            case '[':
                tn++;
                if (*tn == 0)
                    return(0);
                else
                    lo = *tn++;

                if (*tn != '-')
                    return(0);
                else
                    tn++;

                if (*tn == 0)
                    return(0);
                else
                    hi = *tn++;

                if (*tn != ']')
                    return(0);
                else
                    tn++;

                if ((*fn >= lo) && (*fn <= hi))
                    fn++;
                else
                    return(0);

                break;

            case '?':
                tn++;

                if (*fn != 0)
                    fn++;
                else
                    return(0);

                break;

            default:
                if (*tn == *fn) {
                    tn++;
                    fn++;
                }
                else
                    return(0);
        }
    }

    while (*tn == '*')
        tn++;

    if ((*tn == 0) && (*fn == 0))
        return(-1);
    else
        return(0);
} 

/* **********************************************************************
   * Find an entry.                                                     *
   *                                                                    *
   ********************************************************************** */

static void find_entry(char *this_file)
{
    char offered_first;

    seek_to_table();
    offered_first = FALSE;

    ucase(this_file);

    while (! feof(disk_file) && !check_keyboard_interrupt()) {
        if (fread(&disk, sizeof(struct Disk_Entry), 1, disk_file) == 1) {
            ucase(disk.file_name);

            if (match(disk.file_name, this_file)) {
                (void)printf("%s %d %s\n",
                    disk.file_name,
                    disk.label,
                    disk.valid ? "" : " (deleted)");
                offered_first = TRUE;
            }
        }
    }

    if (! offered_first) {
        (void)printf("No files matching were found\n\n");
    }
}

/* **********************************************************************
   * Find out what the next disk number is and offer that information.  *
   *                                                                    *
   ********************************************************************** */

static short offer_next_disk(void)
{
    short loop;

    for (loop = 0; loop < Total_Disks; loop++) {
        if (! disk_defined[loop]) {
            (void)printf("Disk number %d is available.\n", loop + 1);
	    return(loop + 1);
        }
    }

    (void)printf("No disk space is available in the directory!\n");
    return(0);
}

/* **********************************************************************
   * See if we were able to write the data file entry.                  *
   *                                                                    *
   ********************************************************************** */

static void disk_entry_fwrite(char *buffer,
    unsigned int count,
    unsigned int many,
    FILE *thisf)
{
    if (fwrite(buffer, count, many, thisf) != 1) {
        (void)printf("I was unable to write data file!\n");
        (void)fcloseall();
        exit(10);
    }
}

/* **********************************************************************
   * Install the entry                                                  *
   *                                                                    *
   ********************************************************************** */

static void add_entry(char *this_name, short label)
{
    char execute_this[80];
    char type;

    skipspace(this_name);

    while (this_name[strlen(this_name) - 1] == ' ')
        this_name[strlen(this_name) - 1] = (char)NULL;

    disk.valid = TRUE;
    (void)strncpy(disk.file_name, this_name, 13);

    disk.label = label;

    disk_entry_fwrite((char *)&disk, sizeof(struct Disk_Entry), 1, disk_file);

    (void)printf("Save file %s\n", this_name);
}

/* **********************************************************************
   * See if we can seek.                                                *
   *                                                                    *
   ********************************************************************** */

static void disk_fseek(FILE *this_file, long this_offset, char this_set)
{
    if (fseek(this_file, this_offset, this_set) != 0) {
        (void)printf("I was unable to seek within the data base file!\n");
        (void)fcloseall();
        exit(10);
    }
}

/* **********************************************************************
   * Write the label offered to the disk.                               *
   *                                                                    *
   ********************************************************************** */

static void write_label(short this_label)
{
    char record[101];

    (void)sprintf(record, "label %c:%d\r", current_drive, this_label);
    (void)system(record);
}

/* **********************************************************************
   * Add a disk to the data base.                                       *
   *                                                                    *
   * Seek to the end of the data file, find the label on the disk, and  *
   * then store all of the file names it finds. If there is no label,   *
   * inform the operator and return.                                    *
   *                                                                    *
   * If the label is not a number, tell the operator that the disk must *
   * be numbered and also offer the first available disk number.        *
   *                                                                    *
   ********************************************************************** */

static void add_a_disk(void)
{
    int result;
    struct ffblk directory;
    short label;
    char record[101];
    short the_label;

    (void)sprintf(record, "%c:*.*", current_drive);

    result = findfirst(record, &directory, FA_LABEL);
    the_label = label = 0;

    if (result != 0) {
        (void)printf("There was no numeric disk label found on drive %c:!\n",
            current_drive);

	the_label = offer_next_disk();
	(void)printf("Should I make this disk label %d?", the_label);
	(void)fgets(record, 100, stdin);

        if (toupper(record[0]) != 'Y') {
	    return;
	}

        write_label(the_label);
	label = the_label;
    }

    if (label == 0)
	label = atoi(directory.ff_name);

    if (label == 0) {
        (void)printf("I need a numeric label on the disk in drive %c:!\n",
            current_drive);

	the_label = offer_next_disk();

        (void)printf("Should I make this disk label %d?", the_label);
	(void)fgets(record, 100, stdin);

        if (toupper(record[0]) != 'Y') {
	    return;
	}

        write_label(the_label);
	label = the_label;
    }

    if (label >= Total_Disks) {
        (void)printf("Disk number out of range! Up to %d supported!\n",
            Total_Disks);

	the_label = offer_next_disk();

        (void)printf("Should I make this disk label %d?", the_label);
	(void)fgets(record, 100, stdin);

        if (toupper(record[0]) != 'Y') {
	    return;
	}

        write_label(the_label);
	label = the_label;
    }

    (void)sprintf(record, "%c:*.*", current_drive);

    result = findfirst(record, &directory,
        FA_RDONLY | FA_HIDDEN | FA_ARCH | FA_SYSTEM | FA_DIREC);

    if (result != 0) {
        (void)printf("There were no files found on this disk!\n");
        return;
    }

    if (disk_defined[label - 1]) {
        (void)printf("Disk %d is already in the data base! Should I add continue? ", label);
        (void)fgets(record, 100, stdin);

        if (toupper(record[0]) != 'Y') {
	    (void)offer_next_disk();
	    (void)printf("Should I make this disk label %d?", the_label);
	    (void)fgets(record, 100, stdin);

	    if (toupper(record[0]) != 'Y') {
		return;
	    }
        }
    }

    (void)rewind(disk_file);
    disk_defined[label - 1]++;

    disk_entry_fwrite(disk_defined, Total_Disks, 1, disk_file);
    disk_fseek(disk_file, 0, SEEK_END);

    (void)printf("Disk number %d is being added...\n", label);
    add_entry(directory.ff_name, label);

    while (result == 0) {
        result = findnext(&directory);
        if (result == 0) {
            add_entry(directory.ff_name, label);
        }
    }
}

/* **********************************************************************
   * See if ftell returned a pass condition.                            *
   *                                                                    *
   ********************************************************************** */

static void check_ftell(long result)
{
    if (result == -1) {
        (void)printf("I was unable to tell where the data file pointer was!\n");
        (void)fcloseall();
        exit(10);
    }
}

/* **********************************************************************
   * Remove a disk from the data base.                                  *
   *                                                                    *
   * Ask the for the number. Check to see if it's actually defined. If  *
   * does not appear to be defined, ask the operator if the program     *
   * should scan through the data base anyway.                          *
   *                                                                    *
   ********************************************************************** */

static void delete_a_disk(void)
{
    char record[101];
    short label;
    char found_any;
    long entry_point, exit_point;

    (void)printf("Enter disk number: ");
    (void)fgets(record, 100, stdin);

    label = atoi(record);
    found_any = FALSE;

    if (label == 0) {
        return;
    }

    if (label >= Total_Disks) {
        (void)printf("Disk number up to %d is supported\n", Total_Disks);
        return;
    }

    if (! disk_defined[label - 1]) {
        (void)printf("That disk appears to not be defined. Look anyway? ");
        (void)fgets(record, 100, stdin);

        if (toupper(record[0]) != 'Y') {
            return;
        }
    }

    seek_to_table();

    while (! feof(disk_file)) {
        entry_point = ftell(disk_file);
        check_ftell(entry_point);

	if (fread(&disk, sizeof(struct Disk_Entry), 1, disk_file) == 1) {
	    exit_point = ftell(disk_file);
            check_ftell(exit_point);

            if (disk.valid) {
                if (disk.label == label) {
                    disk.valid = FALSE;
                    disk_fseek(disk_file, entry_point, SEEK_SET);
                    disk_entry_fwrite((char *)&disk, sizeof(struct Disk_Entry), 1, disk_file);
		    (void)printf("File %s was erased\n", disk.file_name);
		    found_any = TRUE;
                    disk_fseek(disk_file, exit_point, SEEK_SET);
                }
            }
        }
    }

    if (found_any) {
	rewind(disk_file);
	disk_defined[label - 1] = FALSE;
        disk_entry_fwrite(disk_defined, Total_Disks, 1, disk_file);
    }
    else {
	(void)printf("No files were found\n\n");
    }
}

/* **********************************************************************
   * Pack out all of the deleted entries.                               *
   *                                                                    *
   ********************************************************************** */

static void pack_the_data_file(void)
{
    char *file_name;
    FILE *temp;
    short retained, erased;

    file_name = mktemp("DSXXXXXX");
    retained = erased = 0;

    if (file_name == (char *)NULL) {
        (void)printf("Couldn't create a temporary file name!\n");
        (void)printf("Most likely the data base has already been packed.\n");
        return;
    }

    if ((temp = fopen(file_name, "wb")) == (FILE *)NULL) {
        (void)printf("Unable to create temp file: %s\n", file_name);
        return;
    }

    (void)rewind(disk_file);

    if (fread(disk_defined, Total_Disks, 1, disk_file) != 1) {
        (void)printf("I could not read file index records from data file!\n");
        (void)fclose(temp);

        if (unlink(file_name) != 0) {
            (void)printf("Unable to erased temporary file: %s\n", file_name);
        }

        return;
    }

    if (fwrite(disk_defined, Total_Disks, 1, temp) != 1) {
        (void)printf("I could not write index to temporary file!\n");
        (void)fclose(temp);

        if (unlink(file_name) != 0) {
            (void)printf("Unable to erased temporary file: %s\n", file_name);
        }

        return;
    }

    while (! feof(disk_file)) {
        if (fread(&disk, sizeof(struct Disk_Entry), 1, disk_file) == 1) {
            if (disk.valid) {
		if (fwrite(&disk, sizeof(struct Disk_Entry), 1, temp) != 1) {
		    (void)printf("Unable to write record to temporary file %s\n", file_name);
                    (void)fclose(temp);

                    if (unlink(file_name) != 0) {
                        (void)printf("Unable to erased temporary file: %s\n", file_name);
                    }

                    return;
                }

                retained++;
            }
            else {
                erased++;
            }
        }
    }

    (void)fclose(temp);
    (void)fclose(disk_file);

    if (unlink("DISK.CAT") != 0) {
        (void)printf("I was unable to erase file: DISK.CAT!\n");
        fcloseall();
        exit(10);
    }

    if (rename(file_name, "DISK.CAT") != 0) {
        (void)printf("Disk file couldn't be renamed from %s to DISK.CAT!\n", file_name);
        (void)printf("Rename it, please!\n");
        (void)fcloseall();
        exit(0);
    }

    (void)printf("%d records erased with %d retained\n", erased, retained);

    if ((disk_file = fopen("disk.cat", "r+b")) == (FILE *)NULL) {
	(void)printf("Can't reopen DISK.CAT file!\n");
        (void)fcloseall();
	exit(10);
    }
}

/* **********************************************************************
   * Offer a display of the files on a disk.                            *
   *                                                                    *
   ********************************************************************** */

static void list_a_disk(void)
{
    char record[101];
    short label, count;

    (void)printf("Enter disk number to view: ");
    (void)fgets(record, 100, stdin);

    label = atoi(record);

    if (label == 0) {
        return;
    }

    if (label >= Total_Disks) {
        (void)printf("That disk number is out of bounds!\n");
        (void)printf("This program supports up to %d disks.\n", Total_Disks);
        return;
    }

    if (! disk_defined[label - 1]) {
        (void)printf("It appears that disk isn't defined yet. Should I continue? ");
        (void)fgets(record, 100, stdin);

        if (toupper(record[0]) != 'Y') {
            return;
        }
    }

    seek_to_table();
    count = 0;

    while (! feof(disk_file)) {
        if (fread(&disk, sizeof(struct Disk_Entry), 1, disk_file) == 1) {
	    if (disk.label == label) {
                (void)printf("%s %s\n",
                    disk.file_name,
                    disk.valid ? "" : " (deleted)");

                count++;
            }
        }
    }

    if (count > 0) {
        (void)printf("Total of %d file%s found\n\n",
            count,
            count == 1 ? "" : "s");
    }
    else {
        (void)printf("No files were found\n\n");
    }
}

/* **********************************************************************
   * Search for a file. This calls the same routine used by the command *
   * line request yet additional information is requested.              *
   *                                                                    *
   ********************************************************************** */

static void search_for_files(void)
{
    char record[101];

    (void)printf("Enter file to look for: ");
    (void)fgets(record, 100, stdin);

    if (strlen(record) == 1) {
        return;
    }

    record[strlen(record) - 1] = (char)NULL;
    find_entry(record);
}

/* **********************************************************************
   * Change the current drive                                           *
   *                                                                    *
   ********************************************************************** */

static void change_drive(void)
{
    (void)printf("Enternew drive designation: ");
    current_drive = toupper(getche());
}

/* **********************************************************************
   * This will produce a listing of all files on all disks. It will put *
   * the output is a nice readable format. This data is sent to a file  *
   * called DISK.OUT which is usefull for providing a list of all       *
   * files.                                                             *
   *                                                                    *
   ********************************************************************** */

static void full_listing(void)
{
    short count;
    char report[81], hold[20];
    FILE *output;
    char the_date[27];
    long itstime;
    short disk_count, loop;
    long file_count;

    if ((output = fopen("DISK.OUT", "wt")) == (FILE *)NULL) {
        (void)printf("I was unable to create the output file!\n");
        return;
    }

    (void)time(&itstime);
    (void)strncpy(the_date, ctime(&itstime), 26);
    (void)sprintf(report, "             List of all floppies as of %s", the_date);
    (void)fputs(report, output);
    (void)fputs("------------------------------------------------------", output);
    (void)fputs("-----------------------\n", output);

    count = 0;
    report[0] = (char)NULL;
    disk_count = 0;
    file_count = 0L;

    for (loop = 0; loop < Total_Disks; loop++)
        if (disk_defined[loop])
            disk_count++;

    seek_to_table();

    while (! feof(disk_file)) {
        if (fread(&disk, sizeof(struct Disk_Entry), 1, disk_file) == 1) {
            if (disk.valid) {
                (void)sprintf(hold, "%13s %03d", disk.file_name, disk.label);
                (void)strcat(report, hold);

                count++;
                file_count++;

                if (count == 4) {
                    (void)strcat(report, "\n");
                    fputs(report, output);
                    report[0] = (char)NULL;
                    count = 0;
                }
                else {
                    (void)strcat(report, " | ");
                }
            }
        }
    }

    if (strlen(report) > 1) {
        (void)fputs(report, output);
    }

    (void)sprintf(report, "\n\nIn %d disks, there were %ld files\n\n",
        disk_count, file_count);

    (void)fputs(report, output);

    fclose(output);
}

/* **********************************************************************
   * Reset the floppy subsystem                                         *
   *                                                                    *
   ********************************************************************** */

static void reset_floppy(void)
{
    int result;

    result = biosdisk(0, current_drive - 'A', 0, 0, 0, 0, NULL);

    if (result != 0x00) {
	(void)printf("Reset drive %c: failed.  Error: %d\n",
	    current_drive, result);
    }

    result = biosdisk(13, current_drive - 'A', 0, 0, 0, 0, NULL);

    if (result != 0x00) {
        (void)printf("Alternate reset drive %c: failed.  Error: %d\n",
	    current_drive, result);
    }
}

/* **********************************************************************
   * The main entry point.                                              *
   *                                                                    *
   * If there is a file name offered, simply call the finder, else we   *
   * need to ask if they are adding or eraseing entries.                *
   *                                                                    *
   ********************************************************************** */

void main(int argc, char **argv)
{
    char record[101];
    short loop;
    int byte;

    for (loop = 0; loop < Total_Disks; loop++)
        disk_defined[loop] = 0;

    current_drive = 'B';

    if ((disk_file = fopen("disk.cat", "r+b")) == (FILE *)NULL) {
        (void)printf("Can't find file: DISK.CAT! Should I create it? ");
        (void)fgets(record, 100, stdin);

        if (toupper(record[0]) == 'Y') {
            if ((disk_file = fopen("disk.cat", "w+b")) == (FILE *)NULL) {
                (void)printf("Could not create file: DISK.CAT!\n");
                exit(10);
            }

            disk_entry_fwrite(disk_defined, Total_Disks, 1, disk_file);
            (void)rewind(disk_file);
        }
        else {
            exit(0);
        }
    }

    if (fread(disk_defined, Total_Disks, 1, disk_file) != 1) {
        (void)printf("I could not read file: DISK.CAT! Reason is not known!\n");
        exit(10);
    }

    table_start = ftell(disk_file);
    check_ftell(table_start);

    if (argc == 2) {
        find_entry(argv[1]);
        exit(0);
    }

    while (TRUE) {
        (void)printf("\nCurrent drive: %c:\n", current_drive);
        (void)printf("(F)ull listing to file, (C)hange drive\n");
	(void)printf("(A)dd,  (D)elete, (E)xit\n");
        (void)printf("(L)ist, (P)ack,   (S)earch: ");

        sound(1100);
	delay(10);
	nosound();
        byte = getche();
        (void)printf("\n");

        switch (byte) {
            case  'A':
            case  'a':
                if (current_drive == 'B') {
                    reset_floppy();
                }

                add_a_disk();
                break;

            case  'D':
            case  'd':
                delete_a_disk();
                break;

            case  'P':
            case  'p':
                pack_the_data_file();
                break;

            case  'L':
            case  'l':
                list_a_disk();
                break;

            case  'S':
            case  's':
                search_for_files();
                break;

            case  'F':
            case  'f':
                full_listing();
                break;

            case  'C':
            case  'c':
                change_drive();
                break;

            case  'E':
            case  'e':
                (void)fcloseall();
                exit(0);
        }
    }
}


