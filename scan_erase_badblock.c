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

int gpmode(int pl_num)
{
    int m;
    switch(pl_num)
    {
        case 1: m = NVM_FLAG_PMODE_SNGL; break;
        case 2: m = NVM_FLAG_PMODE_DUAL; break;
        case 4: m = NVM_FLAG_PMODE_QUAD; break;
        default: m=-1;
    }
    return m;
}

int s_e_badblock()
{
	const struct nvm_bbt *bbt;
    int i,res,pln;
    int pmode = gpmode(geo->nplanes); //set the option mode
	struct nvm_ret ret = {0,0};
    struct nvm_addr erase_nvm_addr;
	nvm_dev_set_bbts_cached(dev, 0);
	if ( nvm_bbt_flush_all(dev, &ret)) {
		return -1;
	}
 	bbt = nvm_bbt_get(dev, lun_addr, &ret);
    erase_nvm_addr = bbt->addr;
    for(i=0; i<bbt->nblks; i++)
    {
        if(bbt->blks[i] == NVM_BBT_BAD)
        {
            pln = i/geo->nblocks;
            erase_nvm_addr.g.blk = i%geo->nblocks;
            erase_nvm_addr.g.pl = pln;
            res = nvm_addr_erase(dev, &erase_nvm_addr, pln, pmode, &ret);
            if(res<0)
            {
                nvm_ret_pr(&ret);
                return -1;
            }
        }
    }
}

int main()
{
    setup();
    s_e_badblock();
    teardown();
    return 1;

}
