#include <liblightnvm.h>
#include <nvm.h>
#include <stdio.h>
#include <string.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
struct nvm_vblk *vblk;

#define FAIL_ERASE do{ printf("Erase Failed\n"); }while(0)

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

    vblk = nvm_vblk_alloc_line(dev, ch_bgn, ch_end, lun_bgn, lun_end, block);

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


static inline int _cmd_nspages(int nblks, int cmd_nspages_max)
{
	int cmd_nspages = cmd_nspages_max;

	while(nblks % cmd_nspages && cmd_nspages > 1) --cmd_nspages;

	return cmd_nspages;
}

void My_pr_addr_cap(const char* str)
{
#define digestlen 8
	char digest[digestlen];
	strncpy(digest, str, digestlen);
	digest[digestlen - 1] = '\0';

	printf("%8s | %s | %s | %s | %-4s | %3s | %s \n",
		digest,"ch", "lun", "pl", "blk", "pg", "sec");
}
void My_pr_nvm_addr(struct nvm_addr addr)
{
	printf("         | %2d | %3d | %2d | %4d | %3d | %d\n",
	       addr.g.ch, addr.g.lun, addr.g.pl,
	       addr.g.blk, addr.g.pg, addr.g.sec);
}

void My_pr_addr_with_str(const char *str, struct nvm_addr x)
{
	My_pr_addr_cap(str);
	My_pr_nvm_addr(x);
}


void test_run_pwrite(void)
{
    nvm_geo_pr(geo);
    int ci[] = {0, 0, 0, 0},
        li[] = {0, 1, 2, 3};
    struct nvm_vblk *vblk2;
    printf("nblks | npages_sum | nthreads | npages/thread | max_npage/thread\n");


    for (int i = 0; i < 4; i++) 
    {
        vblk2 = nvm_vblk_alloc_line(dev, 0, ci[i], 0, li[i], 10);
        nvm_vblk_write(vblk2, NULL, vblk2->nbytes);
        nvm_vblk_free(vblk2);
    }
}

void test_vblk_erase(void)
{
    ssize_t res = 0;
	res = nvm_vblk_erase(vblk);
    if (res < 0) {
        FAIL_ERASE;
    }
}

void test_vblk_erase_addr(void)
{
    struct nvm_ret ret;
	ssize_t res;
    printf("vblk->nblks %d\n", vblk->nblks);
    struct nvm_addr a0;
    a0.ppa = 0;
    a0.g.blk = 10;
    struct nvm_addr addrs[4];
    int pmode = NVM_FLAG_PMODE_SNGL;
    const int lun[4] = { 0,0,1,1};
    const int pl[4]  = { 0,1,0,1};

    for (int i = 0; i < 4; i++) 
    {
        addrs[i].ppa = 0;
        addrs[i].g.lun = lun[i];
        addrs[i].g.pl = pl[i];
        addrs[i].g.blk = 10;
        My_pr_addr_with_str("e", addrs[i]);
    }
    res = nvm_addr_erase(dev, addrs, 2, pmode, &ret);
    if(res < 0){
		FAIL_ERASE;
		nvm_ret_pr(&ret);
    }
}

void test_vblk_pwrite(void)
{
    const int SPAGE_NADDRS = geo->nplanes * geo->nsectors;
	const int CMD_NSPAGES = _cmd_nspages(vblk->nblks,
				vblk->dev->write_naddrs_max / SPAGE_NADDRS);

	const int ALIGN = SPAGE_NADDRS * geo->sector_nbytes;
	const int NTHREADS = vblk->nblks < CMD_NSPAGES ? 1 : vblk->nblks / CMD_NSPAGES;

    int vblk_idx = 0;
    int vblk_size = 2;

    size_t offset = vblk_idx * SPAGE_NADDRS * geo->sector_nbytes;
    int count = vblk->nbytes;

    const size_t bgn = offset / ALIGN;
	const size_t end = bgn + (count / ALIGN);

    nvm_geo_pr(geo);
    printf("need to issue: %d max per: %d\n", count / ALIGN, vblk->dev->write_naddrs_max / SPAGE_NADDRS);
    printf("SPAGE_NADDRS | CMD_NSPAGES | ALIGN | NTHREADS\n");
    printf("%12d | %11d | %5d | %7d\n", SPAGE_NADDRS, CMD_NSPAGES , ALIGN, NTHREADS);

    printf("bgn %zu, end %zu， npage %zu, off:\n", bgn, end, geo->npages); 
    int num = 0;
    for (size_t off = bgn; off < end; off += CMD_NSPAGES) {
        printf("%4zu ", off);
        if (num % 11 == 0){
            printf("\n");
        }
        num++;
    }
    printf("\n");
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
    printf("vblk->nblks: %d\n", vblk->nblks);
    printf("vblk->nbytes: %zu\n", vblk->nbytes);
    printf("a block bytes: %zu\n", geo->npages * geo->nsectors * geo->sector_nbytes);
}




typedef void (* FuncType) (void);
void RunTests()
{
	FuncType tests[] = { 
//          PrPmode,
//          test_vblk_alloc_line_attr,
//          test_cmd_nblks,            
//          test_vblk_pwrite,
//          test_vblk_erase
//          test_vblk_erase_addr
            test_run_pwrite
		};
	const char *teststr[] = {
//          "PrPmode",
//          "test_vblk_alloc_line_attr",
//          "test_cmd_nblks",
//          "test_vblk_pwrite",
//          "test_vblk_erase"
//          "test_vblk_erase_addr"
            "test_run_pwrite"
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
