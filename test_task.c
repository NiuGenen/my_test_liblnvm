//
// Created by raohui on 17-3-3.
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>
#include <pthread.h>


#define FLUSH_ALL 1

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";


static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr blk_addr;


struct parameter
{
    const struct nvm_bbt *bbt;
    struct nvm_addr *testaddr;

};


int setup(void) {
    dev = nvm_dev_open(nvm_dev_path);
    if (!dev) {
        perror("nvm_dev_open");
    }
    geo = nvm_dev_get_geo(dev);


    return 0;
}

int teardown(void) {
    nvm_dev_close(dev);

    return 0;
}



int test_r_w_e(char *string, struct nvm_addr *testaddr) {
    struct nvm_ret ret;
    char *buf_w = NULL, *buf_r = NULL;

    ssize_t res;
    int ires = 1;
    size_t buf_w_nbytes, buf_r_nbytes;
    int pmode = NVM_FLAG_PMODE_SNGL;



    buf_w_nbytes = geo->sector_nbytes;
    buf_r_nbytes = geo->sector_nbytes;


    buf_w = nvm_buf_alloc(geo, buf_w_nbytes);// Setup buffers
//    buf_w = (char *) malloc(buf_w_nbytes);// Setup buffers
    if (!buf_w) {
        goto exit_naddr;
    }
    strcpy(buf_w,string);


    buf_r = nvm_buf_alloc(geo, buf_r_nbytes);
//    buf_r = (char *) malloc(buf_r_nbytes);
    if (!buf_r) {
        goto exit_naddr;
    }

    /*以防万一首先做擦除操作*/
    res = nvm_addr_erase(dev, testaddr, 1, pmode, &ret);
    if (res < 0) {
        printf("erase error\n");
        goto exit_naddr;

    }


    /*设置要写入的0块0页0sector*/
    testaddr->g.blk = 0;
    testaddr->g.pg = 0;
    testaddr->g.sec = 0;
    nvm_addr_pr(*testaddr);

    res = nvm_addr_write(dev, testaddr, 1, buf_w, NULL, pmode, &ret);
    if (res < 0) {
        printf("write error!\n");
        goto exit_naddr;

    }

    /*从该地址读出来并打印*/
    res = nvm_addr_read(dev, testaddr, 1, buf_r, NULL, pmode, &ret);
    if (res < 0) {

        printf("read error\n");
        goto exit_naddr;

    }

    char *rstring = malloc(strlen(string));
    strcpy(rstring,buf_r);
    printf("%s", rstring);



    exit_naddr:
    free(buf_w);
    free(buf_r);

    return 0;

}

int test_set_bbt(const struct nvm_bbt* bbt,struct nvm_addr *testaddr,enum nvm_bbt_state state){

    struct nvm_ret ret = {};
    struct nvm_bbt *bbt_exp;
    int res;

    nvm_dev_set_bbts_cached(dev, 0);
    bbt_exp = nvm_bbt_alloc_cp(bbt);
    if (!bbt_exp) {
        printf("FAILED: nvm_bbt_get -- prior to nvm_bbt_set");
        return 1;
    }
    bbt_exp->blks[0] = state;
    res = nvm_bbt_set(dev, bbt_exp, &ret);    // Persist changes
    if (res < 0) {
        printf("FAILED: nvm_bbt_set");
        nvm_bbt_free(bbt_exp);
        return 1;
    }

    bbt = nvm_bbt_get(dev, *testaddr, &ret);
    printf("第一个块bbt状态的%x\n", bbt->blks[0]);

    return 0;
}

int test_erase_bad_block(void *arg){

    struct parameter *prm;
    prm = (struct parameter *)arg;
    struct nvm_ret ret;
    ssize_t res;
    int pmode = NVM_FLAG_PMODE_SNGL;

    for (int i = 0; i <prm->bbt->nblks ; ++i) {

        if(prm->bbt->blks[i] == NVM_BBT_BAD){

            if(i > geo->nblocks){
                return 1;
            }
            prm->testaddr->g.blk = i;
            test_set_bbt(prm->bbt,prm->testaddr,NVM_BBT_FREE);
            res = nvm_addr_erase(dev, prm->testaddr, 1, pmode, &ret);


            if (res < 0) {
                printf("erase error\n");
                return 1;
            }

        }
        
    }

    return 0;

}


int main(void) {

    struct nvm_addr addr[1];
    struct nvm_addr *testaddr = addr;
    const struct nvm_bbt *bbt;
    struct nvm_ret ret = {};
    pthread_t thread;
    int ires;
    void *retval;
    struct parameter prm;


    /*设置地址*/
    setup();
    testaddr->ppa = 0;
    testaddr->g.ch = 0;
    testaddr->g.lun = 0;
    testaddr->g.pl = 0;

    /*测试往第一个地址里面写入hello world 并且读出来打印*/
    test_r_w_e("hello world\n", testaddr);

    /*获取bad block table*/
    nvm_dev_set_bbts_cached(dev, 0);
    if (FLUSH_ALL && nvm_bbt_flush_all(dev, &ret)) {
        printf("FAILED: nvm_bbt_flush_all");
        return 1;
    }
    bbt = nvm_bbt_get(dev, *testaddr, &ret);// Verify that we can call it
    if (!bbt)
        return 1;

    if (bbt->nblks != geo->nplanes * geo->nblocks) {
        printf("FAILED: Unexpected value of bbt->nblks");
        return 1;
    }
    printf("该地址的bbt开始的状态为%x\n", bbt->blks[0]);

    /*设置该地址的bbt的状态为bad*/
    test_set_bbt(bbt,testaddr,NVM_BBT_BAD);

    prm.bbt = bbt;
    prm.testaddr = testaddr;
//    test_erase_bad_block(&prm);
    /*开启一个线程执行擦除bad的操作*/

    ires = pthread_create(&thread, NULL, (void *)&test_erase_bad_block, (void *)&prm);
    if (ires != 0) {
        printf("线程创建失败\n");
    } else {
        printf("线程创建成功\n");
    }

    ires = pthread_join(thread, &retval);

    if (ires != 0) {
        printf("线程join失败\n");
    }else{
        printf("线程join成功\n");
    }


    return 0;

}