#ifndef FACULTY_FUNCTIONS
#define FACULTY_FUNCTIONS

#include <sys/ipc.h>
#include <sys/sem.h>
#include<time.h>
#include "server-constants.h"

struct Faculty loggedInFaculty;

// structure which is used to sort enrollment records by enrollment time 
struct middleware{
    int myid;
    time_t mytime;
};
struct middleware myDict[100];


int semid;

bool lock_critical_section(struct sembuf *semOp);
bool unlock_critical_section(struct sembuf *sem_op);
int faculty_operation_handler(int connFD);
bool add_course(int connFD);
int view_offering_course(int connFD);
int remove_course(int connFD);
int modify_course(int connFD);
int change_password(int connFD);
bool logout(int connFD);

// function for faculty operations
int faculty_operation_handler(int connFD){

    // login handling
    if(login_handler(2,connFD,&loggedInFaculty,NULL)){
        key_t semKey = ftok(FACULTY_FILE,loggedInFaculty.id);
        union semun
        {
          int val; 
        } semSet;

        int semctlStatus;
        semid = semget(semKey, 1, 0); // Get the semaphore if it exists
        if (semid == -1)
        {
            semid = semget(semKey, 1, IPC_CREAT | 0700); // Create a new semaphore
            if (semid == -1)
            {
                perror("Error while creating semaphore!");
                _exit(1);
            }

            semSet.val = 1; // Set a binary semaphore
            semctlStatus = semctl(semid, 0, SETVAL, semSet);
            if (semctlStatus == -1)
            {
                perror("Error while initializing a binary sempahore!");
                _exit(1);
            }
        }

        ssize_t writeBytes, readBytes;            
        char readBuffer[1000], writeBuffer[1000];
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, LOGIN_SUCCESS);
        
        while(1){
            strcat(writeBuffer, "\n");
            strcat(writeBuffer, FACULTY_MENU);
            
            // displaying facutly menu
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing FACULTY_MENU to client!");
                return 0;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for FACULTY_MENU");
                return 0;
            }

            int choice = atoi(readBuffer);
            switch (choice)
            {
            case 1:
                view_offering_course(connFD);
                break;
            case 2:
                add_course(connFD);
                 break;
            case 3: 
                remove_course(connFD);
                break;
            case 4:
                modify_course(connFD);
                break;
            case 5:
                change_password(connFD);
                break;
            case 6:
                logout(connFD);
                break;    
            default:
                writeBytes = write(connFD,"wrong choice ^",14);
                readBytes = read(connFD,readBuffer,sizeof(readBuffer));
                break;
            }
        }
    }
    else
    {
        // FACULTY LOGIN FAILED
        return 0;
    }
    return 1;
}

