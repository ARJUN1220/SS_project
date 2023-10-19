/*
This file handles all admin functionalities of system like adding student,faculty,viewing their details,modifying their details 
*/


#ifndef ADMIN_FUNCTIONS
#define ADMIN_FUNCTIONS

#include "./common.h"
#include <regex.h>
#include <string.h>
#include <ctype.h>
#define MAX_EMAIL_LENGTH 100


bool admin_operation_handler(int connFD);
bool add_student(int connFD);
int get_student_details(int connFD);
bool add_faculty(int connFD);
int get_faculty_details(int connFD);
bool modify_student_info(int connFD);
bool modify_faculty_info(int connFD);
bool log_out(int connFD);
bool block_student(int connFD);
bool activate_student(int connFD);

bool admin_operation_handler(int connFD)
{
    // handles login of student,faculty and admin
    if(login_handler(1,connFD,NULL,NULL))
    {
        ssize_t writeBytes, readBytes;            
        char readBuffer[1000], writeBuffer[1000]; 
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, LOGIN_SUCCESS);
        while (1)
        {
            strcat(writeBuffer, ADMIN_MENU);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ADMIN_MENU to client!");
                return false;
            }
            bzero(writeBuffer, sizeof(writeBuffer));

            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            if (readBytes == -1)
            {
                perror("Error while reading client's choice for ADMIN_MENU");
                return false;
            }

            int choice = atoi(readBuffer);
            switch (choice)
            {
            case 1:
                add_student(connFD);
                break;
             case 2:
                get_student_details(connFD);
                 break;
            case 3: 
                add_faculty(connFD);
                break;
            case 4:
                get_faculty_details(connFD);
                break;
            case 5:
                activate_student(connFD);
                break;
            case 6:
                block_student(connFD);
                break;
            case 7:
                modify_student_info(connFD);
                break;
            case 8:
                modify_faculty_info(connFD);
                break;
            case 9:
                log_out(connFD);
                break;            
            default:
                writeBytes = write(connFD, "wrong choice ^", sizeof("wrong choice ^"));
                readBytes = read(connFD, readBuffer, sizeof(readBuffer));
                break;
            }
        }
    }
    else
    {
        // ADMIN LOGIN FAILED
        return false;
    }
    return true;
}



// adding student
bool add_student(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];
    struct Student newStudent, previousStudent;

    int studentFileDescriptor = open(STUDENT_FILE, O_RDONLY);
    if (studentFileDescriptor == -1 && errno == ENOENT)
    {
        // Student file was never created
        newStudent.id = 1;
    }
    else if (studentFileDescriptor == -1)
    {
        perror("Error while opening student file");
        return false;
    }
    else
    {
        int offset = lseek(studentFileDescriptor, -sizeof(struct Student), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last student record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};
        int lockingStatus = fcntl(studentFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on student record!");
            return false;
        }

        readBytes = read(studentFileDescriptor, &previousStudent, sizeof(struct Student));
        if (readBytes == -1)
        {
            perror("Error while reading Student record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(studentFileDescriptor, F_SETLK, &lock);
        close(studentFileDescriptor);

        newStudent.id = previousStudent.id + 1;
    }

    //Enter your name:
    writeBytes = write(connFD, ADD_NAME, strlen(ADD_NAME));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_NAME message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading name response from client!");
        return false;
    }
 
    //validation for name
    for(int i = 0; readBuffer[i] != '\0'; i++) {
        if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
            write(connFD, "Invalid name ^", sizeof("Invalid name ^"));
            readBytes= read(connFD, readBuffer, sizeof(readBuffer));
            return false;
        }
    }
    strcpy(newStudent.name, readBuffer);

    // Enter age:
    writeBytes = write(connFD, ADD_AGE, strlen(ADD_AGE));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_AGE message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading age response from client!");
        return false;
    }
    int var = atoi(readBuffer);
    
    //checking for wrong age
    if(var <= 0) {
        write(connFD,"Invalid age ^", sizeof("Invalid age ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
     
    newStudent.age = var;

    // Enter address:
    writeBytes = write(connFD, ADD_ADDRESS, strlen(ADD_ADDRESS));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_ADDRESS message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading address response from client!");
        return false;
    }
    strcpy(newStudent.address, readBuffer);

    // Enter email:
    writeBytes = write(connFD, ADD_EMAIL, strlen(ADD_EMAIL));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_EMAIL message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading email response from client!");
        return false;
    }
    
    //validation for email
    const char *pattern = "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}$";
    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        return false; // Regular expression compilation failed
    }
    int result;

    result = regexec(&regex,readBuffer, 0, NULL, 0);
    regfree(&regex);

    if(result != 0) {
      write(connFD, "Invalid emailid ^", sizeof("Invalid emailid ^"));
      readBytes = read(connFD, readBuffer, sizeof(readBuffer));
      return false; 
    } 
    strcpy(newStudent.email, readBuffer);

    //loginid as name+"-"+system_id    
    strcpy(newStudent.loginid, "ST");
    strcat(newStudent.loginid, "-");
    sprintf(writeBuffer, "%d", newStudent.id);
    strcat(newStudent.loginid, writeBuffer);

    //access
    newStudent.access = true;
    char hashedPassword[1000];
    strcpy(hashedPassword, crypt(AUTOGEN_PASSWORD, SALT_BAE));
    strcpy(newStudent.password, hashedPassword);

    //creating record in file
    studentFileDescriptor = open(STUDENT_FILE, O_CREAT|O_APPEND | O_WRONLY, S_IRWXU);
     if (studentFileDescriptor == -1)
     {
         perror("Error while creating / opening student file!");
         return false;
     }
    writeBytes = write(studentFileDescriptor, &newStudent, sizeof(newStudent));
    if (writeBytes == -1)
    {
        perror("Error while writing Student record to file!");
        return 0;
    }
    close(studentFileDescriptor);
    bzero(writeBuffer, sizeof(writeBuffer));

    //Displaying login id 
    sprintf(writeBuffer, "%s%s\n", ADMIN_ADD_STUDENT_AUTOGEN_LOGIN, newStudent.loginid);
    strcat(writeBuffer, "^");
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error displaying login details");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer)); //dummy read
    return true;
}



