#include <stdio.h>
#include <liblightnvm.h>
#include <nvm.h>
char dpath[] = "/dev/nvme0n1";

void pr_geo(struct nvm_geo *g)
{
	printf("%zu,%zu,%zu,%zu,%zu,%zu\n"
		"%zu,%zu,%zu\n"
		"%zu,%zu,%zu\n", 
		g->nchannels, g->nluns, g->nplanes, g->nblocks, g->npages, g->nsectors, 
		g->page_nbytes, g->sector_nbytes, g->meta_nbytes,
		g->tbytes, g->vblk_nbytes, g->vpg_nbytes);
}

void test_geo()
{
	struct nvm_dev* dev = nvm_dev_open(dpath);
	if (!dev) {
		perror("nvm_dev_open");
		return ;
	}
//	const struct nvm_geo* geo = nvm_dev_get_geo(dev);
//	if(!geo){
//		perror("nvm_dev_get_geo");
//		return ;
//	}
	nvm_geo_pr(&dev->geo);
	nvm_dev_close(dev);
}


int main()
{
	test_geo();
	return 0;
}
