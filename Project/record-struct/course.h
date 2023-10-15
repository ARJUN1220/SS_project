#ifndef COURSE_RECORD
#define COURSE_RECORD

struct Course
{
    int id; // 0, 1, 2 ....
    char name[25];
    int facultyID;
    char facultyName[80];
    int numSeats;
    char enrolledStudents[100][100];
};

#endif