//Displaying student details
int get_student_details(int connFD)
{
    int studentID;
    ssize_t readBytes, writeBytes;             
    char readBuffer[1000], writeBuffer[10000]; 
    char tempBuffer[1000];
    struct Student fetchstudent;
    int studentFileDescriptor;
    struct flock lock = {F_RDLCK, SEEK_SET, 0, sizeof(struct Student), getpid()};

    writeBytes = write(connFD, GET_STUDENT_ID, strlen(GET_STUDENT_ID));
    if (writeBytes == -1){
        perror("Error while writing GET_STUDENT_ID message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1){
        perror("Error getting student ID from client!");
        return false;
    }
    bzero(tempBuffer, sizeof(tempBuffer));
    strcpy(tempBuffer, readBuffer);
    strtok(tempBuffer, "-");
    studentID = atoi(strtok(NULL, "-"));
    

    studentFileDescriptor = open(STUDENT_FILE, O_RDONLY);
    if (studentFileDescriptor == -1)
    {
        // Student File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "student id doesn't exists ^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing STUDENT_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));  //dummy read
        return 0;
    }

    //checking whether our record id greater than the last record present
    int offset = lseek(studentFileDescriptor, -sizeof(struct Student), SEEK_END);
    readBytes = read(studentFileDescriptor, &fetchstudent, sizeof(struct Student));
    if (readBytes == -1)
    {
        perror("Error reading student record from file!");
        return false;
    }
    //if then throw error
    if(studentID > fetchstudent.id) {
        write(connFD, "Invalid student id ^", sizeof("Invalid student id ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return 0;
    }
    close(studentFileDescriptor);

    // if not seek to the record and read details
    studentFileDescriptor = open(STUDENT_FILE, O_RDONLY);
    offset = lseek(studentFileDescriptor, (studentID - 1) * sizeof(struct Student), SEEK_SET);
    if (errno == EINVAL)
    {
        // Student record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Student id doesn't exists ^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing STUDENT_ID_DOESNT_EXIT message to client!");
            return 0;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return 0;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required Student record!");
        return false;
    }

    lock.l_start = offset;
    int lockingStatus = fcntl(studentFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining read lock on the Student file!");
        return false;
    }

    readBytes = read(studentFileDescriptor, &fetchstudent, sizeof(struct Student));
    if (readBytes == -1)
    {
        perror("Error reading student record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(studentFileDescriptor, F_SETLK, &lock);
    bzero(writeBuffer, sizeof(writeBuffer));

    sprintf(writeBuffer, "********* Student Details *********  \n\tName: %s\n\tAge : %d\n\tEmail : %s\n\tAddress: %s\n\tLogin-id: %s", fetchstudent.name, fetchstudent.age, fetchstudent.email, fetchstudent.address, fetchstudent.loginid);
    strcat(writeBuffer, "\n\nYou'll now be redirected to the Admin menu ^ \n");

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing student info to client!");
        return false;
    }
    readBytes = read(connFD,readBuffer,sizeof(readBuffer)); //dummy read

    return true;
}


// Viewing facutly details
bool add_faculty(int connFD)
{
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000];
    struct Faculty newFaculty, previousFaculty;

    int facultyFileDescriptor = open(FACULTY_FILE, O_RDONLY);
    if (facultyFileDescriptor == -1 && errno == ENOENT)
    {
        // Facutly file was never created
        newFaculty.id = 1;
    }
    else if (facultyFileDescriptor == -1)
    {
        perror("Error while opening faculty file");
        return false;
    }
    else{
        int offset = lseek(facultyFileDescriptor, -sizeof(struct Faculty), SEEK_END);
        if (offset == -1)
        {
            perror("Error seeking to last faculty record!");
            return false;
        }

        struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Faculty), getpid()};
        int lockingStatus = fcntl(facultyFileDescriptor, F_SETLKW, &lock);
        if (lockingStatus == -1)
        {
            perror("Error obtaining read lock on faculty record!");
            return false;
        }

        readBytes = read(facultyFileDescriptor, &previousFaculty, sizeof(struct Faculty));
        if (readBytes == -1)
        {
            perror("Error while reading faculty record from file!");
            return false;
        }

        lock.l_type = F_UNLCK;
        fcntl(facultyFileDescriptor, F_SETLK, &lock);
        close(facultyFileDescriptor);

        newFaculty.id = previousFaculty.id + 1;
    }

    //Enter your name:
    writeBytes = write(connFD, ADD_NAME, strlen(ADD_NAME));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_NAME message to client!");
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading name response from client!");
        return false;
    }
    
    //validation for name
    for(int i = 0; readBuffer[i] != '\0'; i++) {
        if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
            write(connFD, "Invalid name ^", sizeof("Invalid name ^"));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            return false;
        }
    }
    
    strcpy(newFaculty.name, readBuffer);
    
    
    // Enter Department:
    writeBytes = write(connFD, ENTER_DEPARTMENT, strlen(ENTER_DEPARTMENT));
    if(writeBytes == -1) {
        perror("Error writing Enter department message to client");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes==-1)
    {
        perror("Error reading department response from client!");
        return false;
    }

    //validation for department
    for(int i = 0; readBuffer[i] != '\0'; i++) {
        if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
            write(connFD, "Invalid dept ^", sizeof("Invalid dept ^"));
            readBytes = read(connFD,readBuffer,sizeof(readBuffer));
            return false;
        }
    }

    strcpy(newFaculty.department, readBuffer);
 
    // Enter Designation:
    writeBytes = write(connFD, ENTER_DESIGNATION, strlen(ENTER_DESIGNATION));
    if(writeBytes==-1) {
        perror("Error writing Enter designation message to client");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading designation response from client!");
        return false;
    }
    
    //validation for designation
    for(int i = 0; readBuffer[i] != '\0'; i++) {
        if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
            write(connFD,"Invalid designation ^", sizeof("Invalid designation ^"));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer));
            return false;
        }
    }

    strcpy(newFaculty.designation, readBuffer);

   // Enter email:
    writeBytes = write(connFD, ADD_EMAIL, strlen(ADD_EMAIL));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_EMAIL message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading email response from client!");
        return false;
    }

    // validation for email
    const char *pattern = "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}$";
    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        return false; // Regular expression compilation failed
    }
    int result;

    result = regexec(&regex, readBuffer, 0, NULL, 0);
    regfree(&regex);

    if(result != 0) {
      write(connFD, "Invalid emailid ^", sizeof("Invalid emailid ^"));
      readBytes = read(connFD, readBuffer, sizeof(readBuffer));
      return false; 
    }
    strcpy(newFaculty.email, readBuffer);
    
    // Enter address:
    writeBytes = write(connFD, ADD_ADDRESS, strlen(ADD_ADDRESS));
    if (writeBytes == -1)
    {
        perror("Error writing ADD_ADDRESS message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error reading address response from client!");
        return false;
    }
    strcpy(newFaculty.address, readBuffer);

    //loginid as name+"-"+system_id    
    strcpy(newFaculty.loginid, "FT");
    strcat(newFaculty.loginid, "-");
    sprintf(writeBuffer, "%d", newFaculty.id);
    strcat(newFaculty.loginid, writeBuffer);

    //hashing password using crypt
    char hashedPassword[1000];
    strcpy(hashedPassword, crypt(AUTOGEN_PASSWORD, SALT_BAE));
    strcpy(newFaculty.password, hashedPassword);

    //creating record in a file
    facultyFileDescriptor = open(FACULTY_FILE, O_CREAT|O_APPEND | O_WRONLY, S_IRWXU);
    if (facultyFileDescriptor == -1)
     {
         perror("Error while creating / opening faculty file!");
         return false;
     }
    writeBytes = write(facultyFileDescriptor, &newFaculty, sizeof(newFaculty));
    if (writeBytes == -1)
    {
        perror("Error while writing Faculty record to file!");
        return false;
    }
    bzero(writeBuffer, sizeof(writeBuffer));

    // displaying login id
    sprintf(writeBuffer, "%s%s\n", ADMIN_ADD_FACULTY_AUTOGEN_LOGIN, newFaculty.loginid);
    strcat(writeBuffer, "^");
    
    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error displaying login details");
        return false;
    }
    read(connFD, readBuffer, sizeof(readBuffer)); //dummy read
    
    close(facultyFileDescriptor);
    return true;
}



