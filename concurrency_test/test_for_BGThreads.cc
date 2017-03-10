#include "BGThreads.h"
#include <unistd.h>
#include <stdio.h>

int Work1(void *arg) { //predefined function type   
    return printf(">>> Arg: %s.\n", (char *)arg);
}

void Test1() {
    {
        BGThreads bg1;
        bg1.AddSchedule(Work1, (void *)"1");
        bg1.AddSchedule(Work1, (void *)"2");
        bg1.AddSchedule(Work1, (void *)"3");
        bg1.AddSchedule(Work1, (void *)"4");
    //    
    //    Barrier();
        bg1.StartAll();
        
    }
}


typedef void (*FuncType)(void);


void Run_Test() {
    FuncType f[] = {
        &Test1
    };
    const char *fname[] = {
        "Single Thread Test"
    };

    for (int i = 0; i < sizeof(f) / sizeof(FuncType); ++i) {
        printf("Test %d: %s.\n", i, fname[i]);
        f[i]();
    }
}


int main() {
    Run_Test();
    return 0;
}