// removing course details
int remove_course(int connFD){
    
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];
    struct Course course;
    int courseID;
    off_t offset;
    int lockingStatus;

    writeBytes = write(connFD, DEL_COURSE_ID, strlen(DEL_COURSE_ID));
    if (writeBytes == -1)
    {
        perror("Error while writing DEL_COURSE_ID message to client!");
        return 0;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while reading course ID from client!");
        return 0;
    }

    char *ftPosition = strstr(readBuffer, "C-");
    char *numberStart = NULL;
    if(ftPosition!=NULL) {
        numberStart = ftPosition + strlen("C-");
        courseID = atoi(numberStart);
    }
    else{
        write(connFD,"wrong courseid",15);
        return 0;
    }

    int courseFileDescriptor = open(COURSE_FILE, O_RDONLY);
    if (courseFileDescriptor == -1)
    {
        // Course File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer,"course file id doesn't exists");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing course id doesnt exists message to client!");
            return 0;
        }
        return 0;
    }
    
    offset = lseek(courseFileDescriptor, (courseID-1)* sizeof(struct Course), SEEK_SET);
    if (errno == EINVAL)
    {
        // course record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer,"Course id doesn't exists");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing course id doesnt exists message to client!");
            return 0;
        }
        return 0;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required course record!");
        return 0;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Course), getpid()};
    struct flock lock1 = {F_RDLCK,SEEK_SET,offset,sizeof(struct Enrollment),getpid()}; 
    // Lock the record to be read
    lockingStatus = fcntl(courseFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on course record!");
        return 0;
    }

    readBytes = read(courseFileDescriptor, &course, sizeof(struct Course));
    if (readBytes == -1)
    {
        perror("Error while reading course record from the file!");
        return 0;
    }

    // Unlock the record
    lock.l_type = F_UNLCK;
    fcntl(courseFileDescriptor, F_SETLK, &lock);

    close(courseFileDescriptor);

    //checking this course belongs to the loggedinfaculty
    if(strcmp(loggedInFaculty.loginid,course.facultyloginid)!=0){
        write(connFD,"Not your course to remove ^",27);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }

    // also checking whether the course is present in the catalog or not
    else if(!course.status) {
        write(connFD,"Course not found to remove ^",29);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }
   
    // updating course record to notactive
    courseFileDescriptor = open(COURSE_FILE, O_WRONLY);
    if (courseFileDescriptor == -1)
    {
        perror("Error while opening course file");
        return 0;
    }
    offset = lseek(courseFileDescriptor, (courseID-1) * sizeof(struct Course), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required course record!");
        return 0;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(courseFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on course record!");
        return 0;
    }

    //Make it notactive to remove
    course.status = false;

    writeBytes = write(courseFileDescriptor, &course, sizeof(struct Course));
    if (writeBytes == -1)
    {
        perror("Error while writing update course info into file");
    }

    lock.l_type = F_UNLCK;
    fcntl(courseFileDescriptor, F_SETLKW, &lock);
    close(courseFileDescriptor);
 
    // make uneroll of last enrolled students for those courses
    struct Enrollment enroll;
    int enrollfd;
    bool flag=false;
    int temp1[15][10];
    int i,n;
    int count=0;

    enrollfd = open(ENROLL_FILE,O_RDONLY);
    while((n = read(enrollfd, &enroll, sizeof(struct Enrollment))) > 0) {
       if(enroll.status && (strcmp(enroll.courseid,readBuffer)==0)){              
         temp1[i][10] = enroll.id;
         i++;
         count++;
         flag = true;  
       }
    }
    close(enrollfd);

    if(flag == true) {
       for(int i = 0; i < count; i++) {
            enrollfd = open(ENROLL_FILE, O_RDONLY);
            int offset = lseek(enrollfd, (temp1[i][10] - 1) * sizeof(struct Enrollment), SEEK_SET);
            if(offset == -1){
                perror("Error while seeking to required enrollment record!");
                return 0;
            }
            lock1.l_type = F_RDLCK;
            lock1.l_start = offset;
            int lockingStatus = fcntl(enrollfd, F_SETLKW, &lock1);
            if (lockingStatus == -1){
                perror("Error while obtaining read lock on enrollment record!");
                return 0;
            }
            readBytes = read(enrollfd, &enroll, sizeof(struct Enrollment));
            if(readBytes == -1){
                perror("Error while reading enrollment record from the file!");
                return 0;
            }
            lock1.l_type = F_UNLCK;
            lockingStatus = fcntl(enrollfd, F_SETLK, &lock1);   
            close(enrollfd);
            enroll.status = false;
  
            enrollfd = open(ENROLL_FILE,O_WRONLY);
            if (enrollfd == -1){
                perror("Error while opening enrollment file");
                return 0;
            }
            offset = lseek(enrollfd, (temp1[i][10]-1) * sizeof(struct Enrollment), SEEK_SET);
            if(offset == -1){
                perror("Error while seeking to required enrollment record!");
                return 0;
            }
            lock1.l_type = F_WRLCK;
            lock1.l_start = offset;
            lockingStatus = fcntl(enrollfd, F_SETLKW, &lock);
            if(lockingStatus == -1){
                perror("Error while obtaining write lock on enrollment record!");
                return 0;
            }
            writeBytes = write(enrollfd, &enroll, sizeof(struct Enrollment));
            if(writeBytes == -1){
                perror("Error while writing update enrollment info into file");
            }
            lock1.l_type = F_UNLCK;
            fcntl(enrollfd, F_SETLKW, &lock1);
            close(enrollfd);
       }
    }

    // success message
    writeBytes = write(connFD, DEL_COURSE_SUCCESS, strlen(DEL_COURSE_SUCCESS));
    if (writeBytes == -1)
    {
        perror("Error while writing DEL_COURSE_SUCCESS message to client!");
        return 0;
    }
    readBytes = read(connFD,readBuffer,sizeof(readBuffer));
    return 1;
}