// Viewing faculty details
int get_faculty_details(int connFD)
{
    int facultyID;
    ssize_t readBytes, writeBytes;             
    char readBuffer[1000], writeBuffer[10000]; 
    char tempBuffer[1000];
    struct Faculty fetchfaculty;
    int facultyFileDescriptor;
    struct flock lock = {F_RDLCK, SEEK_SET, 0, sizeof(struct Faculty), getpid()};

    writeBytes = write(connFD, GET_FACULTY_ID, strlen(GET_FACULTY_ID));
    if (writeBytes == -1){
        perror("Error while writing GET_FACULTY_ID message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1){
        perror("Error getting faculty ID from client!");
        return false;
    }
    bzero(tempBuffer, sizeof(tempBuffer));
    strcpy(tempBuffer, readBuffer);
    strtok(tempBuffer, "-");
    facultyID = atoi(strtok(NULL, "-"));

    facultyFileDescriptor = open(FACULTY_FILE, O_RDONLY);
    if (facultyFileDescriptor == -1)
    {
        // Faculty File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "faculty id doesn't exists ^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing FACULTY_ID_DOESNT_EXIT message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return 0;
    }

    // checking our record id is greater than the last record id present
    int offset = lseek(facultyFileDescriptor, -sizeof(struct Faculty), SEEK_END);
    readBytes = read(facultyFileDescriptor, &fetchfaculty, sizeof(struct Faculty));
    if (readBytes == -1)
    {
        perror("Error reading faculty record from file!");
        return false;
    }

    // if then throw error 
    if(facultyID > fetchfaculty.id) {
        write(connFD, "Invalid faculty id ^", sizeof("Invalid faculty id ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return 0;
    }
    close(facultyFileDescriptor);
    
    // else seek to the record for reading details
    facultyFileDescriptor = open(FACULTY_FILE, O_RDONLY);
    offset = lseek(facultyFileDescriptor, (facultyID - 1) * sizeof(struct Faculty), SEEK_SET);
    lock.l_start = offset;

    int lockingStatus = fcntl(facultyFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining read lock on the Faculty file!");
        return false;
    }

    readBytes = read(facultyFileDescriptor, &fetchfaculty, sizeof(struct Faculty));
    if (readBytes == -1)
    {
        perror("Error reading faculty record from file!");
        return false;
    }

    lock.l_type = F_UNLCK;
    fcntl(facultyFileDescriptor, F_SETLK, &lock);
    bzero(writeBuffer, sizeof(writeBuffer));

    sprintf(writeBuffer, "********* Faculty Details *********  \n\tName: %s\n\tDepartment : %s\n\tDesignation: %s\n\tEmail : %s\n\tAddress: %s\n\tLogin-id: %s", fetchfaculty.name, fetchfaculty.department, fetchfaculty.designation, fetchfaculty.email, fetchfaculty.address, fetchfaculty.loginid);
    strcat(writeBuffer, "\n\nYou'll now be redirected to the Admin menu... ^ ");

    writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
    if (writeBytes == -1)
    {
        perror("Error writing faculty info to client!");
        return 0;
    }
    read(connFD, readBuffer, sizeof(readBuffer)); //dummy read
    
    return 1;
}



// Code for activating student
bool activate_student(int connFD) {
    ssize_t readBytes, writeBytes;             
    char readBuffer[1000], writeBuffer[10000]; 
    char tempBuffer[1000];
    struct Student student;
    int studentfd;
    struct flock lock = {F_WRLCK, SEEK_SET, 0, sizeof(struct Student), getpid()};
    
    writeBytes = write(connFD, ENTER_ACTIVATE_ID, strlen(ENTER_ACTIVATE_ID));
    if(writeBytes == -1) {
        perror("Error writing statement activate id to client");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if(readBytes == -1) {
        perror("error reading activate id response from client");
        return false;
    }

    // fetching id from student code "ST-1..."
    char *ftPosition = strstr(readBuffer, "ST-");
    char *numberStart = NULL;
    int studentID;

    if(ftPosition != NULL) {
        numberStart = ftPosition + strlen("ST-");
        studentID = atoi(numberStart);
    }
    else {
        write(connFD,"wrong studentid ^", sizeof("wrong studentid ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }

    //seeking to the record for reading details
    studentfd = open(STUDENT_FILE, O_RDONLY);
    int offset = lseek(studentfd, (studentID - 1) * sizeof(struct Student), SEEK_SET);
    if(offset == -1) {
        perror("Error while seeking to required student record!");
        return false;
    }
    lock.l_type = F_RDLCK;
    lock.l_start = offset;
    int lockingStatus = fcntl(studentfd, F_SETLKW, &lock);
    if(lockingStatus == -1) {
        perror("Error while obtaining read lock on student record!");
        return false;
    }

    readBytes = read(studentfd, &student, sizeof(struct Student));
    if(readBytes == -1) {
        perror("Error while reading student record from the file!");
        return false;
    }
    else if(readBytes == 0) {
        write(connFD, "wrong student id ^", sizeof("wrong student id ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
    lock.l_type = F_UNLCK;
    lockingStatus = fcntl(studentfd, F_SETLK, &lock);   
    close(studentfd);

    // Checking whether student is already active
    if(student.access) {
       write(connFD, "Already active ^", sizeof("Already active ^"));
       readBytes = read(connFD, readBuffer, sizeof(readBuffer));
       return false;
    } 
    
    //else giving access to student
    student.access = true;
  
    //updating details back into the record
    studentfd = open(STUDENT_FILE, O_WRONLY);
    if(studentfd == -1) {
        perror("Error while opening student file");
        return false;
    }
    offset = lseek(studentfd, (studentID - 1) * sizeof(struct Student), SEEK_SET);
    if(offset == -1) { 
        perror("Error while seeking to required student record!");
        return false;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(studentfd, F_SETLKW, &lock);
    if(lockingStatus == -1) {
        perror("Error while obtaining write lock on student record!");
        return false;
    }
    
    writeBytes = write(studentfd, &student, sizeof(struct Student));
    if(writeBytes == -1) {
        perror("Error while writing update student info into file");
        return false;            
    }

    lock.l_type = F_UNLCK;
    fcntl(studentfd, F_SETLKW, &lock);
    close(studentfd);

    //success message
    writeBytes = write(connFD, "Student activated successfully ^", sizeof("Student activated successfully ^"));
    if(writeBytes == -1) {
        perror("Error while writing student activate success info to client");
        return false;            
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));  //dummy read
    return true; 
}



// Code for bloking student
bool block_student(int connFD){
    ssize_t readBytes, writeBytes;             
    char readBuffer[1000], writeBuffer[10000]; 
    char tempBuffer[1000];
    struct Student student;
    int studentfd;
    struct flock lock = {F_WRLCK, SEEK_SET, 0, sizeof(struct Student), getpid()};
    writeBytes = write(connFD, ENTER_BLOCK_ID, strlen(ENTER_BLOCK_ID));
    if(writeBytes == -1) {
        perror("Error writing statement block id to client");
        return false;
    }

    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if(readBytes == -1) {
        perror("error reading block id response from client");
        return false;
    }

    char *ftPosition = strstr(readBuffer, "ST-");
    char *numberStart = NULL;
    int studentID;

    if(ftPosition != NULL) {
        numberStart = ftPosition + strlen("ST-");
        // Convert the numeric part to an integer
        studentID = atoi(numberStart);
    }
    else {
        write(connFD,"wrong studentid ^", strlen("wrong studentid ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }

    // seeking to particular record for reading data
    studentfd = open(STUDENT_FILE, O_RDONLY);
    int offset = lseek(studentfd, (studentID - 1) * sizeof(struct Student), SEEK_SET);
    if(offset == -1) {
        perror("Error while seeking to required student record!");
        return false;
    }
    lock.l_type = F_RDLCK;
    lock.l_start = offset;
    int lockingStatus = fcntl(studentfd, F_SETLKW, &lock);
    if(lockingStatus == -1) {
        perror("Error while obtaining read lock on student record!");
        return false;
    }

    readBytes = read(studentfd, &student, sizeof(struct Student));
    if(readBytes == -1) {
        perror("Error while reading student record from the file!");
        return false;
    }
    else if(readBytes == 0) {
        write(connFD,"wrong student id ^", strlen("wrong student id ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
    lock.l_type = F_UNLCK;
    lockingStatus = fcntl(studentfd, F_SETLK, &lock);   
    close(studentfd);

    // Checing whether student is already blocked or not 
    if(!student.access) {
       write(connFD, "Already blocked ^", strlen("Already blocked ^"));
       readBytes = read(connFD, readBuffer, sizeof(readBuffer));
       return false;
    } 
    
    //else blocking the student
    student.access = false;
  
    //updating the details again into the record 
    studentfd = open(STUDENT_FILE, O_WRONLY);
    if(studentfd == -1) {
        perror("Error while opening student file");
        return false;
    }
    offset = lseek(studentfd, (studentID - 1) * sizeof(struct Student), SEEK_SET);
    if(offset == -1) {
        perror("Error while seeking to required student record!");
        return false;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(studentfd, F_SETLKW, &lock);
    if(lockingStatus == -1) {
        perror("Error while obtaining write lock on student record!");
        return false;
    }
    
    writeBytes = write(studentfd, &student, sizeof(struct Student));
    if(writeBytes == -1) {
        perror("Error while writing update student info into file");
        return false;            
    }

    lock.l_type = F_UNLCK;
    fcntl(studentfd, F_SETLKW, &lock);
    close(studentfd);

    //displaying success message
    writeBytes = write(connFD, "student blocked successfully ^", sizeof("student blocked successfully ^"));
    if(writeBytes == -1) {
        perror("Error while writing student block success info to client");
        return false;            
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    return true; 
}



// modifying student details
bool modify_student_info(int connFD){
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000], tempBuffer[1000];
    struct Student student;
    int studentID;
    int offset;
    int lockingStatus;

    writeBytes = write(connFD, ADMIN_MOD_STUDENT_ID, strlen(ADMIN_MOD_STUDENT_ID));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_STUDENT_ID message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while reading student ID from client!");
        return false;
    }
    bzero(tempBuffer, sizeof(tempBuffer));
    strcpy(tempBuffer, readBuffer);
    strtok(tempBuffer, "-");
    studentID = atoi(strtok(NULL, "-"));
  
    int studentFileDescriptor = open(STUDENT_FILE, O_RDONLY);
    if (studentFileDescriptor == -1)
    {
        // Student File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Student id doesn't exists ^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing student id doesnt exists message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
    
    // checking whether our record id is greater then the last record id present
    offset = lseek(studentFileDescriptor, -sizeof(struct Student), SEEK_END);
    readBytes = read(studentFileDescriptor, &student, sizeof(struct Student));
    if (readBytes == -1)
    {
        perror("Error reading student record from file!");
        return false;
    }
    // if then throw error
    if(studentID > student.id) {
        write(connFD, "Invalid student id ^", sizeof("Invalid student id ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
    close(studentFileDescriptor);
    
    // else seek to the record for reading details
    studentFileDescriptor = open(STUDENT_FILE, O_RDONLY); 
    offset = lseek(studentFileDescriptor, (studentID - 1)* sizeof(struct Student), SEEK_SET);
    if (errno == EINVAL)
    {
        // Student record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, "Student id doesn't exists ^");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing student id doesnt exists message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required student record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Student), getpid()};

    // Lock the record to be read
    lockingStatus = fcntl(studentFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on student record!");
        return false;
    }

    readBytes = read(studentFileDescriptor, &student, sizeof(struct Student));
    if (readBytes == -1)
    {
        perror("Error while reading student record from the file!");
        return false;
    }

    // Unlock the record
    lock.l_type = F_UNLCK;
    fcntl(studentFileDescriptor, F_SETLK, &lock);

    close(studentFileDescriptor);

    //displaying student menu
    writeBytes = write(connFD, ADMIN_MOD_STUDENT_MENU, strlen(ADMIN_MOD_STUDENT_MENU));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_STUDENT_MENU message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while getting student modification menu choice from client!");
        return false;
    }

    int choice = atoi(readBuffer);

    // validation for non numeric input
    if (choice == 0)
    { 
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return false;
        }
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }

    bzero(readBuffer, sizeof(readBuffer));
    
    switch (choice) { 
     
    // new name 
    case 1:
        writeBytes = write(connFD, ADMIN_MOD_STUDENT_NEW_NAME, strlen(ADMIN_MOD_STUDENT_NEW_NAME));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_STUDENT_NEW_NAME message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for student's new name from client!");
            return false;
        }
        //validation for name
        for(int i = 0; readBuffer[i] != '\0'; i++) {
           if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
              write(connFD, "Invalid name ^", sizeof("Invalid name ^"));
              readBytes = read(connFD, readBuffer, sizeof(readBuffer));
              return false;
            }
        }

        strcpy(student.name, readBuffer);
        break;
    
    // new age
    case 2:
        writeBytes = write(connFD, ADMIN_MOD_STUDENT_NEW_AGE, strlen(ADMIN_MOD_STUDENT_NEW_AGE));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_STUDENT_NEW_AGE message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for student's new age from client!");
            return false;
        }
        int updatedAge = atoi(readBuffer);
        
        //validation for age
        if (updatedAge == 0)
        {
            
            bzero(writeBuffer, sizeof(writeBuffer));
            strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
            writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
            if (writeBytes == -1)
            {
                perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
                return false;
            }
            readBytes = read(connFD,readBuffer,sizeof(readBuffer));
            return false;
        }
        student.age = updatedAge;
        break;
    
    // new emailid
    case 3:
        writeBytes = write(connFD, ADMIN_MOD_STUDENT_NEW_EMAIL, strlen(ADMIN_MOD_STUDENT_NEW_EMAIL));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_STUDENT_NEW_EMAIL message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for student's new email from client!");
            return false;
        }
        
        //validation for email
        const char *pattern = "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}$";
        regex_t regex;
        if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
           return false; // Regular expression compilation failed
        }
        int result;
        result = regexec(&regex, readBuffer, 0, NULL, 0);
        regfree(&regex);
        if(result != 0) {
            write(connFD, "Invalid emailid ^", sizeof("Invalid emailid ^"));
            readBytes = read(connFD,readBuffer,sizeof(readBuffer)); //dummy read
            return false; 
        } 

        strcpy(student.email, readBuffer);
        break;

    // new address    
    case 4:
        writeBytes = write(connFD, ADMIN_MOD_STUDENT_NEW_ADDRESS, strlen(ADMIN_MOD_STUDENT_NEW_ADDRESS));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_STUDENT_NEW_ADDRESS message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for student's new address from client!");
            return false;
        }
        strcpy(student.address, readBuffer);
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

        readBytes = read(connFD,readBuffer,sizeof(readBuffer)); //dummy read
        return false;
    }

    //updating details of record into file
    studentFileDescriptor = open(STUDENT_FILE, O_WRONLY);
    if (studentFileDescriptor == -1)
    {
        perror("Error while opening student file");
        return false;
    }
    offset = lseek(studentFileDescriptor, (studentID - 1) * sizeof(struct Student), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required student record!");
        return false;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(studentFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on student record!");
        return false;
    }

    writeBytes = write(studentFileDescriptor, &student, sizeof(struct Student));
    if (writeBytes == -1)
    {
        perror("Error while writing update student info into file");
    }

    lock.l_type = F_UNLCK;
    fcntl(studentFileDescriptor, F_SETLKW, &lock);

    close(studentFileDescriptor);

    //success message
    writeBytes = write(connFD, ADMIN_MOD_STUDENT_SUCCESS, strlen(ADMIN_MOD_STUDENT_SUCCESS));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_STUDENT_SUCCESS message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    return true;
}



// modifying faculty details
bool modify_faculty_info(int connFD){
    ssize_t readBytes, writeBytes;
    char readBuffer[1000], writeBuffer[1000], tempBuffer[1000];
    struct Faculty faculty;
    int facultyID;
    off_t offset;
    int lockingStatus;

    writeBytes = write(connFD, ADMIN_MOD_FACULTY_ID, strlen(ADMIN_MOD_FACULTY_ID));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_FACULTY_ID message to client!");
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while reading faculty ID from client!");
        return false;
    }
    bzero(tempBuffer, sizeof(tempBuffer));
    strcpy(tempBuffer, readBuffer);
    strtok(tempBuffer, "-");
    facultyID = atoi(strtok(NULL, "-"));

    int facultyFileDescriptor = open(FACULTY_FILE, O_RDONLY);
    if (facultyFileDescriptor == -1)
    {
        // Faculty File doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer,"Faculty id doesn't exists");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing faculty id doesnt exists message to client!");
            return false;
        }
        return false;
    }

    //checking whether our record id is greater than the id of last record
    offset = lseek(facultyFileDescriptor, -sizeof(struct Faculty), SEEK_END);
    readBytes = read(facultyFileDescriptor, &faculty, sizeof(struct Faculty));
    if (readBytes == -1)
    {
        perror("Error reading faculty record from file!");
        return false;
    }

    // if then throw error
    if(facultyID>faculty.id) {
        write(connFD, "Invalid faculty id ^", sizeof("Invalid faculty id ^"));
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }
    close(facultyFileDescriptor);

    //else seek the record to read the details
    facultyFileDescriptor = open(FACULTY_FILE, O_RDONLY);
    offset = lseek(facultyFileDescriptor, (facultyID - 1)* sizeof(struct Faculty), SEEK_SET);
    if (errno == EINVAL)
    {
        // faculty record doesn't exist
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer,"Faculty id doesn't exists");
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing faculty id doesnt exists message to client!");
            return false;
        }
        return false;
    }
    else if (offset == -1)
    {
        perror("Error while seeking to required faculty record!");
        return false;
    }

    struct flock lock = {F_RDLCK, SEEK_SET, offset, sizeof(struct Faculty), getpid()};

    // Lock the record to be read
    lockingStatus = fcntl(facultyFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Couldn't obtain lock on faculty record!");
        return false;
    }

    readBytes = read(facultyFileDescriptor, &faculty, sizeof(struct Faculty));
    if (readBytes == -1)
    {
        perror("Error while reading faculty record from the file!");
        return false;
    }

    // Unlock the record
    lock.l_type = F_UNLCK;
    fcntl(facultyFileDescriptor, F_SETLK, &lock);

    close(facultyFileDescriptor);

    //displaying faculty menu
    writeBytes = write(connFD, ADMIN_MOD_FACULTY_MENU, strlen(ADMIN_MOD_FACULTY_MENU));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_FACULTY_MENU message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    if (readBytes == -1)
    {
        perror("Error while getting faculty modification menu choice from client!");
        return false;
    }

    int choice = atoi(readBuffer);

    //validation of choice
    if (choice == 0)
    { // A non-numeric string was passed to atoi
        bzero(writeBuffer, sizeof(writeBuffer));
        strcpy(writeBuffer, ERRON_INPUT_FOR_NUMBER);
        writeBytes = write(connFD, writeBuffer, strlen(writeBuffer));
        if (writeBytes == -1)
        {
            perror("Error while writing ERRON_INPUT_FOR_NUMBER message to client!");
            return false;
        }
        return false;
    }
    bzero(readBuffer, sizeof(readBuffer));
    
    switch (choice){

    // new name    
    case 1:
        writeBytes = write(connFD, ADMIN_MOD_FACULTY_NEW_NAME, strlen(ADMIN_MOD_FACULTY_NEW_NAME));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_FACULTY_NEW_NAME message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's new name from client!");
            return false;
        }
        
        //validation for name
        for(int i = 0; readBuffer[i] != '\0'; i++) {
           if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
              write(connFD, "Invalid name ^", sizeof("Invalid name ^"));
              readBytes = read(connFD, readBuffer, sizeof(readBuffer));
              return false;
            }
        }

        strcpy(faculty.name, readBuffer);
        break;

    // new dept    
    case 2:
        writeBytes = write(connFD, ADMIN_MOD_FACULTY_NEW_DEPARTMENT, strlen(ADMIN_MOD_FACULTY_NEW_DEPARTMENT));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_FACULTY_NEW_DEPARTMENT message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's new department from client!");
            return false;
        }
        
        //validation for name
        for(int i = 0; readBuffer[i] != '\0'; i++) {
           if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
              write(connFD, "Invalid dept ^", sizeof("Invalid dept ^"));
              readBytes = read(connFD, readBuffer, sizeof(readBuffer));
              return false;
            }
        }
        strcpy(faculty.department, readBuffer);
        break;

    // new designation 
    case 3:
        writeBytes = write(connFD, ADMIN_MOD_FACULTY_NEW_DESIGNATION, strlen(ADMIN_MOD_FACULTY_NEW_DESIGNATION));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_FACULTY_NEW_DESIGNATION message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's new designation from client!");
            return false;
        }

        //validation for designation
        for(int i = 0; readBuffer[i] != '\0'; i++) {
           if(!isalpha(readBuffer[i]) && !isspace(readBuffer[i])) {
              write(connFD, "Invalid designation ^", sizeof("Invalid designation ^"));
              readBytes = read(connFD, readBuffer, sizeof(readBuffer));
              return false;
            }
        }
        strcpy(faculty.designation, readBuffer);
        break;

    // new email
    case 4:
        writeBytes = write(connFD, ADMIN_MOD_FACULTY_NEW_EMAIL, strlen(ADMIN_MOD_FACULTY_NEW_EMAIL));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_FACULTY_NEW_EMAIL message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's new email from client!");
            return false;
        }
        
        //validation for email
        const char *pattern = "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}$";
        regex_t regex;
        if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
           return false; // Regular expression compilation failed
        }
        int result;
        result = regexec(&regex, readBuffer, 0, NULL, 0);
        regfree(&regex);
        if(result != 0) {
            write(connFD, "Invalid emailid ^", sizeof("Invalid emailid ^"));
            readBytes = read(connFD, readBuffer, sizeof(readBuffer)); //dummy read
            return false; 
        } 

        strcpy(faculty.email, readBuffer);
        break;

    // new address
    case 5:
        writeBytes = write(connFD, ADMIN_MOD_FACULTY_NEW_ADDRESS, strlen(ADMIN_MOD_FACULTY_NEW_ADDRESS));
        if (writeBytes == -1)
        {
            perror("Error while writing ADMIN_MOD_FACULTY_NEW_ADDRESS message to client!");
            return false;
        }
        readBytes = read(connFD, &readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
        {
            perror("Error while getting response for faculty's new address from client!");
            return false;
        }
        strcpy(faculty.address, readBuffer);
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
        readBytes = read(connFD, readBuffer, sizeof(readBuffer));
        return false;
    }

    //updating the record into file
    facultyFileDescriptor = open(FACULTY_FILE, O_WRONLY);
    if (facultyFileDescriptor == -1)
    {
        perror("Error while opening faculty file");
        return false;
    }
    offset = lseek(facultyFileDescriptor, (facultyID - 1) * sizeof(struct Faculty), SEEK_SET);
    if (offset == -1)
    {
        perror("Error while seeking to required faculty record!");
        return false;
    }

    lock.l_type = F_WRLCK;
    lock.l_start = offset;
    lockingStatus = fcntl(facultyFileDescriptor, F_SETLKW, &lock);
    if (lockingStatus == -1)
    {
        perror("Error while obtaining write lock on faculty record!");
        return false;
    }

    writeBytes = write(facultyFileDescriptor, &faculty, sizeof(struct Faculty));
    if (writeBytes == -1)
    {
        perror("Error while writing update faculty info into file");
    }

    lock.l_type = F_UNLCK;
    fcntl(facultyFileDescriptor, F_SETLKW, &lock);

    close(facultyFileDescriptor);

    //sucess message
    writeBytes = write(connFD, ADMIN_MOD_FACULTY_SUCCESS, strlen(ADMIN_MOD_FACULTY_SUCCESS));
    if (writeBytes == -1)
    {
        perror("Error while writing ADMIN_MOD_FACULTY_SUCCESS message to client!");
        return false;
    }
    readBytes = read(connFD, readBuffer, sizeof(readBuffer));
    return true;
}



//Logout
bool log_out(int connFD){
    ssize_t writeBytes;
    writeBytes = write(connFD, LOGOUT, strlen(LOGOUT));
    return false;
}
#endif
