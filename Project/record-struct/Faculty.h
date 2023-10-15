#ifndef FACULTY_RECORD
#define FACULTY_RECORD

struct Faculty
{
    int id; // 0, 1, 2 ....
    char name[25];
    char gender; // M -> Male, F -> Female, O -> Other
    char course[50];
    int num_course;
    char department[50];
    char designation[50];
    char email[80];
    // Login Credentials
    char login[30]; // Format : name-id (name will the first word in the structure member `name`)
    char password[30];
};

#endif