// To get sorted records according to enroll time
int compareEnrollmentTime(const void* a, const void* b) {
    const struct middleware* enrollA = (const struct middleware*)a;
    const struct middleware* enrollB = (const struct middleware*)b;
    
    char timeStrA[20];
    char timeStrB[20];

    strftime(timeStrA, sizeof(timeStrA), "%Y-%m-%d %H:%M", localtime(&(enrollA->mytime)));
    strftime(timeStrB, sizeof(timeStrB), "%Y-%m-%d %H:%M", localtime(&(enrollB->mytime)));

    // Compare the strings in reverse order (descending)
    return strcmp(timeStrB, timeStrA);
}
    
// modifying course details    
int modify_course(int connFD){
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];
    struct Course course;
    int courseID;
    off_t offset;
    int lockingStatus;

    // display enter course id message
    writeBytes = write(connFD, MOD_COURSE_ID, strlen(MOD_COURSE_ID));
    if (writeBytes == -1)
    {
        perror("Error while writing MOD_COURSE_ID message to client!");
        return 0;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while reading course ID from client!");
        return 0;
    }


    char *ftPosition = strstr(readBuffer, "C-");
    char *numberStart = NULL;
    if(ftPosition!=NULL) {
        numberStart = ftPosition + strlen("C-");
        courseID = atoi(numberStart);
    }
    else{
        write(connFD,"wrong courseid ^",17);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }

    int courseFileDescriptor = open(COURSE_FILE, O_RDONLY);
    if (courseFileDescriptor == -1)
    {
        // Faculty File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer,"course id doesn't exists ^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing course id doesnt exists message to client!");
            return 0;
        }
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }
    
    // checking whether our record id is greater than the last record is present
    offset = lseek(courseFileDescriptor,-sizeof(struct Course),SEEK_END);
    readBytes = read(courseFileDescriptor, &course, sizeof(struct Course));
    if (readBytes == -1)
    {
        perror("Error reading course record from file!");
        return false;
    }
    if(courseID>course.id){
        write(connFD,"Invalid course id ^",20);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }
    close(courseFileDescriptor);
    courseFileDescriptor = open(COURSE_FILE,O_RDONLY);

    // seeking to the record of file
    offset = lseek(courseFileDescriptor, (courseID-1)* sizeof(struct Course), SEEK_SET);
    if (errno == EINVAL)
    {
        // course record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer,"Course id doesn't exists ^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing course id doesnt exists message to client!");
            return 0;
        }
        readBytes =read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required course record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Course), getpid()};
    struct flock lock1 = {F_RDLCK,SEEK_SET,offset,sizeof(struct Enrollment),getpid()};
    // Lock the record to be read
    lockingStatus = fcntl(courseFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on course record!");
        return 0;
    }

    readBytes = read(courseFileDescriptor, &course, sizeof(struct Course));
    
    // if course is not present
    if(!course.status) {
        write(connFD,"Invalid course id ^",20);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }

    int noofavailseats = course.no_of_available_seats;
    int noofseatsbefore = course.no_of_seats;
    if (readBytes == -1)
    {
        perror("Error while reading course record from the file!");
        return 0;
    }

    // Unlock the record
    lock.l_type = F_UNLCK;
    fcntl(courseFileDescriptor, F_SETLK, &lock);
    close(courseFileDescriptor);

    // if course is not offered by loggedinfaculty
    if(strcmp(loggedInFaculty.loginid,course.facultyloginid)!=0){
        write(connFD,"Not your course to modify ^",27);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }
    writeBytes = write(connFD, MOD_COURSE_MENU, strlen(MOD_COURSE_MENU));
    if (writeBytes == -1)
    {
        perror("Error while writing MOD_COURSE_MENU message to client!");
        return 0;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while getting course modification menu choice from client!");
        return 0;
    }

    
    int choice = atoi(readBuffer);

    //validation for choice
    if (choice == 0)
    { 
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return 0;
        }
        return 0;
    }

    bzero(readBuffer, sizeof(readBuffer));

    switch (choice){
    
    // new course name
    case 1:
        writeBytes = write(connFD, MOD_COURSE_NEW_NAME, strlen(MOD_COURSE_NEW_NAME));
        if (writeBytes == -1)
        {
            perror("Error while writing MOD_COURSE_NEW_NAME message to client!");
            return 0;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for course's new name from client!");
            return 0;
        }
         
        //validation for name
        for(int i = 0;readBuffer[i]!='\0';i++) {
           if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
               write(connFD,"Invalid course name ^",21);
               readBytes= read(connFD,readBuffer,sizeof(readBuffer));
               return 0;
            }
        } 

        strcpy(course.name, readBuffer);
        break;

    // new course department name
    case 2:
        writeBytes = write(connFD, MOD_COURSE_NEW_DEPARTMENT, strlen(MOD_COURSE_NEW_DEPARTMENT));
        if (writeBytes == -1)
        {
            perror("Error while writing MOD_COURSE_NEW_DEPARTMENT message to client!");
            return 0;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for course's new department from client!");
            return 0;
        }
        
        //validation for course dept
        for(int i = 0;readBuffer[i]!='\0';i++) {
            if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
                write(connFD,"Invalid deptname ^",18);
                readBytes= read(connFD,readBuffer,sizeof(readBuffer));
                return 0;
            }
        }
    
        strcpy(course.department,readBuffer);
        break;

    // new no of seats
    case 3:
        writeBytes = write(connFD, MOD_COURSE_NEW_NOOFSEATS, strlen(MOD_COURSE_NEW_NOOFSEATS));
        if (writeBytes == -1)
        {
            perror("Error while writing MOD_COURSE_NEW_NOOFSEATS message to client!");
            return 0;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for course's new noofseats from client!");
            return 0;
        }
        int value = atoi(readBuffer);

        //validation for no of seats
        if(value<0){
            write(connFD,"Invalid total seats ^",21);
            readBytes = read(connFD,readBuffer,sizeof(readBuffer));
            return 0;
        }

        // if no of seats are zero update available seats 
        if(value==0)
           course.no_of_available_seats=0;

        // if new value entered is greater than before seats   
        if(value>noofseatsbefore){
            course.no_of_available_seats= course.no_of_available_seats+(value-noofseatsbefore);
        }
        else if(value<noofseatsbefore){
            int temp = value-noofseatsbefore;
            int n;
            char unenroll_course[10];
            strcpy(unenroll_course,course.courseid);

            // checking whether there is need to uneroll last enrolled students 
            if((course.no_of_available_seats+(temp))<0){
                int temp2 = -1*(course.no_of_available_seats+temp);
                course.no_of_available_seats=0;
                int i,size;
                struct Enrollment enroll1;

                int enrollfd = open(ENROLL_FILE,O_RDWR);
                
                while((n = read(enrollfd, &enroll1, sizeof(struct Enrollment)))>0){
                    if((enroll1.status) && (strcmp(enroll1.courseid,unenroll_course)==0)){
                       myDict[i].myid = enroll1.id;
                       myDict[i].mytime = enroll1.enroll_time;
                       size++;
                       i++;
                    }                                      
                }
                close(enrollfd);
                
                // sorting records according to enroll time
                qsort(myDict,size, sizeof(struct middleware), compareEnrollmentTime);
                int a;

                //unerolling latest enrolled students
                while(a < temp2 && a < size) {
                    int ID = myDict[a].myid;
                    struct Enrollment enroll;
                    enrollfd = open(ENROLL_FILE, O_RDONLY);
                    offset = lseek(enrollfd, (ID - 1) * sizeof(struct Enrollment), SEEK_SET);
                    if(offset == -1) {
                        perror("Error while seeking to required course record!");
                        return 0;
                    }
                    lock1.l_type = F_RDLCK;
                    lock1.l_start = offset;
                    lockingStatus = fcntl(enrollfd, F_SETLKW, &lock);
                    if(lockingStatus == -1){
                        perror("Error while obtaining read lock on enrollment record!");
                        return 0;
                    }
                    readBytes = read(enrollfd, &enroll, sizeof(struct Enrollment));
                    if(readBytes == -1){
                        perror("Error while reading enrollment record from the file!");
                        return 0;
                    }
                    lock.l_type = F_UNLCK;
                    lockingStatus = fcntl(enrollfd, F_SETLK, &lock);   
                    close(enrollfd);

                    enroll.status = false;
                    enrollfd = open(ENROLL_FILE,O_WRONLY);
                    if (enrollfd == -1){
                        perror("Error while opening enrollment file");
                        return 0;
                    }
                    offset = lseek(enrollfd, (ID-1) * sizeof(struct Enrollment), SEEK_SET);
                    if(offset == -1){
                      perror("Error while seeking to required enrollment record!");
                      return 0;
                    }
                    lock.l_type = F_WRLCK;
                    lock.l_start = offset;
                    lockingStatus = fcntl(enrollfd, F_SETLKW, &lock);
                    if(lockingStatus == -1){
                        perror("Error while obtaining write lock on enrollment record!");
                        return 0;
                    }
                    writeBytes = write(enrollfd, &enroll, sizeof(struct Enrollment));
                    if (writeBytes == -1){
                        perror("Error while writing update enrollment info into file");
                    }

                     lock.l_type = F_UNLCK;
                     fcntl(enrollfd, F_SETLKW, &lock);
                     close(enrollfd);
                     a++;
                }
            }
            else    
                course.no_of_available_seats=course.no_of_available_seats+(temp);
        }   
        course.no_of_seats= value;
        break;

    // new course credits
    case 4:
        writeBytes = write(connFD, MOD_COURSE_NEW_CREDITS, strlen(MOD_COURSE_NEW_CREDITS));
        if (writeBytes == -1)
        {
            perror("Error while writing MOD_COURSE_NEW_CREDITS message to client!");
            return 0;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for course's new credits from client!");
            return 0;
        }
        int vale =atoi(readBuffer);
        
        if(vale<=0){
            write(connFD,"wrong credits entered ^",23);
            readBytes = read(connFD,readBuffer,sizeof(readBuffer));
            return 0;
        }
        course.credits=atoi(readBuffer);
        break;

    default:
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, INVALID_MENU_CHOICE);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing INVALID_MENU_CHOICE message to client!");
            return false;
        }
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }

    //updating course record
    courseFileDescriptor = open(COURSE_FILE, O_WRONLY);
    if (courseFileDescriptor == -1)
    {
        perror("Error while opening course file");
        return 0;
    }
    offset = lseek(courseFileDescriptor, (courseID-1) * sizeof(struct Course), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required course record!");
        return 0;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(courseFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on course record!");
        return 0;
    }

    writeBytes = write(courseFileDescriptor, &course, sizeof(struct Course));
    if (writeBytes == -1)
    {
        perror("Error while writing update course info into file");
    }

    lock.l_type = F_UNLCK;
    fcntl(courseFileDescriptor, F_SETLKW, &lock);

    close(courseFileDescriptor);

    // success message
    writeBytes = write(connFD, MOD_COURSE_SUCCESS, strlen(MOD_COURSE_SUCCESS));
    if (writeBytes == -1)
    {
        perror("Error while writing MOD_COURSE_SUCCESS message to client!");
        return 0;
    }
    readBytes = read(connFD,readBuffer,sizeof(readBuffer));
    return 1;    
}

