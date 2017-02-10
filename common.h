#ifndef COMMON_H
#define COMMON_H
#include <liblightnvm.h>
#include <nvm.h>

#define DEBUG_MSG(MSG) do{ printf("[DEBUG] - "#MSG"\n"); }while(0)

//for io_issue:
#define FAIL_ERASE DEBUG_MSG(FAIL_ERASE)
#define FAIL_ALLOC DEBUG_MSG(FAIL_ALLOC)
#define FAIL_WRITE DEBUG_MSG(FAIL_WRITE)
#define FAIL_READ  DEBUG_MSG(FAIL_READ)
#define ADDR_INVALID DEBUG_MSG(ADDR_INVALID)
#define THE_SAME_IO DEBUG_MSG(THE_SAME_IO) 
#define NOT_THE_SAME_IO DEBUG_MSG(NOT_THE_SAME_IO)
#define THE_SAME_META DEBUG_MSG(THE_SAME_META)
#define NOT_THE_SAME_META DEBUG_MSG(NOT_THE_SAME_META)

//for bbt_issue:
#define PLANE_EINVAL DEBUG_MSG(PLANE_EINVAL)
#define GET_BBT_FAIL DEBUG_MSG(GET_BBT_FAIL)
#define FLUSH_BBT_FAIL DEBUG_MSG(FLUSH_BBT_FAIL)
#define MARK_BBT_FAIL DEBUG_MSG(MARK_BBT_FAIL)
//for common use:
#define NVM_MIN(x, y) ({                \
        __typeof__(x) _min1 = (x);      \
        __typeof__(y) _min2 = (y);      \
        (void) (&_min1 == &_min2);      \
        _min1 < _min2 ? _min1 : _min2; })


inline static void My_pr_addr_cap(const char* str)
{
#define digestlen 8
	char digest[digestlen];
	strncpy(digest, str, digestlen);
	digest[digestlen - 1] = '\0';

	printf("%8s | %s | %s | %s | %-4s | %3s | %s \n",
		digest,"ch", "lun", "pl", "blk", "pg", "sec");
}
inline static void My_pr_nvm_addr(struct nvm_addr addr)
{
	printf("         | %2d | %3d | %2d | %4d | %3d | %d\n",
	       addr.g.ch, addr.g.lun, addr.g.pl,
	       addr.g.blk, addr.g.pg, addr.g.sec);
}

inline static void My_pr_addr_with_str(const char *str, struct nvm_addr x)
{
	My_pr_addr_cap(str);
	My_pr_nvm_addr(x);
}
inline static void My_pr_naddrs_with_str(const char *str, struct nvm_addr x[], int naddrs)
{
	My_pr_addr_cap(str);
    for (int i = 0; i < naddrs; i++) {
        My_pr_nvm_addr(x[i]);
    }
}

inline static void My_nvm_bbt_pr(const struct nvm_bbt *bbt)
{
	int nnotfree = 0;
    const int Pr_num = 4;
    int pred = 0, pr_sr = 0;
	if (!bbt) {
		printf("bbt { NULL }\n");
		return;
	}

	printf("bbt {\n");
	printf("  addr"); nvm_addr_pr(bbt->addr);
	printf("  nblks(%lu) {", bbt->nblks);
	for (int i = 0; i < bbt->nblks; i += bbt->dev->geo.nplanes) {
		int blk = i / bbt->dev->geo.nplanes;
        if (pred < Pr_num/*first Pr_num ones*/ 
            || i == bbt->nblks - bbt->dev->geo.nplanes/*last one*/) {
            printf("\n    blk(%04d): [ ", blk);
            for (int blk = i; blk < (i + bbt->dev->geo.nplanes); ++blk) {
                nvm_bbt_state_pr(bbt->blks[blk]);
                printf(" ");
                if (bbt->blks[blk]) {
                    ++nnotfree;
                }
            }
            printf("]");
            pred++;
        }else if(!pr_sr){
            printf("\n....");
            pr_sr = 1;
        }


	}
	printf("\n  }\n");
	printf("  #notfree(%d)\n", nnotfree);
	printf("}\n");
}


#endif
