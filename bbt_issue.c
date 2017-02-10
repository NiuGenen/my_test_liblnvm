#include <liblightnvm.h>
#include <malloc.h>
#include <nvm.h>
#include <stdio.h>
#include <string.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr lun_addr;

static int channel = 0;
static int lun = 0;
static void *Buf_for_1_read = NULL; //Buf for reading 1 sector
static void *Buf_for_1_meta_read = NULL; // Buf for reading 1 metadata

const int FirstBLK = 0;
const int SecondBLK = 1;
const int Plane = 0;

const uint16_t State_HBAD = NVM_BBT_HMRK;
const uint16_t State_FREE = NVM_BBT_FREE;

const char* TestStr = "K:Foo;V:Bar";

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

    Buf_for_1_read =  nvm_buf_alloc(geo, 2 * geo->sector_nbytes);
	if (!Buf_for_1_read) {
		FAIL_ALLOC;
		return -1;
	}
 	
 	Buf_for_1_meta_read = nvm_buf_alloc(geo, 2 * geo->meta_nbytes);
 	if (!Buf_for_1_meta_read) {
		FAIL_ALLOC;
		return -1;
	}

	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);
    if ( Buf_for_1_read ) {
		free(Buf_for_1_read);
	}
	if ( Buf_for_1_meta_read) {
		free(Buf_for_1_meta_read);
	}
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
void Read_1Sector(struct nvm_addr wh, int use_meta)
{
	struct nvm_ret ret;
	ssize_t res;

	int pmode = NVM_FLAG_PMODE_SNGL;

	res = nvm_addr_read(dev, &wh, 1, Buf_for_1_read,
		use_meta ? Buf_for_1_meta_read : NULL, pmode, &ret);//only use the first sector_nbytes
	if(res < 0){
		FAIL_READ;
		nvm_ret_pr(&ret);
	}
}


void Erase1LunALLBlk(struct nvm_addr lun_addr)//
{
//#define TEST_EraseAllLunBlk_FUNC
#define WAY1 1
#define WAY2 2
#define WAY WAY2        //WAY1 is not good!!
    struct nvm_ret ret;
	ssize_t res;
    int i, j;
	const int npl = geo->nplanes;
    const int nblks = geo->nblocks;
    int pmode = GetPmode(npl);
//    int pmode = NVM_FLAG_PMODE_SNGL;
    if (pmode < 0) {
        PLANE_EINVAL;
        return;
    }
    
    const int max_naddrs_1_time = NVM_NADDR_MAX;
    const int naddrs_1_time = max_naddrs_1_time / npl;

    struct nvm_addr whichblks[NVM_NADDR_MAX];//for all block
   
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
        
        if (WAY == WAY1) {      //TEST Result: 2 ways of addr layout are both OK.
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
#endif
        res = nvm_addr_erase(dev, whichblks, naddrs, pmode, &ret);//Erase SUM blocks
        if(res < 0){
            FAIL_ERASE;
            printf("%d naadrs From:\n", naddrs);
            My_pr_addr_with_str("Bad Addr", whichblks[0]);
            nvm_ret_pr(&ret);
            printf("Stop\n");
            return ;
        }

        erased += naddrs_1_time;
        t++;
    }
}