// Viewing course details
int view_offering_course(int connFD){
    ssize_t readBytes, writeBytes;             
    char readBuffer[1000], writeBuffer[10000]; 
    char tempBuffer[1000];
    struct Course fetchcourse;
    int courseFileDescriptor;
    struct flock lock = {F_RDLCK, SEEK_SET, 0, sizeof(struct Course), getpid()};

    writeBytes = write(connFD, GET_COURSE_ID, strlen(GET_COURSE_ID));
    if (writeBytes == -1){
        perror("Error while writing GET_COURSE_ID message to client!");
        return 0;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1){
        perror("Error getting course ID from client!");
        return 0;
    }

    // retreiving id from course code
    char *ftPosition = strstr(readBuffer, "C-");
    char *numberStart = NULL;
    int courseID;
    if(ftPosition!=NULL) {
        // Move the pointer to the character right after "C-"
        numberStart = ftPosition + strlen("C-");
        // Convert the numeric part to an integer
        courseID = atoi(numberStart);
        
    }
    else{
        write(connFD,"wrong courseid ^",17);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    } 


    courseFileDescriptor = open(COURSE_FILE, O_RDONLY);
    if (courseFileDescriptor == -1)
    {
        // Course File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "course file id doesn't exists ^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes ==-1)
        {
            perror("Error while writing COURSE_ID_DOESNT_EXIT message to client!");
            return 0;
        }
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }

    // checking whether our record id is greater than last record it present
    int offset = lseek(courseFileDescriptor,-sizeof(struct Course),SEEK_END);
    readBytes = read(courseFileDescriptor, &fetchcourse, sizeof(struct Course));
    if (readBytes == -1)
    {
        perror("Error reading course record from file!");
        return false;
    }

    //if throw error
    if(courseID>fetchcourse.id){
        write(connFD,"Invalid course id ^",20);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }
    close(courseFileDescriptor);

    courseFileDescriptor = open(COURSE_FILE,O_RDONLY);

    // seeking to the record for reading
    offset = lseek(courseFileDescriptor, (courseID-1) * sizeof(struct Course), SEEK_SET);
    if (errno == EINVAL)
    {
        // Course record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Course id doesn't exists $");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing COURSE_ID_DOESNT_EXIT message to client!");
            return 0;
        }
        return 0;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required course record!");
        return false;
    }
    lock.l_start = offset;

    int lockingStatus = fcntl(courseFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining read lock on the Course file!");
        return false;
    }

    readBytes = read(courseFileDescriptor, &fetchcourse, sizeof(struct Course));
    if (readBytes == -1)
    {
        perror("Error reading course record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(courseFileDescriptor, F_SETLK, &lock);

    bzero(writeBuffer, sizeof(writeBuffer));

    // if the courseid is of not loggedinfaculty throw error
    if(strcmp(fetchcourse.facultyloginid,loggedInFaculty.loginid)!=0){
        write(connFD,NOT_YOUR_COURSE,strlen(NOT_YOUR_COURSE));
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }

    // if the course not at all present in the catalog throw error
    else if(!fetchcourse.status) {
        write(connFD,"No course found ^",18);
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        return 0;
    }

    // displaying course details
    sprintf(writeBuffer, "********* Course Details *********  \n\tName: %s\n\tDepartment : %s\n\tNo of Seats: %d\n\tCredits : %d\n\tNo of available seats: %d\n\tCourse-id: %s", fetchcourse.name, fetchcourse.department,fetchcourse.no_of_seats,fetchcourse.credits,fetchcourse.no_of_available_seats,fetchcourse.courseid);
    strcat(writeBuffer, "\n\nYou'll now be redirected to the Faculty menu ^");

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing course info to client!");
        return 0;
    }
    readBytes = read(connFD,readBuffer,sizeof(readBuffer)); //dummy read
    return true;
}

