#include <liblightnvm.h>
#include <stdio.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr a0;

static int channel = 0;
static int lun = 0;
static int plane = 0;
static int block = 10;
static const char * StrPmode[] = {
            "single",
            "dual",
            "quad" 
        };


int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		return -1;
	}
	geo = nvm_dev_get_geo(dev);

	a0.ppa = 0;
	a0.g.ch = channel;
	a0.g.lun = lun;
	a0.g.pl = plane;
	a0.g.blk = block;	//only blk addr

	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);
	return 0;
}

const char* StrPmode(int pmode)
{
    int i;
    int mode[] = {
        NVM_FLAG_PMODE_SNGL,
        NVM_FLAG_PMODE_DUAL,
        NVM_FLAG_PMODE_QUAD
    };
    for (i = 0; i < 3; i++) {
        if (mode[i] == pmode)
            break;
    }
    return StrPmode[i];
}

void PrPmode(void)
{
    printf("Pmode %s\n", StrPmode(dev->pmode));
}



typedef void (* FuncType) (void);
void RunTests()
{
	FuncType tests[] = { 
            PrPmode
		};
	const char *teststr[] = {
            "PrPmode"
        };
	for(int i = 0; i < (sizeof(tests) / sizeof(FuncType)); i++){
		printf("====Test %d : %s====\n", i, teststr[i]);
		tests[i]();
	}
}



int main()
{
    if( setup() < 0){
		goto OUT;
	}
	RunTests();

OUT:
	teardown();
    return 0;
}
