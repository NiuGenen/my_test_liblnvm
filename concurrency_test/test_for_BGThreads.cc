#include "BGThreads.h"
#include <unistd.h>
#include <stdio.h>

int Work1(void *arg) { //predefined function type   
    return printf(">>> Arg: %s.\n", (const char *)arg);
}

void Test1() {
    {
        BGThreads bg1;
        bg1.AddSchedule(Work1, (void *)"1");
        bg1.AddSchedule(Work1, (void *)"2");
        bg1.StartAll();

        bg1.AddSchedule(Work1, (void *)"3");
        bg1.AddSchedule(Work1, (void *)"4");
    //  
    //  Barrier();
        bg1.StartAll();
        
    }
}
struct work2arg{
    BGThreads *bg;
    const char *str;
    work2arg(BGThreads *b, const char* s):
        bg(b), str(s){ }
};

int Work2(void *arg){
    work2arg* t = (work2arg*) arg;
    BGThreads *bg = t->bg;
    int x = bg->AddSchedule(Work1, (void *)t->str);
    if (x < 0) {
        printf("add failed.\n");
    }
    delete t;
}


void Test2(){
    BGThreads bg1;
    bg1.AddSchedule(Work2, new work2arg(&bg1, "1x"));
    bg1.AddSchedule(Work2, new work2arg(&bg1, "2x"));
    bg1.AddSchedule(Work2, new work2arg(&bg1, "3x"));
    bg1.AddSchedule(Work2, new work2arg(&bg1, "4x"));
    bg1.StartAll();
    sleep(5);
    printf("Back\n");
}

typedef void (*FuncType)(void);


void Run_Test() {
    FuncType f[] = {
 //       &Test1,
        &Test2
    };
    const char *fname[] = {
 //       "Single Thread Test",
        "Multi Threads Test"
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
