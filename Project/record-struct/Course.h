#ifndef COURSE_RECORD
#define COURSE_RECORD

struct Course
{
    int id; // 0, 1, 2 ....
    char name[25];
    char facultyloginid[10];
    char department [30];
    int no_of_seats;
    int credits;
    int no_of_available_seats;
    char courseid[6];
    char status[8];    
};

#endif
