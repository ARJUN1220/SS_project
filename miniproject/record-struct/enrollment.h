#ifndef ENROLLMENT_RECORD
#define ENROLLMENT_RECORD

struct Enrollment
{
    int id; // 0, 1, 2 ....
    char courseid[6];
    char studentid[6];
    bool status;
    time_t enroll_time;    
};

#endif
