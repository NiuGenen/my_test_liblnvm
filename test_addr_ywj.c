#include <stdio.h>
#include <liblightnvm.h>
#include <nvm.h>

// Managed by setup/teardown and used by tests
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
	}
	geo = nvm_dev_get_geo(dev);

	return 0;
}

int teardown(void)
{
	geo = NULL;
	nvm_dev_close(dev);

	return 0;
}

void pr_nvm_dev_ssw(uint64_t* ssw)
{
	printf("%lu\n", *ssw);
}

void pr_fmt()
{
	nvm_addr_fmt_pr(&dev->fmt);
	nvm_addr_fmt_mask_pr(&dev->mask);
	pr_nvm_dev_ssw(&dev->ssw);
}



int main()
{
	setup();
	pr_fmt();


	teardown();
	return 0;
}