bool logout(int connFD){
    ssize_t writeBytes;
    writeBytes = write(connFD, LOGOUT, strlen(LOGOUT));
    return false;
}

int change_password(int connFD){
   ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000], hashedPassword[1000];
    char newPassword[1000];

    // Lock the critical section
    struct sembuf semOp = {0, -1, SEM_UNDO};
    int semopStatus = semop(semid, &semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }

    // Enter your old password
    writeBytes = write(connFD, PASSWORD_CHANGE_OLD_PASS, strlen(PASSWORD_CHANGE_OLD_PASS));
    if (writeBytes == -1)
    {
        perror("Error writing PASSWORD_CHANGE_OLD_PASS message to client!");
        unlock_critical_section(&semOp);
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading old password response from client");
        unlock_critical_section(&semOp);
        return 0;
    }

    // If password matches with the password
    if (strcmp(crypt(readBuffer, SALT_BAE), loggedInFaculty.password) == 0)
    {
        // Asking for the new password
        writeBytes = write(connFD, PASSWORD_CHANGE_NEW_PASS, strlen(PASSWORD_CHANGE_NEW_PASS));
        if (writeBytes == -1)
        {
            perror("Error writing PASSWORD_CHANGE_NEW_PASS message to client!");
            unlock_critical_section(&semOp);
            return false;
        }
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading new password response from client");
            unlock_critical_section(&semOp);
            return false;
        }

        strcpy(newPassword, crypt(readBuffer, SALT_BAE));

        //Asking to reenter the new password
        writeBytes = write(connFD, PASSWORD_CHANGE_NEW_PASS_RE, strlen(PASSWORD_CHANGE_NEW_PASS_RE));
        if (writeBytes == -1)
        {
            perror("Error writing PASSWORD_CHANGE_NEW_PASS_RE message to client!");
            unlock_critical_section(&semOp);
            return false;
        }
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error reading new password reenter response from client");
            unlock_critical_section(&semOp);
            return false;
        }


         // New & reentered passwords match
        if (strcmp(crypt(readBuffer, SALT_BAE), newPassword) == 0)
        {
            strcpy(loggedInFaculty.password, newPassword);
            int facultyFileDescriptor = open(FACULTY_FILE, O_WRONLY);
            if (facultyFileDescriptor == -1)
            {
                perror("Error opening faculty file!");
                unlock_critical_section(&semOp);
                return false;
            }

            //seek to the record to change the password attribute
            off_t offset = lseek(facultyFileDescriptor, (loggedInFaculty.id-1) * sizeof(struct Faculty), SEEK_SET);
            if (offset == -1)
            {
                perror("Error seeking to the faculty record!");
                unlock_critical_section(&semOp);
                return false;
            }

            struct flock lock = {F_WRLCK, SEEK_SET, offset, sizeof(struct Faculty), getpid()};
            int lockingStatus = fcntl(facultyFileDescriptor, F_SETLKW, &lock);
            if (lockingStatus == -1)
            {
                perror("Error obtaining write lock on faculty record!");
                unlock_critical_section(&semOp);
                return false;
            }

            writeBytes = write(facultyFileDescriptor, &loggedInFaculty, sizeof(struct Faculty));
            if (writeBytes == -1)
            {
                perror("Error storing updated faculty password into faculty record!");
                unlock_critical_section(&semOp);
                return false;
            }

            lock.l_type = F_UNLCK;
            lockingStatus = fcntl(facultyFileDescriptor, F_SETLK, &lock);

            close(facultyFileDescriptor);

            writeBytes = write(connFD, PASSWORD_CHANGE_SUCCESS, strlen(PASSWORD_CHANGE_SUCCESS));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); // Dummy read

            unlock_critical_section(&semOp);

            return true;
        }
        else
        {
            // New & reentered passwords don't match
            writeBytes = write(connFD, PASSWORD_CHANGE_NEW_PASS_INVALID, strlen(PASSWORD_CHANGE_NEW_PASS_INVALID));
            readBytes = read(connFD,readBuffer,sizeof(readBuffer));
        }
    }
    else
    {
        // Password doesn't match with old password
        writeBytes = write(connFD, PASSWORD_CHANGE_OLD_PASS_INVALID, strlen(PASSWORD_CHANGE_OLD_PASS_INVALID));
        readBytes = read(connFD,readBuffer,sizeof(readBuffer));
    }

    unlock_critical_section(&semOp);
    return 0;  
}

