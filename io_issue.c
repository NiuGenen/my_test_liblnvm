#include <liblightnvm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr a0;

static int channel = 0;
static int lun = 0;
static int plane = 0;
static int block = 10;

#define STRLEN 20
#define METALEN 16
static char Str[STRLEN] = "OCSSD Test";
static char Meta[METALEN] = "I am Meta";
static char Meta2[METALEN] = "I am Meta2";
static void *Buf_for_1_read = NULL; //Buf for reading 1 sector
static void *Buf_for_1_meta_read = NULL; // Buf for reading 1 metadata

#include "common.h"

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

uint64_t alignblk(struct nvm_addr addr)
{
	struct nvm_addr alg;
	alg.ppa = addr.ppa;
	alg.g.pg = 0;
	alg.g.sec = 0;
	return alg.ppa;
}
uint64_t Get_addr_for_test()
{
	struct nvm_addr addr;
	addr.ppa = a0.ppa;
	addr.g.pg = 1;
	addr.g.sec = 0;
	return addr.ppa;
}


void EraseNpl_1Blk(struct nvm_addr wh)//
{
	struct nvm_ret ret;
	ssize_t res;
	int pmode = NVM_FLAG_PMODE_SNGL;
	const int npl = geo->nplanes;
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


//pmode = Single-plane, with or without meta
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

void Write_2Sectors(struct nvm_addr wh, const char *IO1, const char *IO2, int use_meta, const char *Meta1, const char *Meta2)
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


	struct nvm_addr whs[2];
	whs[0].ppa = wh.ppa;
	whs[1].ppa = wh.ppa;
	whs[0].g.pl = 0;
	whs[1].g.pl = 1;

	printf("Write 2 Sectors SGNL:\n");
	My_pr_addr_with_str("First", whs[0]);
	My_pr_addr_with_str("Second", whs[1]);
	//buf setup
	bufptr = nvm_buf_alloc(geo, 2 * geo->sector_nbytes);//sector size
	if(!bufptr){
		FAIL_ALLOC;
		goto OUT;
	}
	memcpy(bufptr, IO1, strlen(IO1));
	memcpy(bufptr + 1 * geo->sector_nbytes, IO2, strlen(IO2));

	if(use_meta){
		bufptr_meta = nvm_buf_alloc(geo, 2 * geo->meta_nbytes);
		if(!bufptr_meta){
			FAIL_ALLOC;
			goto FREE_OUT1;
		}
		memcpy(bufptr_meta, Meta1, 
			strlen(Meta1) >= geo->meta_nbytes ? geo->meta_nbytes : strlen(Meta1) );
		memcpy(bufptr_meta + 1 * geo->meta_nbytes, Meta2, 
			strlen(Meta2) >= geo->meta_nbytes ? geo->meta_nbytes : strlen(Meta2) );
	}


	//2. write
	res = nvm_addr_write(dev, whs, 2, bufptr, 
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


//pmode = Dual-plane, with or without meta
void Write_Dual_Sector(struct nvm_addr wh, const char *IO1, const char *IO2, int use_meta, const char *Meta1, const char *Meta2)
{
	struct nvm_ret ret;
	ssize_t res;
	int pmode = NVM_FLAG_PMODE_DUAL;
	void *bufptr = NULL, *bufptr_meta = NULL;

	int addr_valid = nvm_addr_check(wh, geo);
	if(addr_valid){
		ADDR_INVALID;
		nvm_bounds_pr(addr_valid);
		goto OUT;
	}


	struct nvm_addr whs[2];
	whs[0].ppa = wh.ppa;
	whs[1].ppa = wh.ppa;
	whs[0].g.pl = 0;
	whs[1].g.pl = 1;

	printf("Write Dual Sectors:\n");
	My_pr_addr_with_str("First", whs[0]);
	My_pr_addr_with_str("Second", whs[1]);
	//buf setup
	bufptr = nvm_buf_alloc(geo, 2 * geo->sector_nbytes);//sector size
	if(!bufptr){
		FAIL_ALLOC;
		goto OUT;
	}
	memcpy(bufptr, IO1, strlen(IO1));
	memcpy(bufptr + 1 * geo->sector_nbytes, IO2, strlen(IO2));

	if(use_meta){
		bufptr_meta = nvm_buf_alloc(geo, 2 * geo->meta_nbytes);
		if(!bufptr_meta){
			FAIL_ALLOC;
			goto FREE_OUT1;
		}
		memcpy(bufptr_meta, Meta1, 
			strlen(Meta1) >= geo->meta_nbytes ? geo->meta_nbytes : strlen(Meta1) );
		memcpy(bufptr_meta + 1 * geo->meta_nbytes, Meta2, 
			strlen(Meta2) >= geo->meta_nbytes ? geo->meta_nbytes : strlen(Meta2) );
	}


	//2. write
	res = nvm_addr_write(dev, whs, 2, bufptr, 
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

void Read_Dual_Sector(struct nvm_addr wh, int use_meta)
{
	struct nvm_ret ret;
	ssize_t res;

	int pmode = NVM_FLAG_PMODE_SNGL;

	struct nvm_addr whs[2];
	whs[0].ppa = wh.ppa;
	whs[1].ppa = wh.ppa;
	whs[0].g.pl = 0;
	whs[1].g.pl = 1;


	res = nvm_addr_read(dev, whs, 2, Buf_for_1_read,
		use_meta ? Buf_for_1_meta_read : NULL, pmode, &ret);//use 2 * sector_nbytes
	if(res < 0){
		FAIL_READ;
		nvm_ret_pr(&ret);
	}
}




int MemCmp(unsigned char *a, unsigned char *b, int len)
{
	for(int i = 0; i < len; ++i){
		if(a[i] != b[i]){
			return -1;
		}
	}
	return 0;
}

void Test_Write_Dual_4_Sectors(struct nvm_addr whs[], const char *IO1, const char *IO2, 
	const char *IO3, const char *IO4,
	int use_meta, const char *Meta1, const char *Meta2,
	const char *Meta3, const char *Meta4)
{
	char str[16];
	struct nvm_ret ret;
	ssize_t res;
	int pmode = NVM_FLAG_PMODE_DUAL;
	void *bufptr = NULL, *bufptr_meta = NULL;
	int addr_valid;
	for(int i = 0; i < 4; i++){
		addr_valid = nvm_addr_check(whs[i], geo);
		if(addr_valid){
			ADDR_INVALID;
			nvm_bounds_pr(addr_valid);
			goto OUT;
		}
	}

	printf("Write Dual Sectors:\n");
	for(int i = 0; i < 4; i++){
		sprintf(str, "a:%d", i);
		My_pr_addr_with_str(str, whs[i]);
	}

	//buf setup
	bufptr = nvm_buf_alloc(geo, 4 * geo->sector_nbytes);//sector size
	if(!bufptr){
		FAIL_ALLOC;
		goto OUT;
	}
	memcpy(bufptr, IO1, strlen(IO1));
	memcpy(bufptr + 1 * geo->sector_nbytes, IO2, strlen(IO2));
	memcpy(bufptr + 2 * geo->sector_nbytes, IO3, strlen(IO3));
	memcpy(bufptr + 3 * geo->sector_nbytes, IO4, strlen(IO4));



	if(use_meta){
		bufptr_meta = nvm_buf_alloc(geo, 4 * geo->meta_nbytes);
		if(!bufptr_meta){
			FAIL_ALLOC;
			goto FREE_OUT1;
		}
		memcpy(bufptr_meta, Meta1, 
			strlen(Meta1) >= geo->meta_nbytes ? geo->meta_nbytes : strlen(Meta1) );
		memcpy(bufptr_meta + 1 * geo->meta_nbytes, Meta2, 
			strlen(Meta2) >= geo->meta_nbytes ? geo->meta_nbytes : strlen(Meta2) );
		memcpy(bufptr_meta + 2 * geo->meta_nbytes, Meta3, 
			strlen(Meta3) >= geo->meta_nbytes ? geo->meta_nbytes : strlen(Meta3) );
		memcpy(bufptr_meta + 3 * geo->meta_nbytes, Meta4, 
			strlen(Meta4) >= geo->meta_nbytes ? geo->meta_nbytes : strlen(Meta4) );
	}


	//2. write
	res = nvm_addr_write(dev, whs, 4, bufptr, 
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

void test_dual_plane_w_r_multi_4_0_f(void)
{
	struct nvm_addr addr[4];
	int plnum[4] = {0, 0, 0, 0};
	int pgnum[4] = {0, 1, 2, 3};
	for(int i = 0; i < 4; i++){
		addr[i].ppa =Get_addr_for_test();
		addr[i].g.pl = plnum[i];
		addr[i].g.pg = pgnum[i];
	}

	struct nvm_addr addr2[4];
	int plnum2[4] = {1, 1, 1, 1};
	int pgnum2[4] = {0, 1, 2, 3};
	for(int i = 0; i < 4; i++){
		addr2[i].ppa =Get_addr_for_test();
		addr2[i].g.pl = plnum2[i];
		addr2[i].g.pg = pgnum2[i];
	}

	EraseNpl_1Blk(addr[0]);
	Test_Write_Dual_4_Sectors(addr, "IO1", "IO2", "IO3", "IO4",
		1, "Meta1", "Meta2", "Meta3", "Meta4");
	for(int i = 0; i < 4; i++){
		Read_1Sector(addr2[i], 1);
		printf("IO1:%s ||| Meta:%s\n", 
			(char*)Buf_for_1_read, (char*)Buf_for_1_meta_read);
	}
}


void test_dual_plane_w_r_multi_4_0(void)
{
	char str[16];
	struct nvm_addr addr[4];
	int plnum[4] = {0, 0, 0, 0};
	int pgnum[4] = {0, 1, 2, 3};
	for(int i = 0; i < 4; i++){
		addr[i].ppa =Get_addr_for_test();
		addr[i].g.pl = plnum[i];
		addr[i].g.pg = pgnum[i];
	}

	EraseNpl_1Blk(addr[0]);
	Test_Write_Dual_4_Sectors(addr, "IO1", "IO2", "IO3", "IO4",
		1, "Meta1", "Meta2", "Meta3", "Meta4");
	for(int i = 0; i < 4; i++){
		Read_1Sector(addr[i], 1);
		printf("IO1:%s ||| Meta:%s\n", 
			(char*)Buf_for_1_read, (char*)Buf_for_1_meta_read);
	}
}

void test_dual_plane_w_r_multi_3_1(void)
{
	char str[16];
	struct nvm_addr addr[4];
	int plnum[4] = {0, 0, 0, 1};
	int pgnum[4] = {0, 1, 2, 3};
	for(int i = 0; i < 4; i++){
		addr[i].ppa =Get_addr_for_test();
		addr[i].g.pl = plnum[i];
		addr[i].g.pg = pgnum[i];
	}

	EraseNpl_1Blk(addr[0]);
	Test_Write_Dual_4_Sectors(addr, "IO1", "IO2", "IO3", "IO4",
		1, "Meta1", "Meta2", "Meta3", "Meta4");
	for(int i = 0; i < 4; i++){
		Read_1Sector(addr[i], 1);
		printf("IO1:%s ||| Meta:%s\n", 
			(char*)Buf_for_1_read, (char*)Buf_for_1_meta_read);
	}
}


void test_dual_plane_w_r_multi_2_2(void)
{
	char str[16];
	struct nvm_addr addr[4];
	int plnum[4] = {0, 0, 1, 1};
	int pgnum[4] = {0, 1, 0, 1};
	for(int i = 0; i < 4; i++){
		addr[i].ppa =Get_addr_for_test();
		addr[i].g.pl = plnum[i];
		addr[i].g.pg = pgnum[i];
	}

	EraseNpl_1Blk(addr[0]);
	Test_Write_Dual_4_Sectors(addr, "IO1", "IO2", "IO3", "IO4",
		1, "Meta1", "Meta2", "Meta3", "Meta4");
	for(int i = 0; i < 4; i++){
		Read_1Sector(addr[i], 1);
		printf("IO1:%s ||| Meta:%s\n", 
			(char*)Buf_for_1_read, (char*)Buf_for_1_meta_read);
	}
}


void test_sngl_plane_2_w_1_r(void)	//write 2 sectors by sngl-plane, 1 by 1 and read it by dual-plane
{
	struct nvm_addr addr;
	addr.ppa = Get_addr_for_test();
	const char *IO1 = "foo_78654";
	const char *IO2 = "bar_90876";
	const char *Meta1 = "foo_meta";
	const char *Meta2 = "bar_meta";

	EraseNpl_1Blk(addr);
	Write_2Sectors(addr, IO1, IO2, 1, Meta1, Meta2);
	Read_Dual_Sector(addr, 1);
	printf("IO1:%s ||| Meta:%s\n", 
		(char*)Buf_for_1_read, (char*)Buf_for_1_meta_read);
	printf("IO2:%s ||| Meta:%s\n", 
		(char*)(Buf_for_1_read + 1 * geo->sector_nbytes), 
		(char*)(Buf_for_1_meta_read + 1 * geo->meta_nbytes));
}


void test_dual_plane_w_r(void)		//write 2 sectors by dual-plane, read 2 sectors by sngl-plane 
{
	struct nvm_addr addr;
	addr.ppa = Get_addr_for_test();
	const char *IO1 = "foo_78654";
	const char *IO2 = "bar_90876";
	const char *Meta1 = "foo_meta";
	const char *Meta2 = "bar_meta";


	EraseNpl_1Blk(addr);
	Write_Dual_Sector(addr, IO1, IO2, 1, Meta1, Meta2);
	Read_Dual_Sector(addr, 1);
	printf("IO1:%s ||| Meta:%s\n", 
		(char*)Buf_for_1_read, (char*)Buf_for_1_meta_read);
	printf("IO2:%s ||| Meta:%s\n", 
		(char*)(Buf_for_1_read + 1 * geo->sector_nbytes), 
		(char*)(Buf_for_1_meta_read + 1 * geo->meta_nbytes));
}



void test_read_a_none_write_addr(void) // read a sector which not writen before.
{
	struct nvm_addr addr;
	addr.ppa = Get_addr_for_test();
	My_pr_addr_with_str("addr", addr);
	EraseNpl_1Blk(addr);
	Read_1Sector(addr, 1);
}


void test_write_read_meta(void) // meta: a sector a meta or a block a meta.
{
	struct nvm_addr addr, addr2;
	addr.ppa = Get_addr_for_test();
	addr2.ppa = Get_addr_for_test();

	addr2.g.pg += 1;

	const char a0[] = "8765uiytr";
	const char a1[] = "234aygt65";
	const char a2[] = "003 teststring";
	const char a3[] = "004 teststring";


	EraseNpl_1Blk(addr);	//same block, only need 1 erase operation.

	Write_1Sector(addr, a0, 1, a1);
	Write_1Sector(addr2, a2, 1, a3);

	My_pr_addr_with_str("addr1", addr);
	Read_1Sector(addr, 1);
	printf("IO:%s ||| Meta:%s\n", (char*)Buf_for_1_read, (char*)Buf_for_1_meta_read);

	My_pr_addr_with_str("addr2", addr2);
	Read_1Sector(addr2, 1);
	printf("IO:%s ||| Meta:%s\n", (char*)Buf_for_1_read, (char*)Buf_for_1_meta_read);
}


void test_write_no_align(void)
{
	struct nvm_ret ret;
	ssize_t res;
	int pmode = NVM_FLAG_PMODE_SNGL;

	struct nvm_addr addr;
	addr.ppa = Get_addr_for_test();
	void *memptr = nvm_buf_alloc(geo, 2 * geo->sector_nbytes);
	if(!memptr){
		FAIL_ALLOC;
		return ;
	}
	void *memptr_unalign = memptr + 2048;

	printf("%p %p\n", memptr, memptr_unalign);
	printf("memptr_align: %lu memptr_unalign: %lu\n", ((uintptr_t)memptr % geo->sector_nbytes), ((uintptr_t)memptr_unalign % geo->sector_nbytes));
	memcpy(memptr_unalign, Str, STRLEN);


	EraseNpl_1Blk(addr);
	
	res = nvm_addr_write(dev, &addr, 1, memptr_unalign, NULL, pmode, &ret);//Write 1 sector
	if(res < 0){
		FAIL_WRITE;
		goto OUT;
	}

	Read_1Sector(addr, 0);
	if(0 == MemCmp(Str, Buf_for_1_read, strlen(Str))){
		THE_SAME_IO;
	}else{
		NOT_THE_SAME_IO;
	}
OUT:
	free(memptr);
	if(res < 0){
		nvm_ret_pr(&ret);
	}
}


void test_basic(void);
void test_write_no_erase(void)
{
	struct nvm_addr addr;
	addr.ppa = Get_addr_for_test();
	printf("Before test: ");
	test_basic();

	printf("Run test: ");
	Write_1Sector(addr, Str, 0, NULL);
}

void test_erase_1pl_1blk(void)
{
	struct nvm_addr addr;
	struct nvm_ret ret;
	ssize_t res;
	int pmode = NVM_FLAG_PMODE_SNGL;
	const int npl = geo->nplanes;
	struct nvm_addr whichblk[npl];

	addr.ppa = a0.ppa;

	for(int i = 0; i < npl; ++i){
		whichblk[i].ppa = alignblk(addr);
		whichblk[i].g.pl = i;
	}
	res = nvm_addr_erase(dev, whichblk, 1, pmode, &ret);//Erase 1 block of 1 planes inside a lun.
	if(res < 0){
		FAIL_ERASE;
		nvm_ret_pr(&ret);
	}
}

void test_basic(void)
{
	struct nvm_addr addr;
	addr.ppa = Get_addr_for_test();

	EraseNpl_1Blk(addr);
	Write_1Sector(addr, Str, 0, NULL);
	Read_1Sector(addr, 0);
	if(0 == MemCmp(Str, Buf_for_1_read, strlen(Str))){
		THE_SAME_IO;
	}else{
		NOT_THE_SAME_IO;
	}
}

typedef void (* FuncType) (void);
void RunTests()
{
	FuncType tests[] = { 
//			test_basic,
//			test_erase_1pl_1blk,
//			test_write_no_erase,
//			test_write_no_align,
//			test_write_read_meta,
//			test_read_a_none_write_addr,
//			test_sngl_plane_2_w_1_r,
//			test_dual_plane_w_r,
//			test_dual_plane_w_r_multi_2_2,
			test_dual_plane_w_r_multi_3_1,
			test_dual_plane_w_r_multi_4_0,
			test_dual_plane_w_r_multi_4_0_f
		};
	const char *teststr[] = {
//			"test_basic",
//			"test_erase_1pl_1blk",
//			"test_write_no_erase",
//			"test_write_no_align",
//			"test_write_read_meta",
//			"test_read_a_none_write_addr",
//			"test_sngl_plane_2_w_1_r",
//			"test_dual_plane_w_r",
//			"test_dual_plane_w_r_multi_2_2",
			"test_dual_plane_w_r_multi_3_1",
			"test_dual_plane_w_r_multi_4_0",
			"test_dual_plane_w_r_multi_4_0_f"
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