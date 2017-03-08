#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

#define FLUSH_ALL 1

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static int channel = 0;
static int lun = 0;
static int VERBOSE = 0;
static struct nvm_bbt *bbt;
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr lun_addr;

static enum nvm_bbt_state states[] = {
	NVM_BBT_FREE,
	NVM_BBT_HMRK
};
static int nstates = sizeof(states) / sizeof(states[0]);
int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);

	geo = nvm_dev_get_geo(dev);

	lun_addr.ppa = 0;
	lun_addr.g.ch = channel;
	lun_addr.g.lun = lun;

	return nvm_addr_check(lun_addr, geo);
}

int main(int bbts_cached){

int pmode = NVM_FLAG_PMODE_SNGL;
struct nvm_ret ret = {0,0};
const int naddrs = geo->nplanes * geo->nsectors;
struct nvm_addr addrs[naddrs];

addrs.g.pg=0;
addrs.g.blk=0;

bbt = nvm_bbt_get(dev, lun_addr, &ret);	

buf_w_nbytes = naddrs * geo->sector_nbytes;
meta_w_nbytes = naddrs * geo->meta_nbytes;

buf_w=nvm_buf_alloc(geo, buf_w_nbytes);
meta_w = nvm_buf_alloc(geo, meta_w_nbytes);


char *s="hello world.";
buf=nvm_buf_alloc(geo,geo->page_nbytes);
memcpy(buf,s,strlen(s));

res = nvm_addr_write(dev, &addrs, 1, buf,
		    use_meta ? meta_w : NULL, pmode, &ret);
if(res<0)
    {FAIL_WRITE;}//写入

 ((char*)Buf_read)[0] = '\0';
res = nvm_addr_read(dev, &addrs, 1, buf_read,
		    use_meta ? meta_r : NULL, pmode, &ret);//读出来

if (strcmp((char*)buf_read, s) != 0) {
            NOT_THE_SAME_IO;
        }
nvm_bbt_mark(dev, &addrs[0], 1, NVM_BBT_BAD, &ret);  //标记bad

nvm_addr_erase(dev, addrs, geo->nplanes, pmode, &ret);//擦除bad
nvm_bbt_flush_all(dev, &ret);//擦除bbt

return 0;
}


