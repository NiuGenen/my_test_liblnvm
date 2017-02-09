#ifndef COMMON_H
#define COMMON_H

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
#define GET_BBT_FAIL DEBUG_MSG(GET_BBT_FAIL)
#define PLANE_EINVAL DEBUG_MSG(PLANE_EINVAL)

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


#endif
