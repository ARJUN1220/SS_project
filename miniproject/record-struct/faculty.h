#ifndef FACULTY_RECORD
#define FACULTY_RECORD

struct Faculty
{
    int id; // 0, 1, 2 ....
    char name[25];
    char department[6];
    char designation[50]; 
    char address[160];
    char email[80];

    // Login Credentials
    char loginid[30]; // Format : name-id (name will the first word in the structure member `name`)
    char password[30];
    
};

#endif
