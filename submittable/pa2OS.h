// Author: Yohan Hmaiti, Anna Zheng, Alyssa Yeekee

/*

 The purpose from this header file is to simply facilitate sharing the mutex and the shared memory

 part between the two read and write modules. We use a global variable that represents the maximum to

 be manipulated when it comes to input of characters/array size for the string.



 Additionally, we track also the size of the message and we make the mutex used to control access to the

 shared memory between the read and write modules be an extern variable.

*/

#define MAXLENGTH 1024 // taken from the first programming assignment

#define OVER_READ -100

#define OVER_WRITE -99

#define CORRECT_READ 1

#define CORRECT_WRITE 1


extern struct mutex sharedData_access_mutex;


struct sharedMemory

{

    char data[1024];

    int length;

    int _isFull;

    int _isEmpty;
};