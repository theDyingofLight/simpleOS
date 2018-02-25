#include "bootpack.h"
#include<stdio.h>


#define FLAGS_OVERRUN       0x0001

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task){
    fifo->buf=buf;
    fifo->flags=0;
    fifo->free=size;
    fifo->p=0;
    fifo->q=0;
    fifo->size=size;
    fifo->task=task;//如果不需要任务自动唤醒功能，只要将task置为0
    return;
}


int fifo32_put(struct FIFO32 *fifo, int data){
    if(fifo->free==0){
        fifo->flags |=FLAGS_OVERRUN;//用flags这一变量记录是否溢出
        return -1;
    }
    fifo->buf[fifo->p]=data;
    fifo->p++;
    if(fifo->p==fifo->size) fifo->p=0;
    fifo->free--;
    if(fifo->task!=0){
        if(fifo->task->flags!=2){
            task_run(fifo->task,-1,0);
        }
    }
    return 0;
}

int fifo32_get(struct FIFO32 *fifo){
    int data;
    if(fifo->free==fifo->size) return -1;
    data=fifo->buf[fifo->q];
    fifo->q++;
    if(fifo->q==fifo->size) fifo->q=0;
    fifo->free++;
    return data;
}

int fifo32_status(struct FIFO32 *fifo){
    return fifo->size - fifo->free;
}