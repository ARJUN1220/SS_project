#ifndef STUDENT_RECORD
#define STUDENT_RECORD

struct Student
{
    int id; // 0, 1, 2 ....
    char name[25];
    char gender; // M -> Male, F -> Female, O -> Other
    int age;
    char email[80];
    bool status;
    // Login Credentials
    char login[30]; // Format : name-id (name will the first word in the structure member `name`)
    char password[30];
};

#endif