void Set1LunALLFree(struct nvm_addr lun_addr)
{    
    struct nvm_ret ret;
	int res;
    int i, j;
	const int npl = geo->nplanes;
    const int nblks = geo->nblocks;

    const int max_naddrs_1_time = NVM_NADDR_MAX;
    const int naddrs_1_time = max_naddrs_1_time / npl;
   

    struct nvm_addr whichblks[NVM_NADDR_MAX];//for all block
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

        if (WAY == WAY1) {      //TEST Result: 2 ways of addr layout are both OK.
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

        res = nvm_bbt_mark(dev, whichblks, naddrs, State_FREE, &ret);
        if (res < 0) {
            MARK_BBT_FAIL;
            printf("%d naadrs From:\n", naddrs);
            My_pr_addr_with_str("Bad Addr", whichblks[0]);
            nvm_ret_pr(&ret);
            printf("Stop\n");
            return ;
        }

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
    My_nvm_bbt_pr(bbt);
}

void test_run_erase_blk_1_lun(void)
{
    Erase1LunALLBlk(lun_addr);
}

void test_write_to_blk(int plnum, int blknum)//write all pages inside a blk of block=blknum, plane=plnum
{
    const int npl = geo->nplanes;
    struct nvm_addr a0;
    a0.ppa = lun_addr.ppa;
    a0.g.pl = plnum;
    a0.g.blk = blknum;

    for (int i = 0; i < geo->npages; i++) {
        a0.g.pg = i;
        Write_1Sector(a0, TestStr, 0, NULL);
        ((char*)Buf_for_1_read)[0] = '\0';
        Read_1Sector(a0, 0);
        if (strcmp((char*)Buf_for_1_read, TestStr) != 0) {
            NOT_THE_SAME_IO;
            My_pr_addr_with_str("not same:", a0);
        }
    }
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
void test_set_cache_bbt(void)
{
    nvm_dev_set_bbts_cached(dev, 1);
}

void test_clean_all_mark_all_free(void)
{
    struct nvm_ret ret;
	ssize_t res;
    Erase1LunALLBlk(lun_addr);
    Set1LunALLFree(lun_addr);
    if(nvm_bbt_flush(dev, lun_addr, &ret)){//flush the lun's bbt
        FLUSH_BBT_FAIL;
        nvm_ret_pr(&ret);
    }
}

void test_write_into_1st_2ed_blk(void)//write something into all pages inside the 1st/2ed block of all plane inside <lun_addr>
{
    for (int i = 0; i < geo->nplanes; i++) {
        test_write_to_blk(i, FirstBLK);
        test_write_to_blk(i, SecondBLK); 
    }
}

void set_1st_2ed_blk_flush(uint16_t flag)
{
    struct nvm_ret ret;
    int res;
    const int nblks = 2;//1st 2ed
    const int naddrs = nblks * geo->nplanes;//the 1st/2ed block of LUN 0 across all plane
    struct nvm_addr addrs[naddrs];
    //*****Sequence: Must Be Like WAY2*****
    for (int i = 0; i < geo->nplanes; i++) {
        for (int j = 0; j < nblks; j++) {
            addrs[j * geo->nplanes + i].ppa = lun_addr.ppa;
            addrs[j * geo->nplanes + i].g.pl = i;
            addrs[j * geo->nplanes + i].g.blk = j;
        }
    }
    //My_pr_naddrs_with_str("Test", addrs, naddrs);
    
    res = nvm_bbt_mark(dev, addrs, naddrs, flag, &ret); 
    if (res < 0) {
       MARK_BBT_FAIL;
       nvm_ret_pr(&ret);
    }
    if(nvm_bbt_flush(dev, lun_addr,&ret)){//flush the lun's bbt
        FLUSH_BBT_FAIL;
        nvm_ret_pr(&ret);
    }
}


void test_set_1st_2ed_blk_bad(void)//set them as HBAD block so that background GC will then GC them.
{
    set_1st_2ed_blk_flush(State_HBAD);
}



void test_GC(void)//**find all HBAD blocks and erase them**(erase operation must be issued to all plane inside a LUN)
{
    struct nvm_ret ret = {};
	const struct nvm_bbt *bbt;
    int nbad = 0;
    ssize_t res;
    bbt = nvm_bbt_get(dev, lun_addr, &ret);
    if (!bbt) {
        GET_BBT_FAIL;
        nvm_ret_pr(&ret);
    }

    const int max_naddrs_1_time = NVM_NADDR_MAX;
    struct nvm_addr whichblks[NVM_NADDR_MAX];//for all block
    int idx = 0;


    //iterator all block, from nvm_bbt_pr
    for (int i = 0; i < bbt->nblks; i += bbt->dev->geo.nplanes) {
        int blk = i / bbt->dev->geo.nplanes;
        int blk_num = blk;
        int pl_num = 0;
        for (int blk = i; blk < (i+ bbt->dev->geo.nplanes); ++blk, ++pl_num) {
            if(bbt->blks[blk] == State_HBAD){
                printf("HBAD: %d %d\n", pl_num, blk_num);

                //setup address array
                whichblks[idx].ppa = lun_addr.ppa;
                whichblks[idx].g.pl = pl_num;
                whichblks[idx].g.blk = blk_num;
                idx++;

                nbad++;
            }
        }
        
        if (max_naddrs_1_time - idx < bbt->dev->geo.nplanes) {
            //do erase.
            
            res = nvm_addr_erase(dev, whichblks, idx, NVM_FLAG_PMODE_DUAL, &ret);
            if(res < 0){
                FAIL_ERASE;
                nvm_ret_pr(&ret);
            }
            
            //set them as FREE again
            if (nvm_bbt_mark(dev, whichblks, idx, State_FREE, &ret) < 0) {
               MARK_BBT_FAIL;
               nvm_ret_pr(&ret);
            }
            if(nvm_bbt_flush(dev, lun_addr,&ret)){//flush the lun's bbt
                FLUSH_BBT_FAIL;
                nvm_ret_pr(&ret);
            } 
            
            idx = 0;
        }

    }
    if (idx > 0) {
        //do erase.
        
        res = nvm_addr_erase(dev, whichblks, idx, NVM_FLAG_PMODE_DUAL, &ret);
        if(res < 0){
            FAIL_ERASE;
            nvm_ret_pr(&ret);
        }
        
        //set them as FREE again
        if (nvm_bbt_mark(dev, whichblks, idx, State_FREE, &ret) < 0) {
           MARK_BBT_FAIL;
           nvm_ret_pr(&ret);
        }
        if(nvm_bbt_flush(dev, lun_addr,&ret)){//flush the lun's bbt
            FLUSH_BBT_FAIL;
            nvm_ret_pr(&ret);
        } 
        
        idx = 0;
    }
    printf("HBAD block number: %d\n", nbad);
}



typedef void (* FuncType) (void);
void RunTests()
{
	FuncType tests[] = { 
//        test_run_erase_blk_1_lun,        
//        test_write_to_1st_blk,
//        test_write_to_2ed_blk,
//        test_get_bbt_1,
//        test_E1_ok,
//        test_E2_fail
        test_set_cache_bbt,     
        test_clean_all_mark_all_free,
        test_get_bbt_1,
        test_write_into_1st_2ed_blk,
        test_set_1st_2ed_blk_bad,
        test_get_bbt_1,
        test_GC,
        test_get_bbt_1
        };
	const char *teststr[] = {
//        "test_run_erase_blk_1_lun",        
//        "test_write_to_1st_blk",
//        "test_write_to_2ed_blk",
//        "test_get_bbt_1",
//        "test_E1_ok",
//        "test_E2_fail"
        "test_set_cache_bbt",
        "test_clean_all_mark_all_free",
        "test_get_bbt_1",
        "test_write_into_1st_2ed_blk",
        "test_set_1st_2ed_blk_bad",
        "test_get_bbt_1",
        "test_GC",
        "test_get_bbt_1"
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
