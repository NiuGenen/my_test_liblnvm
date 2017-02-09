#include <liblightnvm.h>
#include <nvm.h>
#include <stdio.h>
#include <string.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr lun_addr;

static int channel = 0;
static int lun = 0;

#include "common.h"


int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		return -1;
	}
	geo = nvm_dev_get_geo(dev);
	lun_addr.ppa = 0;
	lun_addr.g.ch = channel;
	lun_addr.g.lun = lun;

	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);
	return 0;
}

static inline int _bbt_idx(const struct nvm_dev *dev,
			   const struct nvm_addr addr)
{
	return addr.g.ch * dev->geo.nluns + addr.g.lun;
}

uint64_t alignblk(struct nvm_addr addr)
{
	struct nvm_addr alg;
	alg.ppa = addr.ppa;
	alg.g.pg = 0;
	alg.g.sec = 0;
	return alg.ppa;
}

int GetPmode(int npl)
{
    int x;
    switch (npl) {
    case 1: 
        x = NVM_FLAG_PMODE_SNGL; break;
    case 2:
        x = NVM_FLAG_PMODE_DUAL; break;
    case 4:
        x = NVM_FLAG_PMODE_QUAD; break;
    default:
        x = -1;
    }
    return x;
}

void EraseNpl_1Blk(struct nvm_addr wh)//
{
	struct nvm_ret ret;
	ssize_t res;
	
	const int npl = geo->nplanes;
    int pmode = GetPmode(npl);
	struct nvm_addr whichblk[npl];
	for(int i = 0; i < npl; ++i){
		whichblk[i].ppa = alignblk(wh);
		whichblk[i].g.pl = i;
	}

	res = nvm_addr_erase(dev, whichblk, npl, pmode, &ret);//Erase 1 block of all planes inside a lun.
	if(res < 0){
		FAIL_ERASE;
		nvm_ret_pr(&ret);
	}
}

void Erase1LunALLBlk(struct nvm_addr lun_addr)//
{
//#define TEST_EraseAllLunBlk_FUNC
#define WAY1 1
#define WAY2 2
#define WAY WAY1
    struct nvm_ret ret;
	ssize_t res;
    int i, j;
	const int npl = geo->nplanes;
    const int nblks = geo->nblocks;
    int pmode = GetPmode(npl);
    if (pmode < 0) {
        PLANE_EINVAL;
        return;
    }
    
    const int max_naddrs_1_time = NVM_NADDR_MAX;
    const int naddrs_1_time = max_naddrs_1_time / npl;

    struct nvm_addr whichblks[NVM_NADDR_MAX];
   
    for(i = 0; i < NVM_NADDR_MAX; ++i){
        whichblks[i].ppa = 0;
        whichblks[i].g.ch = lun_addr.g.ch; 
        whichblks[i].g.lun = lun_addr.g.lun;
    }
    printf("Nblks: %d, Npl: %d\n", nblks, npl);
    int t = 0;
    int erased = 0;
    while (erased < nblks) {
        int issue_nblks = NVM_MIN(nblks - erased, naddrs_1_time);
        int naddrs = issue_nblks * npl;
        //SUM: issue_nblks * npl
        
        if (WAY == WAY1) {
            //Way 1:
            for (i = 0; i < issue_nblks; ++i) {
                for (j = 0; j < npl; j++) {
                    whichblks[j * issue_nblks + i].g.blk = erased + i;
                    whichblks[j * issue_nblks + i].g.pl = j;
                }
            }
        }else{
            //Way 2:
            for (i = 0; i < issue_nblks; ++i) {
                for (j = 0; j < npl; j++) {
                    whichblks[i * npl + j].g.blk = erased + i;
                    whichblks[i * npl + j].g.pl = j;
                }
            }
        }

        
#ifdef TEST_EraseAllLunBlk_FUNC
        char title[10];
        sprintf(title, "idx%d", t);
        My_pr_addr_cap(title);

        for (int idx = 0; idx < naddrs; ++idx) {
            My_pr_nvm_addr(whichblks[idx]);
        }
#else
        res = nvm_addr_erase(dev, whichblks, naddrs, pmode, &ret);//Erase SUM blocks
        if(res < 0){
            FAIL_ERASE;
            printf("%d naadrs From:\n", naddrs);
            My_pr_addr_with_str("Bad Addr", whichblks[0]);
            nvm_ret_pr(&ret);
        }
#endif
        erased += naddrs_1_time;
        t++;
    }
}



void test_get_bbt_1(void)
{
    struct nvm_ret ret = {};
	const struct nvm_bbt *bbt;
    bbt = nvm_bbt_get(dev, lun_addr, &ret);
    if (!bbt) {
        GET_BBT_FAIL;
        nvm_ret_pr(&ret);
    }

    nvm_bbt_pr(bbt);
}
void test_run_erase_blk_1_lun(void)
{
    Erase1LunALLBlk(lun_addr);
}



typedef void (* FuncType) (void);
void RunTests()
{
	FuncType tests[] = { 
        test_run_erase_blk_1_lun
		};
	const char *teststr[] = {
        "test_run_erase_blk_1_lun"
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
