#include "bootpack.h"
#include <stdio.h>

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start, unsigned int end){
	char flg486=0;
	unsigned int eflg, cr0, i;

	eflg=io_load_eflags();//读入eflags
	eflg |=EFLAGS_AC_BIT;//第18位设为1
	io_store_eflags(eflg);//存入
	eflg=io_load_eflags();//读出
	if((eflg & EFLAGS_AC_BIT) != 0) flg486=1;//486就是保持1，386会自动再设为0

	eflg &= ~EFLAGS_AC_BIT;//eflags第18位再设为0
	io_store_eflags(eflg);//保存

	if(flg486 !=0){//是486
		cr0=load_cr0();
		cr0 |=CR0_CACHE_DISABLE;//禁止缓存
		store_cr0(cr0);
	}

	i=memtest_sub(start,end);

	if(flg486!=0){//是486
		cr0=load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;//允许缓存
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man){
	man->frees=0;//可用信息数量
	man->losts=0;//释放内存失败次数
	man->lostsize=0;//释放失败的内存总和
	man->maxfrees=0;//frees最大值
	return;
}

unsigned int memman_total(struct MEMMAN *man){
	unsigned int i,t=0;
	for(i=0;i<man->frees;i++){
		t += man->free[i].size;
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
	unsigned int i,a;
	for(i=0;i<man->frees;i++){
		if(man->free[i].size>=size){
			a = man->free[i].addr;
			man->free[i].addr+=size;
			man->free[i].size -=size;
			if(man->free[i].size==0){
				man->frees--;
				for(;i<man->frees;i++){
					man->free[i]=man->free[i+1];
				}
			}
			return a;//返回地址
		}
	}
	return 0;
}

int memman_free(struct MEMMAN *man, unsigned int addr,unsigned int size){//返回是否成功释放
	int i,j;
	for(i=0;i<man->frees;i++){
		if(man->free[i].addr > addr){
			break;
		}
	}
	if(i>0){
		if(man->free[i-1].addr+man->free[i-1].size==addr){
			man->free[i-1].size+=size;
			if(i<man->frees){
				if(addr+size == man->free[i].addr){
					man->free[i-1].size +=man->free[i].size;
					man->frees--;
					for(;i<man->frees;i++){
						man->free[i]=man->free[i+1];
					}
				}
			}
			return 0;
		}
	}
	if(i<man->frees){
		if(addr + size ==man->free[i].addr){
			man->free[i].addr=addr;
			man->free[i].size+=size;
			return 0;
		}
	}
	if(man->frees < MEMMAN_FREES){
		for(j=man->frees;j>i;j--){
			man->free[j]=man->free[j-1];
		}
		man->frees++;
		if(man->maxfrees<man->frees){
			man->maxfrees=man->frees;
		}
		man->free[i].addr=addr;
		man->free[i].size=size;
		return 0;
	}
	man->losts++;
	man->lostsize+=size;
	return -1;
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size){
    unsigned int a;
    size=(size + 0xfff)&0xfffff000;
    a=memman_alloc(man,size);
    return a;
}

int memman_free_4k(struct MEMMAN *man,unsigned int addr, unsigned int size){
    int i;
    size=(size + 0xfff) & 0xfffff000;
    i=memman_free(man,addr,size);
    return i;
}