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

int teardown()
{
        nvm_dev_close(dev);
        return 0;

}

int set_bad_block()
{
    struct nvm_ret ret = {0,0};
    struct nvm_bbt *bbt_exp;
    const struct nvm_bbt *bbt_act;
    int res;
    nvm_dev_set_bbts_cached(dev, 0);
    if ( nvm_bbt_flush_all(dev, &ret))
    {
        printf("failed nvm_bbt_flush_all\n");
        return -1;
    }
    bbt_exp = nvm_bbt_alloc_cp(nvm_bbt_get(dev, lun_addr, &ret));
    if (!bbt_exp)
    {
        printf("FAILED: nvm_bbt_get -- prior to nvm_bbt_set");
        return -1;
    }
    bbt_exp->blks[1] = NVM_BBT_BAD;
    res = nvm_bbt_set(dev, bbt_exp, &ret);// Persist changes
    if (res < 0)
    {
        printf("FAILED: nvm_bbt_set");
        nvm_bbt_free(bbt_exp);
        return -1;
    }
    bbt_act = nvm_bbt_get(dev, lun_addr, &ret);
    if(!bbt_act)
    {
        printf("failed nvm_bbt_get----after nvm set");
        nvm_bbt_free(bbt_exp);
        return -1;
    }
    printf("%x\n",bbt_act->blks[1]);
    return 1;
}

int main()
{
	setup();
	set_bad_block();
	teardown();
	return 1;
}
