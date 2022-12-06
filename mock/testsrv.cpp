#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <dlfcn.h>

namespace testing
{
    class testclass
    {
        public:
        void first_function(long catch_me)
        {
            printf("%ld: I am a function, I do fancy things!\n", catch_me);
        }
    };

}

extern "C" void second_function(long catch_me) {
    printf("%ld: I am another function, I do fancy things too!\n", catch_me);
}

int main()
{

    printf("~~~~~ Hello, I am a mock service (%d).\n", getpid());

    //dlopen("../apinject", 1);

    //printf("%s", dlerror());

    testing::testclass *test = new testing::testclass();

    while (1)
    {
        
        test->first_function(12345);
        second_function(67890);
        sleep(5);
    }

    return 0;
}