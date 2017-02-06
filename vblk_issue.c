#include <liblightnvm.h>
#include <nvm.h>
#include <stdio.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;

static int ch_bgn = 0, 
    ch_end = 0, 
    lun_bgn = 0, 
    lun_end = 1,
    block = 10;


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

	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);
	return 0;
}

const char* GetStrPmode(int pmode)
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


static inline int _cmd_nblks(int nblks, int cmd_nblks_max)
{
	int cmd_nblks = cmd_nblks_max;

	while(nblks % cmd_nblks && cmd_nblks > 1) --cmd_nblks;

	return cmd_nblks;
}


void test_cmd_nblks(void)
{
    printf("%d\n", _cmd_nblks(60, 64 / 2));
}


void PrPmode(void)
{
    printf("Pmode %s\n", GetStrPmode(dev->pmode));
}

void test_vblk_alloc_line_attr(void)
{
    struct nvm_vblk *vblk;
    vblk = nvm_vblk_alloc_line(dev, ch_bgn, ch_end, lun_bgn, lun_end, block);
    printf("vblk->nblks: %d\n", vblk->nblks);
    printf("vblk->nbytes: %zu\n", vblk->nbytes);
    printf("a block bytes: %zu\n", geo->npages * geo->nsectors * geo->sector_nbytes);
}




typedef void (* FuncType) (void);
void RunTests()
{
	FuncType tests[] = { 
//           PrPmode,
//           test_vblk_alloc_line_attr,
             test_cmd_nblks,
		};
	const char *teststr[] = {
//          "PrPmode",
//          "test_vblk_alloc_line_attr",
            "test_cmd_nblks"
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
