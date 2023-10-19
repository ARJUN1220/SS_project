#ifndef STUDENT_RECORD
#define STUDENT_RECORD

struct Student
{
    int id; // 0, 1, 2 ....
    char name[25];
    int age; 
    char address[160];
    char email[80];

    // Login Credentials
    char loginid[30]; // Format : name-id (name will the first word in the structure member `name`)
    char password[30];
    char access;
    
};

#endif