// locking
bool unlock_critical_section(struct sembuf *semOp)
{
    semOp->sem_op = 1;
    int semopStatus = semop(semid, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while operating on semaphore!");
        _exit(1);
    }
    return true;
}

bool lock_critical_section(struct sembuf *semOp)
{
    semOp->sem_flg = SEM_UNDO;
    semOp->sem_op = -1;
    semOp->sem_num = 0;
    int semopStatus = semop(semid, semOp, 1);
    if (semopStatus == -1)
    {
        perror("Error while locking critical section");
        return false;
    }
    return true;
}


// Adding course details
bool add_course(int connFD) {
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];
    struct Course newCourse;
    struct Course prevCourse;


    int courseFileDescriptor = open(COURSE_FILE, O_RDONLY);
    if (courseFileDescriptor == -1 && errno == ENOENT)
    {
        // Course file was never created
        newCourse.id = 1;
    }
    else if (courseFileDescriptor == -1)
    {
        perror("Error while opening course file");
        return false;
    }
    else
    {
        // getting the id of last record
        int offset = lseek(courseFileDescriptor, -sizeof(struct Course), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last Course record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Course), getpid()};
        int lockingStatus = fcntl(courseFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on Course record!");
            return false;
        }

        readBytes = read(courseFileDescriptor, &prevCourse, sizeof(struct Course));
        if (readBytes == -1)
        {
            perror("Error while reading Course record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(courseFileDescriptor, F_SETLK, &lock);

        close(courseFileDescriptor);

        newCourse.id = prevCourse.id + 1;
    }

    // course name 
    writeBytes = write(connFD, ADD_COURSE_NAME, strlen(ADD_COURSE_NAME));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_COURSE_NAME message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading course name from client!");
        return false;
    }

    //validation for course name
    for(int i = 0; readBuffer[i] != '\0'; i++) {
        if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
            write(connFD,"Invalid  course name ^", sizeof("Invalid  course name ^"));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            return false;
        }
    }
    strcpy(newCourse.name, readBuffer);


    // course department 
    writeBytes = write(connFD, ADD_COURSE_DEPARTMENT, strlen(ADD_COURSE_DEPARTMENT));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_COURSE_DEPARTMENT message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading course department  from client!");
        return false;
    }

    //validation for course department
    for(int i = 0; readBuffer[i] != '\0'; i++) {
        if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
            write(connFD, "Invalid dept name ^", sizeof("Invalid dept name ^"));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            return false;
        }
    }
    strcpy(newCourse.department, readBuffer);

    // no of seats
    writeBytes = write(connFD, ADD_COURSE_SEATS, strlen(ADD_COURSE_SEATS));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_COURSE_SEATS message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading no of seats from client!");
        return false;
    }
    int seats = atoi(readBuffer);

    // validation for no of seats
    if(seats <= 0) {
        write(connFD, "Invalid no of seats ^", sizeof("Invalid no of seats ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
    newCourse.no_of_seats = seats;


    // credits
    writeBytes = write(connFD, ADD_COURSE_CREDITS, strlen(ADD_COURSE_CREDITS));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_COURSE_CREDITS message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading credits from client!");
        return false;
    }
    int credits = atoi(readBuffer);

    // validation for credits
    if(credits <= 0) {
        write(connFD, "Invalid credits ^", sizeof("Invalid credits ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
    newCourse.credits = credits;
     
    
    // no of available seats
    newCourse.no_of_available_seats = seats;

    //coursecode
    char y[4];
    strcpy(newCourse.courseid, "C-");
    sprintf(y , "%d", newCourse.id);
    strcat(newCourse.courseid, y);

    //course faculty name
    strcpy(newCourse.facultyloginid, loggedInFaculty.loginid);

    //course status
    newCourse.status = true;
    
    // creating course record in file
    courseFileDescriptor = open(COURSE_FILE, O_CREAT | O_APPEND | O_WRONLY, S_IRWXU);
    if (courseFileDescriptor == -1)
    {
        perror("Error while creating / opening course file!");
        return false;
    }

    writeBytes = write(courseFileDescriptor, &newCourse, sizeof(struct Course));
    if (writeBytes == -1)
    {
        perror("Error while writing Course record to file!");
        return false;
    }

    close(courseFileDescriptor);
    bzero(writeBuffer, sizeof(writeBuffer));
    
    sprintf(writeBuffer, "%s%s%d", ADD_COURSE_SUCCESS, newCourse.courseid,newCourse.id);
    writeBytes = write(connFD, writeBuffer, sizeof(writeBuffer));
    
    readBytes = read(connFD,readBuffer,sizeof(readBuffer)); //dummy read
    return true;
}
#endif
