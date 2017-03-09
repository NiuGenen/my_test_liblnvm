#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>
#include <inttypes.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr lun_addr;

int setup(void);
int teardown(void);
int test_bbt();

int main()
{
	setup();
	nvm_dev_pr(dev);
	test_bbt();
	teardown();
	return 1;
}

int test_bbt()
{		
	const struct nvm_bbt *bbt;
	struct nvm_ret ret = {0,0};
	nvm_dev_set_bbts_cached(dev, 0);
	if ( nvm_bbt_flush_all(dev, &ret)) {
		return -1;
	}
 	bbt = nvm_bbt_get(dev, lun_addr, &ret);
	if(!bbt)
		return 0;
	printf("nblks:%" PRId64 "\nnbad:%4u\nngbad:%4u\nndmrk:%4u\nnhmrk:%4u\n",bbt->nblks,bbt->nbad,bbt->ngbad,bbt->ndmrk,bbt->nhmrk);
	return 1;
	
}

int setup()
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		return -1;
	}
	geo = nvm_dev_get_geo(dev);
	lun_addr.ppa = 0;
	lun_addr.g.ch = 0;
	lun_addr.g.lun = 0;
	return nvm_addr_check(lun_addr, geo);	
}

int teardown(void)
{
	nvm_dev_close(dev);
	return 0;
}
