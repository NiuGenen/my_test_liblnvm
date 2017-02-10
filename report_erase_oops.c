#include <liblightnvm.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_MSG(MSG) do{ printf("[DEBUG] - "#MSG"\n"); }while(0)
//for io issue:
#define FAIL_ERASE DEBUG_MSG(FAIL_ERASE)
#define FAIL_ALLOC DEBUG_MSG(FAIL_ALLOC)
#define FAIL_WRITE DEBUG_MSG(FAIL_WRITE)
#define FAIL_READ  DEBUG_MSG(FAIL_READ)
#define ADDR_INVALID DEBUG_MSG(ADDR_INVALID)

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;


inline static void My_pr_addr_cap(const char* str)
{
#define digestlen 8
	char digest[digestlen];
	strncpy(digest, str, digestlen);
	digest[digestlen - 1] = '\0';

	printf("%8s | %s | %s | %s | %-4s | %3s | %s \n",
		digest,"ch", "lun", "pl", "blk", "pg", "sec");
}
inline static void My_pr_nvm_addr(struct nvm_addr addr)
{
	printf("         | %2d | %3d | %2d | %4d | %3d | %d\n",
	       addr.g.ch, addr.g.lun, addr.g.pl,
	       addr.g.blk, addr.g.pg, addr.g.sec);
}

inline static void My_pr_addr_with_str(const char *str, struct nvm_addr x)
{
	My_pr_addr_cap(str);
	My_pr_nvm_addr(x);
}


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


void Write_1Sector(struct nvm_addr wh, const char *IO, int use_meta, const char *Meta)
{
	struct nvm_ret ret;
	ssize_t res;
	int pmode = NVM_FLAG_PMODE_SNGL;
	void *bufptr = NULL, *bufptr_meta = NULL;

	int addr_valid = nvm_addr_check(wh, geo);
	if(addr_valid){
		ADDR_INVALID;
		nvm_bounds_pr(addr_valid);
		goto OUT;
	}

	//buf setup
	bufptr = nvm_buf_alloc(geo, geo->sector_nbytes);//sector size
	if(!bufptr){
		FAIL_ALLOC;
		goto OUT;
	}
	memcpy(bufptr, IO, strlen(IO));

	if(use_meta){
		bufptr_meta = nvm_buf_alloc(geo, geo->meta_nbytes);
		if(!bufptr_meta){
			FAIL_ALLOC;
			goto FREE_OUT1;
		}
		memcpy(bufptr_meta, Meta, geo->meta_nbytes);
	}


	//2. write
	res = nvm_addr_write(dev, &wh, 1, bufptr, 
		use_meta ? bufptr_meta : NULL, pmode, &ret);//Write 1 sector
	if(res < 0){
		FAIL_WRITE;
	}

	free(bufptr_meta);
FREE_OUT1:
	free(bufptr);
OUT:
	if(res < 0){
		nvm_ret_pr(&ret);
	}
	return;
}

void Test_Erase1()
{
    struct nvm_ret ret;
	ssize_t res;
    int pl[4] = {0,1,0,1};
    int blk[4] = {0,0,1,1};
    struct nvm_addr a0[4];
    for (int i = 0; i < 4; i++) {
        a0[i].ppa = 0;
        a0[i].g.pl = pl[i];
        a0[i].g.blk = blk[i];
    }
    My_pr_addr_cap("1");
    for (int i = 0; i < 4; i++) {
    	My_pr_nvm_addr(a0[i]);
	}

    res = nvm_addr_erase(dev, a0, 4, NVM_FLAG_PMODE_DUAL, &ret);//Erase 4 blocks
    if(res < 0){
        FAIL_ERASE;
        nvm_ret_pr(&ret);
    }
}

void Test_Erase2()
{
    struct nvm_ret ret;
	ssize_t res;
    int pl[4] = {0,0,1,1};
    int blk[4] = {0,1,0,1};
    struct nvm_addr a0[4];
    for (int i = 0; i < 4; i++) {
        a0[i].ppa = 0;
        a0[i].g.pl = pl[i];
        a0[i].g.blk = blk[i];
    }
    My_pr_addr_cap("2");
    for (int i = 0; i < 4; i++) {
    	My_pr_nvm_addr(a0[i]);
	}
    res = nvm_addr_erase(dev, a0, 4, NVM_FLAG_PMODE_DUAL, &ret);//Erase 4 blocks
    if(res < 0){
        FAIL_ERASE;
        nvm_ret_pr(&ret);
    }
}

void test_E1_ok()
{
    struct nvm_addr a0;
    a0.ppa = 0;
    a0.g.blk = 1;
    Test_Erase1();
    Write_1Sector(a0,"This is the test.", 0, NULL);
}

void test_E2_fail()
{
    struct nvm_addr a0;
    a0.ppa = 0;
    a0.g.blk = 1;
    Test_Erase2();
    Write_1Sector(a0,"This is the test.", 0, NULL);
}

typedef void (* FuncType) (void);
void RunTests()
{
	FuncType tests[] = { 
        test_E1_ok,
        test_E2_fail
		};
	const char *teststr[] = {
        "test_E1_ok",
        "test_E2_fail"
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
